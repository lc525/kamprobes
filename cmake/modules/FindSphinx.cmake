# check whether version constraints were expressed for the sphinx binaries
# i.e choose between sphinx-build and sphinx-build2
if(NOT DEFINED SPHINX_PYTHON_VERSIONS)
  set(SPHINX_PYTHON_VERSIONS 3 2)
endif()

set(BIN_NAMES)
foreach(ver ${SPHINX_PYTHON_VERSIONS})
  if(ver EQUAL 3)
    set(ver "")
  endif()
  list(APPEND BIN_NAMES "sphinx-build${ver}")
endforeach(ver)

find_program(SPHINX_EXECUTABLE
    NAMES ${BIN_NAMES}
    HINTS
    $ENV{SPHINX_DIR}
    PATH_SUFFIXES bin
    DOC "Sphinx documentation generator"
)
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Sphinx DEFAULT_MSG SPHINX_EXECUTABLE)

mark_as_advanced(
  SPHINX_EXECUTABLE
)

set_package_properties(Sphinx PROPERTIES
  URL "http://sphinx-doc.org/"
  TYPE OPTIONAL
  PURPOSE "Generate html manuals and design docs")
