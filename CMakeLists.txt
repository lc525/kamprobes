# ========================================
# Global Resourceful build file
# Author: Lucian Carata <lc525@cam.ac.uk>
# ========================================
#
# Run cmake from a separate build directory (out of source build is required).
# It's recommended you use the provided .build script within the build
# directory:
#
# [..build]$ ./.build
#
# If you want to run cmake manually, that is of course also possible
# (but check for what variables need setting before the command - the .build
# script provides the required hints);
#
# Sample build command line:
# [..]$ cd build
# [..build]$ cmake -DCMAKE_BUILD_TYPE=Release ..
# [..build]$ make
#
#
# To look at available optional compilation features, run (from the separate
# build dir):
#
# [..build]$ cmake -LH ..
#
# Highlights from option list, add -D[OPTION_NAME]=ON to cmake cmd to enable:
#   - WITH_DOCS          - build project documentation
#        default:   ON
#        requires:  python -- set SPHINX_PYTHON_VERSIONS to enforce particular
#                             version
#                   sphinx (sphinx-doc.org)
#
#   - WITH_TESTS         - build the unit tests for each project. run them
#                          with "make check" after running make.
#        default:   ON
#        requires:  gtest (bundled with this project)
#        provides:  make target named "check"
#
#   - WITH_XEN_ENABLED   - build with support for gathering xen hypervisor
#                          scheduling data. the time the VM running rscfl
#                          has been scheduled out is integrated into subsystem
#                          measurements
#        default:   ON  (to avoid code rot)
#        requires:  (at runtime) - rscfl running on a Xen VM, with rscfl-enabled
#                                  Xen (private repo)
#
#   - WITH_SHDW_ENABLED  - build with support for shadow kernels. this adds
#                          functionality for creating/destroying shadow kernels
#                          and only setting measurement probes in some of the
#                          shadows.
#        default:   ON (to avoid code rot)
#        recommend: OFF (experimental feature)
#        requires:  (at runtime) - rscfl running on a Xen VM
#
# sample command line:
# [..build]$ cmake -DWITH_DOCS=ON ..
#
cmake_minimum_required(VERSION 3.2)

# Project versioning
#
# variable definitions for generating configuration headers
# please use semantic versioning (semver.org)
# - all releases with the same major version need to be compatible API-wise
# - on publishing a given rscfl version, set PROJECT_TAG_VERSION to something
#   like "beta", "dev", "rel" (or leave it empty); leave it commented out to
#   use the current git revision hash (default)
set(PROJECT_MAJOR_VERSION 0)
set(PROJECT_MINOR_VERSION 1)
set(PROJECT_PATCH_VERSION 0)
set(PROJECT_TAG_VERSION "test")

# data layout version. libraries and kernel modules with different data
# layouts will not be able to communicate. this is not the same as MAJOR_VERSION
# because you can modify the interface in non-backwards compatible ways but
# still retain compatiblity with older rscfl kernel modules.

if(PROJECT_TAG_VERSION STREQUAL "rel")
  set(PVER ${PROJECT_MAJOR_VERSION}.${PROJECT_MINOR_VERSION}.${PROJECT_PATCH_VERSION})
else()
  set(PVER ${PROJECT_MAJOR_VERSION}.${PROJECT_MINOR_VERSION}.${PROJECT_PATCH_VERSION}-${PROJECT_TAG_VERSION})
endif()

message("
                    ╦╔═╔═╗╔╦╗┌─┐┬─┐┌─┐┌┐ ┌─┐┌─┐
                    ╠╩╗╠═╣║║║├─┘├┬┘│ │├┴┐├┤ └─┐
                    ╩ ╩╩ ╩╩ ╩┴  ┴└─└─┘└─┘└─┘└─┘

                  # Build Type:          ${CMAKE_BUILD_TYPE}
                  # Version:             ${PVER}


Starting kamprobes build...
")
if(NOT DEFINED CMAKE_HEADER_SLEEP)
  set(CMAKE_HEADER_SLEEP 0)
endif()
execute_process(COMMAND ${CMAKE_COMMAND} -E sleep ${CMAKE_HEADER_SLEEP} )

set(PNAME kamprobes)
project(${PNAME} C CXX)

# Documentation: look for sphynx installations as part of the following python
# interpreters, giving priority to the versions appearing earlier in the list;
set(SPHINX_PYTHON_VERSIONS 3 2)


###
#
#  CMAKE MODULES
#
###
set(CMAKE_MODULE_PATH ${${PNAME}_SOURCE_DIR}/cmake/modules ${CMAKE_MODULE_PATH})
set(CMAKE_MODULE_PATH ${${PNAME}_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})
include(CMakeDependentOption)
include(FeatureSummary)
# custom modules
include(MacroOutOfSourceBuild)
include(InternalMacros)



###
#
# OPTIONS
#
###

set(CMAKE_C_FLAGS "-Werror")
if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)

string( TOLOWER "${CMAKE_BUILD_TYPE}" bt_lower )
if(bt_lower STREQUAL "debug")
  set(DEFINE_NDEBUG)
else(bt_lower STREQUAL "debug")
  set(DEFINE_NDEBUG "\#define NDEBUG")
endif(bt_lower STREQUAL "debug")


###
#
# BUILD
#
###

# Uninstall target, place it here because others might want to set
# dependencies on it
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/cmake/uninstall.cmake.in"
               "${CMAKE_CURRENT_BINARY_DIR}/uninstall.cmake"
               IMMEDIATE @ONLY)
add_custom_target(uninstall_files
  COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/uninstall.cmake)
add_custom_target(uninstall)

ensure_out_of_source_build("${PNAME} requires an out of source build. ")

configure_project(
  ${PROJECT_MAJOR_VERSION}
  ${PROJECT_MINOR_VERSION}
  ${PROJECT_PATCH_VERSION}
  ${PROJECT_TAG_VERSION}
)

set (PROJECT_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/include)
set (PROJECT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/module)
set (PROJECT_LDRY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/ldry)
set (PROJECT_COMMON_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

# common includes and project settings
include_directories("${PROJECT_INCLUDE_DIR}")
include_directories("${PROJECT_INCLUDE_DIR}/kam/external")

# build kernel modules
include(kam_kbuild)
set(UPDATE_MODULES_UNINSTALL_AFTER uninstall_files)
include(update_modules)

set (kam_OUT_DIR ${PROJECT_BINARY_DIR}/Kbuild)
set (kam_MOD_NAME kamprobes)

set (kam_KINCLUDES
  ${PROJECT_INCLUDE_DIR} # for cost.h and other headers
  ${PROJECT_INCLUDE_DIR}/kam/external
)

set (kam_KSOURCES_static
  ${PROJECT_SOURCE_DIR}/kam.c
  ${PROJECT_SOURCE_DIR}/probes.c
)

set (kam_KSOURCES ${kam_KSOURCES_static})

set (kam_KDEPS # if any of those files change, the stap ko is rebuilt
  ${PROJECT_INCLUDE_DIR}/kam/asm2bin.h
  ${PROJECT_INCLUDE_DIR}/kam/config.h
  ${PROJECT_INCLUDE_DIR}/kam/constants.h
  ${PROJECT_INCLUDE_DIR}/kam/probes.h
  ${PROJECT_INCLUDE_DIR}/kam/kallsyms.h
  ${PROJECT_INCLUDE_DIR}/kam/external/require_kallsyms.h
)

install(DIRECTORY src/include/kam/ DESTINATION include/kam)
install(DIRECTORY src/include/ldry/ DESTINATION include/ldry)

file(MAKE_DIRECTORY ${kam_OUT_DIR})

# remember to pass variables that contain lists of files/directories with ""
kam_kbuild(${kam_MOD_NAME}
  "${kam_KINCLUDES}"
   ${kam_OUT_DIR}
  "${kam_KSOURCES}"
  "${kam_KDEPS}"
)
