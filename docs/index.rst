Welcome to Marian's documentation!
==================================

|buildgpu| |buildcpu| |tests| |release| |license| |gitter|

Marian is an efficient and self-contained Neural Machine Translation framework
with an integrated automatic differentiation engine based on dynamic
computation graphs, written entirely in C++.

- It is written completely in C++11 and intentionally does not provide Python
  bindings: model code and meta-algorithms are meant to be implemented in
  efficient C++ code.
- It is self-contained with its own back end, which provides reverse-mode
  automatic differentiation based on dynamic graphs.
- It has minimal dependencies: CUDA or a BLAS library. Boost will be removed in
  the near future.
- It enables barrier-free optimization at all levels: meta-algorithms such as
  MPI-based multi-node training, efficient batched beam search, compact
  implementations of new models, custom operators, and custom GPU kernels.


.. toctree::
   :maxdepth: 2
   :caption: Contents:

   install.rst

   examples.rst

   reference/memory.rst
   reference/cli.rst
   reference/data.rst
   reference/graph.rst
   reference/encdec.rst
   reference/inference.rst
   reference/debug.rst
   reference/testing.rst

   notes.rst
   faq.rst

   api/library_index.rst

   ../CONTRIBUTING.md 


Indices and tables

* :ref:`genindex`
* :ref:`modindex`
* :ref:`search`


.. |buildgpu| image:: https://img.shields.io/jenkins/s/http/vali.inf.ed.ac.uk/jenkins/view/marian/job/marian-dev.svg?label=CUDA
   :target: http://vali.inf.ed.ac.uk/jenkins/job/marian-dev/
   :alt: Build status

.. |buildcpu| image:: https://img.shields.io/jenkins/s/http/vali.inf.ed.ac.uk/jenkins/view/marian/job/marian-dev-cpu.svg?label=CPU
   :target: http://vali.inf.ed.ac.uk/jenkins/job/marian-dev-cpu/
   :alt: CPU build status

.. |tests| image:: https://img.shields.io/jenkins/s/http/vali.inf.ed.ac.uk/jenkins/view/marian/job/marian-regression-tests.svg?label=tests
   :target: http://vali.inf.ed.ac.uk/jenkins/job/marian-regression-tests/
   :alt: Tests status

.. |release| image:: https://img.shields.io/github/release/marian-nmt/marian.svg?label=release
   :target: https://github.com/marian-nmt/marian/releases
   :alt: Latest release

.. |license| image:: https://img.shields.io/badge/License-MIT-blue.svg
   :target: ../LICENSE.md
   :alt: License: MIT

.. |gitter| image:: https://img.shields.io/gitter/room/marian-nmt/home.svg
   :target: https://gitter.im/marian-nmt/home
   :alt: Chat on Gitter

.. |twitter| image:: https://img.shields.io/twitter/follow/marian_nmt.svg?style=social
   :target: https://twitter.com/intent/follow?screen_name=marian_nmt
   :alt: Twitter
