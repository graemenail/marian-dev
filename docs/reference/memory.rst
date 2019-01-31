Memory management
=================

For efficiency, Marian avoids using bare C/C++ pointers and provides
:func:`marian::New` functions creating shared pointers:

.. doxygenfunction:: marian::New(Args&&...)
.. doxygenfunction:: marian::New(Ptr<T>)

And a few aliases for smart pointers:

.. doxygentypedef:: marian::Ptr
.. doxygentypedef:: marian::UPtr
.. doxygentypedef:: marian::Weak

In vast majority of cases, :type:`marian::Ptr` is the one that needs to be used.

Usage example:

.. code-block:: cpp

  Ptr<CorpusBase> dataset = New<Corpus>(options);

