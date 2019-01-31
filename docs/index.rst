Welcome to Marian's documentation!
==================================

|buildgpu| |buildcpu| |tests| |release| |license| |gitter|

.. toctree::
   :maxdepth: 2
   :caption: Contents:

   overview.rst

   .. motivating examples with explanation
   examples/iris.rst

   .. memory allocation
   reference/memory.rst
   .. graph construction, node types, operators, layers, graph execution
   reference/graph.rst
   .. adding program options, passing them deeper
   reference/cli.rst
   .. vocabs, batch specification
   reference/data.rst
   .. adding new models
   reference/models.rst
   .. beam search, n-best lists
   reference/inference.rst
   .. debug(), gdb
   reference/debugging.rst
   .. unit tests, regression tests
   reference/testing.rst

   notes/faq.rst
   notes/3rd_party.rst
   notes/windows.rst

   .. automatically generated API
   api/library_index

   .. code style, writing documentation, sphinx cheatsheet
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
