# -*- coding: utf-8 -*-
#
# Configuration file for the Sphinx documentation builder.
#
# This file does only contain a selection of the most common options. For a
# full list see the documentation:
# http://www.sphinx-doc.org/en/master/config

import os
import datetime


# -- Project information -----------------------------------------------------

project = 'Marian NMT'
copyright = '2019, Marian NMT Team'
author = 'Marian NMT Team'

version_file = os.path.join(os.path.dirname(os.path.dirname(__file__)), 'VERSION')
with open(os.path.abspath(version_file)) as f:
    version = f.read().strip()
release = version + ' ' + str(datetime.date.today())


# -- General configuration ---------------------------------------------------

needs_sphinx = '1.3'

extensions = [
    'sphinx.ext.todo',
    'breathe',
    'exhale',
    'recommonmark',
]

templates_path = ['_templates']
source_suffix = ['.rst', '.md']
master_doc = 'index'
exclude_patterns = [
    '_build',
    '_doxygen',
    'README.md',
]

if tags.has('api'):
    exclude_patterns.append('api')

pygments_style = None


# -- Options for HTML output -------------------------------------------------

html_theme = 'sphinx_rtd_theme'
html_static_path = ['_static']

htmlhelp_basename = 'mariandoc'


# -- Extension configuration -------------------------------------------------

breathe_projects = { 'marian': './_doxygen/xml' }
breathe_default_project = 'marian'

doxygen_config = """
INPUT                = ../src
EXCLUDE              = ../src/3rd_party
EXCLUDE_PATTERNS     = *.cpp *.inc *.md
WARN_IF_UNDOCUMENTED = NO
"""

exhale_args = {
    'containmentFolder'     : './api',
    'rootFileName'          : 'library_index.rst',
    'rootFileTitle'         : 'Library API',
    'doxygenStripFromPath'  : '..',
    'createTreeView'        : True,
    'exhaleExecutesDoxygen' : True,
    'exhaleDoxygenStdin'    : doxygen_config.strip(),
}

primary_domain = 'cpp'
highlight_language = 'cpp'

todo_include_todos = True
