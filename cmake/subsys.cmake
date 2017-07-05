# build module for generating kernel subsystems header file
# uses the scripts/find_subsystems.py
# author: Lucian Carata <lc525@cam.ac.uk>

#  ====================================================================
#
# SUBSYS_HEADER_GEN (public function)
#   L_ROOT = directory containing the linux source tree
#   L_VMLINUX = path towards a vmlinux that was built with CONFIG_DEBUG_INFO=y
#   L_BUILD = directory where vmlinux was originally built
#   OUT_LIST = name of header file to generate with list of subsystems
#   OUT_ADDR = name of header file to generate with kernel function
#              addresses for the given kernel binary, per subsystem
#   OUT_JSON = name of json file to generate with list of subsystems
#
#  ====================================================================
function(SUBSYS_HEADER_GEN L_ROOT L_VMLINUX L_BUILD OUT_LIST OUT_DEFAULT OUT_CUSTOM OUT_ADDR OUT_ADDR_C OUT_JSON)
  execute_process(
    COMMAND uname -r
    OUTPUT_VARIABLE KERNEL_RELEASE
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  execute_process(
    COMMAND strings ${L_VMLINUX}
    COMMAND grep "Linux version"
    COMMAND awk "{print $3}"
    OUTPUT_VARIABLE VMLINUX_RELEASE
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  get_filename_component(OUT_JSON_DIR ${OUT_JSON} DIRECTORY)

  if(KERNEL_RELEASE STREQUAL VMLINUX_RELEASE)
    set(K_DEP_FILE ${OUT_JSON_DIR}/.subsys-for-${KERNEL_RELEASE})
  else()
    string(REGEX MATCH "^([0-9]+)\\.([0-9]+)\\.([0-9]+).*$" K_VER ${KERNEL_RELEASE})
    set(K_MAJOR ${CMAKE_MATCH_1})
    set(K_MINOR ${CMAKE_MATCH_2})
    set(K_PATCH ${CMAKE_MATCH_3})

    string(REGEX MATCH "^([0-9]+)\\.([0-9]+)\\.([0-9]+).*$" K_VER ${VMLINUX_RELEASE})
    set(VML_MAJOR ${CMAKE_MATCH_1})
    set(VML_MINOR ${CMAKE_MATCH_2})
    set(VML_PATCH ${CMAKE_MATCH_3})

    if(K_MAJOR STREQUAL VML_MAJOR AND K_MINOR STREQUAL VML_MINOR AND K_PATCH STREQUAL VML_PATCH)
      message("!! WARNING: minor kernel version difference between:\n   \trscfl-analysed kernel: ${VMLINUX_RELEASE} (${L_VMLINUX}) and\n   \tthe running kernel:    ${KERNEL_RELEASE}.\n   Rscfl kernel probing should still work.")
      set(K_DEP_FILE ${OUT_JSON_DIR}/.subsys-for-${KERNEL_RELEASE})
    else()
      message("\n   ***********\n!! WARNING: Kernel image used for computing probe addresses (${L_VMLINUX}) has a different version (${VMLINUX_RELEASE}) than the running kernel (${KERNEL_RELEASE}).\n   This means that some probes might be placed incorrectly, continue at your own risk!\n   ***********\n")
    endif()
  endif()

  if(${ARGC} GREATER 8)
    set(OPT_SUB_ARG ${ARGN})
  endif(${ARGC} GREATER 8)

  # Generate subsystems header files
  set(CMD_ENV_VARS "RSCFL_LINUX_ROOT=${L_ROOT};RSCFL_LINUX_BUILD=${L_BUILD};RSCFL_LINUX_VMLINUX=${L_VMLINUX};")
  add_custom_command(
    OUTPUT ${OUT_JSON} ${OUT_LIST} ${OUT_ADDR} ${OUT_DEFAULT} ${OUT_CUSTOM} ${OUT_ADDR_C} ${K_DEP_FILE}
    COMMAND ${CMAKE_COMMAND} -E env ${CMD_ENV_VARS} ${CMAKE_CURRENT_SOURCE_DIR}/scripts/blacklist.sh ${CMAKE_CURRENT_SOURCE_DIR}/scripts/blacklist.fn
    COMMAND ${CMAKE_COMMAND} -E env ${CMD_ENV_VARS} ${CMAKE_CURRENT_SOURCE_DIR}/scripts/find_subsystems.py
     ARGS
      -l ${L_ROOT}
      -v ${L_VMLINUX}
      --build_dir ${L_BUILD}
      --find_subsystems
      -J ${OUT_JSON}
      --update_json
      --gen_shared_header ${OUT_LIST}
      --gen_default_subsys ${OUT_DEFAULT}
      --gen_custom_subsys ${OUT_CUSTOM}
      --fn_blacklist ${CMAKE_CURRENT_SOURCE_DIR}/scripts/blacklist.fn
      --defaults_file ${PROJECT_COMMON_DIR}/default_subsys.json
      --hooks_file ${PROJECT_COMMON_DIR}/custom_subsys.json
      --addr_out_h ${OUT_ADDR}
      --addr_out_c ${OUT_ADDR_C}
      --stderr ${PROJECT_BINARY_DIR}/subsys-err.log
      ${OPT_SUB_ARG}
    COMMAND rm -f .subsys-for-* && touch ARGS ${K_DEP_FILE}
    COMMENT "Building probes address list from kernel binary, generating header files"
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/scripts/find_subsystems.py
  )
  add_custom_target(subsys_gen DEPENDS
    ${OUT_JSON}
    ${OUT_LIST}
    ${OUT_DEFAULT}
    ${OUT_CUSTOM}
    ${OUT_ADDR}
    ${OUT_ADDR_C}
    ${K_DEP_FILE}
    )
endfunction()
