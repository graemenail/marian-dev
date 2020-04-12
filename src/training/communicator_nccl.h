// @TODO: rename to communicator_nccl.h
// Note: This must only be included if defined(CUDA_FOUND) && defined(USE_NCCL)
#include "training/communicator.h"
#include "3rd_party/threadpool.h"
#include "tensors/gpu/cuda_helpers.h"

#include "common/timer.h"

// Generated by NCCL make files in build/nccl/include;
// include dir has been set in CMake files. NCCL add version number etc.
#include "nccl.h"
#include <cuda_runtime.h>

#if (NCCL_MAJOR<3 || NCCL_MINOR<2)
#define ncclGetVersion(pv) (*(pv) = (NCCL_MAJOR * 1000 + NCCL_MINOR * 100 + NCCL_PATCH))
#endif

#include <signal.h> // HACK
#include <sys/types.h>
#include <sys/syscall.h>
pid_t gettid(void){ return syscall(SYS_gettid); }

namespace marian {

class NCCLCommunicator : public ICommunicator {
private:
  std::vector<ncclComm_t> comms_;     // [device index]
  std::vector<cudaStream_t> streams_; // [device index]
  std::vector<int> devices_;          // [device index]
  Ptr<IMPIWrapper> mpi_; // (may be null)
  mutable ThreadPool threadPool_;

  void groupStart() const { NCCL_CHECK(ncclGroupStart()); } // helpers to make sure we check the error
  void groupEnd()   const { NCCL_CHECK(ncclGroupEnd());   }

  void synchronizeAll() const {
    for(int i = 0; i < graphs_.size(); ++i) {
      CUDA_CHECK(cudaSetDevice(devices_[i]));
      CUDA_CHECK(cudaStreamSynchronize(streams_[i]));
      // @TODO: why do we sync the CPU, and not the GPU?
      //  - cudaEventRecord() an event on the nccl stream
      //  - submit a cudaStreamWaitEvent() into our compute stream (=NULL stream)
      // cf. https://github.com/pytorch/pytorch/blob/master/torch/lib/c10d/ProcessGroupNCCL.cpp
    }
  }

  void synchronizeAllOnNullStream() const {
    for (int i = 0; i < graphs_.size(); ++i) {
      auto backend = graphs_[i]->params()->vals()->getBackend();
      backend->setDevice();
      backend->synchronize(); // note: synchronize() does not set the device by itself
    }
  }

  std::string mpiIdStr() const { // (for logging)
    return mpi_ ? mpi_->idStr() : "";
  }

  size_t myNcclRank(size_t localDeviceIndex) const { // map local device index to a global rank
    if (mpi_)
      return mpi_->myMPIRank() * devices_.size() + localDeviceIndex;
    else
      return localDeviceIndex;
  }

  size_t numNcclRanks() const { // total number of devices across all MPI processes
    if (mpi_)
      return mpi_->numMPIProcesses() * devices_.size();
    else
      return devices_.size();
  }

  size_t dataSize() const { // total number of floats that comprise the concatenated parameter and gradient vector
    return graphs_[0]->params()->vals()->size();
  }

  // determine the (max) shard size
  // All shards except the last one have this size.
  // Presently, even all shards must have identical size, due to a limitation in NCCL we have not yet worked around.
  size_t shardSize() const {
    size_t numShards = numNcclRanks();
    size_t size = (dataSize() + numShards - 1) / numShards;
#if 1 // for now, all shards must have the same size, since NCCL does not allow a sub-slice for the last shard
    ABORT_IF(size * numShards != dataSize(), "presently, all shards must have the same size");
#endif
    return size;
  }

  // determine the index range (begin, end) of a shard
  std::pair<size_t, size_t> ncclRankShardRange(size_t rank) const {
    size_t size = shardSize();
    size_t begin = rank * size;
    size_t end = begin + size;
    end = std::min(end, dataSize()); // clip last shard. Note: Presently this never happens, since shardSize() enforces uniform shard size.
    return{ begin, end };
  }

  // determine the index range (begin, end) of a shard
  std::pair<size_t, size_t> localShardRange(size_t localDeviceIndex) const {
    return ncclRankShardRange(myNcclRank(localDeviceIndex));
  }

  static std::string ncclVersionString() {
    int ncclVersion = 0;
    ncclGetVersion(&ncclVersion);
    return std::to_string(ncclVersion/1000) + "." + std::to_string((ncclVersion/100)%10) + "." + std::to_string(ncclVersion%100);
  }

  void mpiBarrier() const {
    if (mpi_)
      mpi_->barrier();
  }

  // helper class to temporarily block a UNIX signal
  class BlockSignal {
    typedef std::function<void(int, const sigset_t*, sigset_t*)> SigMaskFn;
    SigMaskFn sigMaskFn_; // function to set the mask, thread or proc
    sigset_t oldSigSet_;  // old set to restore the signal
  public:
    BlockSignal(int signal, const SigMaskFn& sigMaskFn) : sigMaskFn_(sigMaskFn) {
      sigset_t newSigSet;
      sigemptyset(&newSigSet);
      sigaddset(&newSigSet, signal); // block signal by setting it in the blocked-signal mask
      sigMaskFn_(SIG_BLOCK, &newSigSet, &oldSigSet_);
    }
    ~BlockSignal() {
      sigMaskFn_(SIG_BLOCK, &oldSigSet_, nullptr); // restore old signal mask
    }
  };

public:
  // a NCCLCommunicator is bound to a set of graphs, one per GPU device
  // If MPI is used, then each MPI process has an instance of this class for its specific
  // set of GPU devices, which are communicating with each other. The total number of GPUs
  // involved in the NCCL communication setup is (#MPI processes) x (#GPUs per process).
  NCCLCommunicator(const std::vector<Ptr<ExpressionGraph>>& graphs, Ptr<IMPIWrapper> mpi)
      : ICommunicator(graphs),
        comms_(graphs.size()),
        streams_(graphs.size()),
        devices_(graphs.size()),
        mpi_(mpi),
        threadPool_(graphs.size(), graphs.size()) {
    mpiBarrier(); // barrier to group the multiple log messages from MPI processes
    LOG(info, "[comm] Using NCCL {} {}for GPU communication",
        ncclVersionString(),
        (mpi_ && mpi_->numMPIProcesses() > 1) ? "and MPI " : "");
    mpiBarrier(); // (synchronize the log messages)

    // set up our local devices
    for(int i = 0; i < graphs_.size(); ++i) {
      auto device = graphs_[i]->getBackend()->getDeviceId();

      ABORT_IF(device.type != DeviceType::gpu,
               "NCCL communicator can only be used with GPUs");

      devices_[i] = device.no;
      CUDA_CHECK(cudaSetDevice(devices_[i]));
      CUDA_CHECK(cudaStreamCreate(&streams_[i]));
    }

    // Note: due to a bug in NCCL 2.3.5, NCCL's allocation of shared memory intermittently fails with
    //          Failed, NCCL error 2 'unhandled system error' - ncclGroupEnd()
    //          include/shm.h:26 NCCL WARN Unable to allocate shared memory (4263936 bytes) : Interrupted system call
    // This is caused by SIGPROF signals being raised, causing EINTR, which NCCL does not handle.
    // Reported as Issue #137 on the NCCL Github, and supposedly fixed for 2.3.7 (to be verified).
    // To work around, we disable the SIGPROF signal during NCCL initialization.
#define SIG_BAD 27 // SIGPROF
    BlockSignal blockThread(SIG_BAD, pthread_sigmask); // Note: I don't know yet which of these two makes the difference.
    BlockSignal blockProc(SIG_BAD, sigprocmask);       // So for now just block both.

    // set up NCCL
    // Since we want to use MPI, we cannot use NCCL's handy convenience function. Instead, we must go the laborious route.
    // cf. https://docs.nvidia.com/deeplearning/sdk/nccl-developer-guide/index.html#multidevprothrd

    // generate NCCL unique ID at one process and broadcast to all
    ncclUniqueId uniqueId = { 0 };
    if (!mpi_ || mpi->myMPIRank() == 0)
      NCCL_CHECK(ncclGetUniqueId(&uniqueId));

    if (mpi_) {
      static_assert(sizeof(uniqueId) == NCCL_UNIQUE_ID_BYTES, "wrong NCCL_UNIQUE_ID_BYTES??"); // (this value is used in NVidia examples)
      mpi_->bCast(&uniqueId, sizeof(uniqueId), MPI_BYTE, 0);
    }

    groupStart();
    for (int localDeviceIndex = 0; localDeviceIndex < devices_.size(); localDeviceIndex++) {
      CUDA_CHECK(cudaSetDevice(devices_[localDeviceIndex]));
      NCCL_CHECK(ncclCommInitRank(&comms_[localDeviceIndex], numNcclRanks(), uniqueId, myNcclRank(localDeviceIndex)));
    }
    groupEnd();

    mpiBarrier(); // (synchronize the log messages)
    LOG(info, "[comm] NCCLCommunicator constructed successfully");
    mpiBarrier(); // (synchronize the log messages)
  }

  ~NCCLCommunicator() override {
    for(int i = 0; i < devices_.size(); ++i) {
      cudaSetDevice(devices_[i]);
      cudaStreamDestroy(streams_[i]);
      ncclCommDestroy(comms_[i]);
    }
  }

  template <typename Ret>
  Ret foreachAcc(const ForeachFunc<Ret>& func, const AccFunc<Ret>& acc, Ret init, bool parallel = true) const {
    parallel &= graphs_.size() > 1;

    Ret retValue = init;
    std::vector<std::future<Ret>> threadResults(graphs_.size()); // [device index]
    for(size_t i = 0; i < graphs_.size(); ++i) {
      size_t begin, end; std::tie
      (begin, end) = localShardRange(i);
      if (parallel)
        threadResults[i] = threadPool_.enqueue(func, i, begin, end);
      else
        acc(retValue, func(i, begin, end));
    }
    if(parallel)
       for(auto& task : threadResults)
          acc(retValue, task.get());

    return retValue;
  }

  float foreach(const ForeachFunc<float>& func, AccFunc<float> acc, float init, bool parallel = true) const override {
    return foreachAcc(func, acc, init, parallel);
  }

  bool foreach(const ForeachFunc<bool>& func, bool parallel = true) const override {
    AccFunc<bool> allTrue = [](bool& x, bool y) { x = x && y; };
    return foreachAcc(func, allTrue, true, parallel);
  }

  void scatterReduceAndResetGrads() const override {
    synchronizeAllOnNullStream();

    groupStart();
    for(int i = 0; i < graphs_.size(); ++i) {
      size_t begin, end; std::tie
      (begin, end) = localShardRange(i);

      auto grads = graphs_[i]->params()->grads();
      const auto* sendbuf = grads->data();
      auto*       recvbuf = grads->subtensor(begin, end-begin)->data();
      size_t      bufsize = shardSize();
      ABORT_IF(grads->subtensor(begin, end-begin)->size() != bufsize, "unexpected subtensor size??");

      ncclDataType_t ncclFloatType = ncclFloat32;
      if(grads->type() == Type::float16)
        ncclFloatType = ncclFloat16;

      NCCL_CHECK(ncclReduceScatter(sendbuf, recvbuf, bufsize, ncclFloatType, ncclSum, comms_[i], streams_[i]));
    }
    groupEnd();
    synchronizeAll();

    // reset gradients
    // In the future, we can keep quantization residuals here straight in the grads themselves.
    // @TODO: all the different places where gradients get reset are confusing
    auto resetGrads = [&](size_t i, size_t begin, size_t end) {
      auto grads = graphs_[i]->params()->grads();
      auto size = grads->size();
      // reset everything outside the shard that we reduce in
      if (begin > 0)
        grads->subtensor(0, begin)->set(0.f);
      if (end < size)
        grads->subtensor(end, size - end)->set(0.f);

      return true; // dummy success
    };
    foreach(resetGrads);
  }

  // This distributes all 64 model shards to all 64 GPUs.
  // @TODO: For unknown reasons, this takes longer than any other operation incl. scatterReduceAndResetGrads().
  //        But both should have the same number of data transfers of the same size.
  void allGatherParams() const override {
    synchronizeAllOnNullStream();

    groupStart();
    for(int i = 0; i < graphs_.size(); ++i) {
      size_t begin, end; std::tie
      (begin, end) = localShardRange(i);

      auto vals = graphs_[i]->params()->vals();
      const auto* sendbuf = vals->subtensor(begin, end-begin)->data();
      void*       recvbuf = vals->data();
      size_t      bufsize = shardSize();

      ncclDataType_t ncclFloatType = ncclFloat32;
      if(vals->type() == Type::float16)
        ncclFloatType = ncclFloat16;

      NCCL_CHECK(ncclAllGather(sendbuf, recvbuf, bufsize, ncclFloatType, comms_[i], streams_[i]));
    }
    groupEnd();
    synchronizeAll();
  }

  // swap distributed paramShards with model params()
  // It is assumed that all model params() on all devices and MPI processes are identical.
  // This is used for the smoothed parameters.
  void swapParams(const std::vector<Tensor>& distributedParamShards) const override {
    ABORT("not implemented");
    // get everything onto the CPU
    // auto distributedParams = gatherState([&](size_t localDeviceIndex) {
    //   return distributedParamShards[localDeviceIndex]->toItem("dummy");
    // });
    // // Now all MPI processes hold an identical copy of a concatenation of all distributedParamShards[] across local and remote devices.
    // std::vector<float> localParams;
    // graphs_[0]->params()->vals()->get(localParams);
    // // Now all MPI processes hold an identical copy of params() (remember, we assumed all devices hold the same params()).
    // ABORT_IF(distributedParams.size() != localParams.size(), "distributed sharded and local params have different size??");

    // // swap
    // std::swap(distributedParams, localParams);

    // // distribute it back
    // scatterState(distributedParams, [&](size_t localDeviceIndex,
    //                                     std::vector<float>::const_iterator begin,
    //                                     std::vector<float>::const_iterator end){
    //   ABORT_IF(distributedParamShards[localDeviceIndex]->size() != end-begin, "swapParams size mismatch??"); // @TODO: move check to set()
    //   distributedParamShards[localDeviceIndex]->set(std::vector<float>(begin, end)); // @TODO: directly pass iterators to set()
    // });
    // for (auto& graph : graphs_) // broadcast to every local graph
    //   graph->params()->vals()->set(localParams);
  }

  // Distribute a single CPU-side vector to shards across multiple devices and MPI processes.
  // This is used when restoring optimizer state, which is sharded, and as part of swapParams().
  // It is assumed that all MPI processes get the same data() passed. Hence, no MPI transfers are needed here.
  void scatterState(const io::Item& data, const OptimizerBase::ScatterStateSetFunc& setFn) const override {
    size_t dataSize = data.size();
    size_t numShards = numNcclRanks();
    size_t shardSize = (dataSize + numShards - 1) / numShards;
    for(size_t localDeviceIndex = 0; localDeviceIndex < graphs_.size(); localDeviceIndex++) {
      // We only slice out data that is kept in our MPI process. Remember that all MPI processes receive the same, complete data.
      auto ncclRank = myNcclRank(localDeviceIndex);
      size_t begin = ncclRank * shardSize;
      size_t end   = std::min(begin + shardSize, dataSize);
      setFn(localDeviceIndex, data.bytes.data() + begin, data.bytes.data() + end);
    }
  }

  // Collect shards across multiple devices and MPI processes in the NCCL configuration into a single CPU-side vector.
  // This is used when persisting optimizer state, which is sharded, and as part of swapParams().
  io::Item gatherState(const OptimizerBase::GatherStateGetFunc& getFn) const override {
    // first, concatenate over all local devices
    io::Item localData = getFn(0);
    for(size_t localDeviceIndex = 1; localDeviceIndex < graphs_.size(); localDeviceIndex++) {
      localData.append(getFn(localDeviceIndex));
    }
    // second, concatenate across MPI processes
    // Note that all local devices occupy consecutive ncclRanks in order.
    io::Item data;
    if (mpi_) {
      io::Item tmp; // (temp buffer used multiple times)
      // push one rank's data at a time using broadcast
      for(size_t mpiRank = 0; mpiRank < mpi_->numMPIProcesses(); mpiRank++) {
        // broadcast mpiRank's localData to all
        if(mpiRank == mpi_->myMPIRank()) {
          tmp = localData;
        }
        mpi_->bCast(tmp, /*rootRank=*/mpiRank);
        // now all ranks have the same slice: concatenate (we will end up with the same on all MPI processes)
        if(mpiRank == 0)
          data = tmp;
        else
          data.append(tmp);
      }
    }
    else { // no MPI: localData is the complete result already
      data = std::move(localData);
    }
    return data;
  }
};

}  // namespace marian
