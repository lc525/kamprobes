# kam kernel module build file
# author: Lucian Carata <lc525@cam.ac.uk>

function(JOIN VALUES GLUE OUTPUT)
  string (REPLACE ";" "${GLUE}" _TMP_STR "${VALUES}")
  set (${OUTPUT} "${_TMP_STR}" PARENT_SCOPE)
endfunction()

#  ====================================================================
#
# kam_KBUILD (public function)
#   MOD_NAME = the name of resulting kernel module
#   INCLUDES = include files to add to kbuild make
#   OUT_DIR = the output directory for stap results
#   GEN_SRC = SystemTap .stp script
#   SRC = Other sources that need to be built for this module
#
#   Unnamed parameters:
#   ARGN = file dependencies (if one of those is modified the module
#          gets rebuilt)
#
#  ====================================================================
function(KAM_KBUILD MOD_NAME INCLUDES OUT_DIR SRC DEPS)
  # Generate includes list
  foreach(FIL ${INCLUDES})
    get_filename_component(ABS_FIL ${FIL} ABSOLUTE)
    list(FIND _kam_include_path -I${ABS_FIL} _contains_already)
    if(${_contains_already} EQUAL -1)
      list(APPEND _kam_include_path -I${ABS_FIL})
    endif()
  endforeach()
  #list(APPEND _kam_include_path -I$ENV{KAM_LINUX_HEADERS})
  list(APPEND _kam_include_path -I/home/lc525/dev/linux-stable-rc/include/linux)
  #list(APPEND _kam_include_path -I$ENV{KAM_LINUX_HEADERS}/arch/x86/include/generated)
  #list(APPEND _kam_include_path -I$ENV{KAM_LINUX_HEADERS}/arch/x86/include/generated/uapi)
  JOIN("${_kam_include_path}" " " _kam_includes)
  set(KAM_MOD_INCLUDES ${_kam_includes})

  if(NOT DEFINED DEFINE_NDEBUG)
    set(K_DBG "-g")
  endif()

  # Generate object file list
  foreach(SRC_FIL ${SRC})
    get_filename_component(ABS_PATH ${SRC_FIL} ABSOLUTE)
    get_filename_component(SRCF_DIR ${ABS_PATH} DIRECTORY)
    get_filename_component(FNAME ${ABS_PATH} NAME)
    get_filename_component(FNAME_NAME ${ABS_PATH} NAME_WE)

    file(RELATIVE_PATH _add_file ${OUT_DIR} ${SRCF_DIR}/${FNAME})
    get_filename_component(S_REL ${_add_file} DIRECTORY)

    list(FIND _kam_objs ${S_REL}/${FNAME_NAME}.o _contains_already)
    if(${_contains_already} EQUAL -1)
      list(APPEND _kam_objs ${S_REL}/${FNAME_NAME}.o)
      list(APPEND _kam_objsfp ${OUT_DIR}/${FNAME_NAME}.o)
      list(APPEND _kam_srcf ${SRC_FIL})
    endif()
  endforeach()
  JOIN("${_kam_objs}" " " KAM_O_FILES)

  # Generate kernel module Makefile
  configure_file(
    "${PROJECT_SOURCE_DIR}/Makefile.in"
    "${OUT_DIR}/Makefile"
    @ONLY)

  # Build kernel module
  set( MODULE_TARGET_NAME kamprobes )
  set( MODULE_BIN_FILE    ${OUT_DIR}/${MOD_NAME}.ko )
  set( MODULE_OUTPUT_FILES    ${_kam_objsfp} )
  set( MODULE_SOURCE_DIR  ${OUT_DIR} )
  set( KERNEL_DIR "/lib/modules/${CMAKE_SYSTEM_VERSION}/build/" )
  set( KBUILD_CMD $(MAKE)
                  -C ${KERNEL_DIR}
                  M=${MODULE_SOURCE_DIR}
                  modules )

  add_custom_command( OUTPUT  ${MODULE_BIN_FILE}
                              ${MODULE_OUTPUT_FILES}
                      COMMAND ${KBUILD_CMD}
                      COMMAND cp -f ${MODULE_BIN_FILE} ${PROJECT_BINARY_DIR}
                      COMMAND find ../src -name "*.o" -exec mv {} ${OUT_DIR} \;
                      COMMAND find ../src -name "*.cmd" -exec mv {} ${OUT_DIR} \;
                      DEPENDS ${_kam_srcf} ${DEPS}
                      COMMENT "Running kbuild for ${MODULE_BIN_FILE}"
                      VERBATIM )

  add_custom_target ( ${MODULE_TARGET_NAME} ALL
                      DEPENDS ${MODULE_BIN_FILE})

  # Install .ko
  install(FILES ${PROJECT_BINARY_DIR}/${MOD_NAME}.ko
    DESTINATION /lib/modules/${CMAKE_SYSTEM_VERSION})

endfunction()
