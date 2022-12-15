# Configuration file for the Sphinx documentation builder.
#
# This file only contains a selection of the most common options. For a full
# list see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Path setup --------------------------------------------------------------

# If extensions (or modules to document with autodoc) are in another directory,
# add these directories to sys.path here. If the directory is relative to the
# documentation root, use os.path.abspath to make it absolute, like shown here.
#
import os
import sys
import os.path
import json
import datetime

from sphinx.builders.html import StandaloneHTMLBuilder

# -- Project information -----------------------------------------------------

# TODO
# WHAT TO DO ABOUT COPYRIGHTS - this is for all the docs! not the cof.py file itself!
#
current_year = datetime.datetime.now().year
copyright = '2019-{}, XMOS Ltd'.format(current_year)
author = 'XMOS Ltd'

# ----------------------------------------------
# conf files path
conf_files_dir = 'doc'

# Set DEV_MODE from env var
DEV_MODE = os.getenv('DEV', False)
if DEV_MODE:
  todo_include_todos = True

# Get repo structure from env vars
repo = "."
doc_dir = "."
doxy_dir = "doc/_build/_doxygen"
template_dir = "doc/_templates"


settingsFile = repo + '/settings.json'
data = {}
if os.path.isfile(settingsFile):
  with open(settingsFile) as infile:
    data = json.load(infile)
print(data)
title = data.get('title', 'XMath Tutorial')
project = data.get('project', 'TMP')
release = data.get('version', 'TBD')

# set derivative variables for sphinx to use
if release == 'TBD':
  version = 'TBD'
else:
  version = release.rpartition('.')[0]

# set rst substitutions
# version/release done automatically
rst_prolog = f"""
.. |project| replace:: {project}
.. |title| replace:: {title}
"""

# ----------------------------------------------
StandaloneHTMLBuilder.supported_image_types = [
    'image/svg+xml',
    'image/gif',
    'image/png',
    'image/jpeg',
]

# -- General configuration ---------------------------------------------------

# Add any Sphinx extension module names here, as strings. They can be
# extensions coming with Sphinx (named 'sphinx.ext.*') or your custom
# ones.
extensions = [
    'sphinx.ext.todo',
    'sphinx_copybutton',
    'sphinx_inline_tabs',
    'breathe',
    'sphinx.ext.autosectionlabel',
    # 'sphinxcontrib.wavedrom',
    'myst_parser',
    # 'sphinx_math_dollar',
    'sphinx.ext.mathjax'
]

myst_enable_extensions = ["dollarmath"]

# mathjax_config = {
#     'tex2jax': {
#         'inlineMath': [["\\(", "\\)"]],
#         'displayMath': [["\\[", "\\]"]],
#     },
# }

# mathjax3_config = {
#     "tex": {
#         "inlineMath": [['\\(', '\\)']],
#         "displayMath": [["\\[", "\\]"]],
#     }
# }


# Breathe Configuration
breathe_projects = {title: doxy_dir + '/xml/'}
breathe_default_project = title

# Add any paths that contain templates here, relative to this directory.
templates_path = ['_templates']

# List of patterns, relative to source directory, that match files and
# directories to ignore when looking for source files.
# This pattern also affects html_static_path and html_extra_path.
exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store', '.venv',
                    'README.rst', '**/README.rst',
                    'CHANGELOG.rst', '**/CHANGELOG.rst',
                    'LICENSE.rst', '**/LICENSE.rst',
                    'CONTRIBUTIONS.rst', '**/CONTRIBUTIONS.rst',
                    'COPYRIGHTS.rst', '**/COPYRIGHTS.rst']


# Append any exclude patterns specified by caller
#   User specified exclude patterns are handy when a repo
#   contains submodules that contain .rst files one wants
#   to ignore.
#exclude_patterns_fn = os.getenv('EXCLUDE_PATTERNS')
# if exclude_patterns_fn:
#    with open(exclude_patterns_fn) as exclude_patterns_file:
#        for line in exclude_patterns_file:
#            if not line.startswith('#'):
#                exclude_patterns.append(line.strip())

# Wavedrom config
# https://pypi.org/project/sphinxcontrib-wavedrom/
render_using_wavedrompy = True  # use python implementation so no npm/node
wavedrom_html_jsinline = False  # always generate images even for html

# -- Options for HTML output -------------------------------------------------

# The theme to use for HTML and HTML Help pages.  See the documentation for
# a list of builtin themes.
#
html_theme = 'furo'

# Override the default format that adds the word "documentation" to the project and release
html_title = '{} {}'.format(title, release)

# Add the last updated on
html_last_updated_fmt = '%b %d, %Y'

# Don't copy the rst sources into the output folder
html_copy_source = False

# Add any paths that contain custom static files (such as style sheets) here,
# relative to this directory. They are copied after the builtin static files,
# so a file named "default.css" will overwrite the builtin "default.css".
#html_static_path = ['doc/src_html', conf_files_dir]

html_css_files = ['custom.css']

html_logo = conf_files_dir + '/xmos_logo.png'

xmos_grey_dark = '#464749'
xmos_grey_light = '#D1D3D4'
xmos_blue_dark = '#1399DB'
xmos_blue_light = '#44BBF1'
xmos_green_dark = '#99DA07'
xmos_green_light = '#B8E720'

html_theme_options = {
    'sidebar_hide_name': False,
    'light_css_variables': {
        'color-brand-primary': xmos_blue_dark,
        'color-brand-content': xmos_blue_dark,
    },
    'dark_css_variables': {
        'color-brand-primary': xmos_blue_dark,
        'color-brand-content': xmos_blue_dark,
    }
}


# -- Options for latex output ------------------------------------------------

# Include xmos specific style files
# latex_additional_files = [conf_files_dir + '/xcore.cls']

latex_logo = conf_files_dir + '/xmos_logo.png'

# https://www.sphinx-doc.org/en/master/latex.html
latex_elements = {
    "papersize": "a4paper",
    "pointsize": "10pt",
    # "\\usepackage{tgtermes}\n\\usepackage{tgheros}\n\\renewcommand\ttdefault{txtt}",
    "fontpkg": "\\usepackage{times}",
    "fncychap": "\\usepackage[Bjarne]{fncychap}",
    "preamble": r"""
% Force page break on new section
\let\oldsection\section
\renewcommand\section{\clearpage\oldsection}
""",  # "\\DeclareUnicodeCharacter{00A0}{\\nobreakspace}",  # "",
    "atendofbody": "",
    "figure_align": "htbp",
    # openany/oneside reduce blank pages
    "extraclassoptions": "openany,oneside",  # "oneside,onecolumn,article,10pt",
    # "maxlistdepth": ,
    "inputenc": "\\usepackage[utf8]{inputenc}",
    "cmappkg": "\\usepackage{cmap}",
    # "fontenc": "\\usepackage[T1]{fontenc}",
    "maketitle": "\\sphinxmaketitle",  # \\maketitle
    "releasename": "",  # Release
    "tableofcontents": "\\sphinxtableofcontents",  # \\tableofcontents
    "transition": "\n\n\\bigskip\\hrule\\bigskip\n\n",
    "makeindex": "\\makeindex",
    "printindex": "\\printindex",
    "extrapackages": "\\usepackage{longtable}",
    "sphinxsetup": "",
}


# -- Options for sphinx_copybutton -------------------------------------------

copybutton_prompt_text = r"\$ |\(gdb\) "
copybutton_prompt_is_regexp = True

# My setup for figures
numfig = True
numfig_secnum_depth = 1


autosectionlabel_prefix_document = True

html_sidebars = {'**': [
    'sidebar/brand.html',
    'sidebar/search.html',
    'sidebar/navigation.html',
]}
