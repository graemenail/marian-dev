# Marian NMT code documentation and API

This directory contains code documentation and library API for developers of
Marian NMT.

The documentation is generated using Sphinx + Breathe + Doxygen + Exhale.


## Installation

On Ubuntu, install the following packages:

    sudo apt-get install python3-sphinx python3-pip python3-setuptools
    pip3 install --upgrade exhale recommonmark

Documentation building should also work on Windows, but has not been tested
yet.


## Generation

Run:

    make html

If no errors occur, the website will be generated into `_build/html`. Open
_index.html_ in your browser.


## Writing documentation

See the 'Writing documentation' section in the generated HTML documentation or
`writing_docs.rst`.
