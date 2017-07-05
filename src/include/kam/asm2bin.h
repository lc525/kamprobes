#ifndef _KAM_ASM2BIN_H_
#define _KAM_ASM2BIN_H_

inline char neg_c2(uint8_t val){
  return (~val)+1;
}

inline void emit_rel_address(char **wrapper_end, char *addr)
{
  int32_t *w_end = (int32_t *)*wrapper_end;
  *w_end = (int32_t)(addr - 4 - *wrapper_end);
  (*wrapper_end) += 4;
}

inline void emit_abs_address(char **wrapper_end, char *addr)
{
  int32_t *w_end = (int32_t *)*wrapper_end;
  // Make a 32 bit pointer.
  *w_end = (int32_t)(addr - (char *)0xffffffff00000000);
  (*wrapper_end) += 4;
}

inline void emit_insn(char **wrapper_end, char c)
{
  **wrapper_end = c;
  (*wrapper_end)++;
}

inline void emit_multiple_insn(char **wrapper_end, const char *c, int num_insn)
{
  memcpy(*wrapper_end, c, num_insn);
  (*wrapper_end) += num_insn;
}

inline void emit_jump(char **wrapper_end, char *addr)
{
  **wrapper_end = 0xe9;
  (*wrapper_end)++;
  emit_rel_address(wrapper_end, addr);
}

inline void emit_callq(char **wrapper_end, char *addr)
{
  **wrapper_end = 0xe8;
  (*wrapper_end)++;
  emit_rel_address(wrapper_end, addr);
}

inline void emit_return_address_to_r11(char **wrapper_end, char *wrapper_fp)
{
  // mov r11, [rip-addr]
  const char machine_code[] = {0x4c, 0x8b, 0x1d};
  emit_multiple_insn(wrapper_end, machine_code, sizeof(machine_code));
  emit_rel_address(wrapper_end, wrapper_fp - 8);
}

inline void emit_mov_rsp_r11(char **wrapper_end)
{
  // mov (%rsp), %r11
  const char machine_code[] = {0x4c, 0x8b, 0x1c, 0x24};
  emit_multiple_insn(wrapper_end, machine_code, sizeof(machine_code));
}

inline void emit_mov_r11_addr(char **wrapper_end, char *addr)
{
  // mov %r11, addr
  const char machine_code[] = {0x4c, 0x89, 0x1d};
  emit_multiple_insn(wrapper_end, machine_code, sizeof(machine_code));
  emit_rel_address(wrapper_end, addr);
}

inline void emit_push_r11(char **wrapper_end)
{
    emit_insn(wrapper_end, 0x41);
    emit_insn(wrapper_end, 0x53);
}

inline void emit_mov_addr_rsp(char **wrapper_end, char *addr, const char disp)
{
  // mov $addr disp(%rsp)
  char machine_code[] = {0x48, 0xc7, 0x44, 0x24, 0x00};
  if ( disp == 0 ) {
    machine_code[2] = 0x04;
    emit_multiple_insn(wrapper_end, machine_code, 4);
  } else {
    machine_code[4] = disp;
    emit_multiple_insn(wrapper_end, machine_code, 5);
  }
  emit_abs_address(wrapper_end, addr);
}

inline void emit_mov_r11_rsp(char **wrapper_end, const char disp)
{
  // mov %r11 disp(%rsp)
  char machine_code[] = {0x4c, 0x89, 0x5c, 0x24, 0x00};
  if ( disp == 0 ) {
    machine_code[2] = 0x1c;
    emit_multiple_insn(wrapper_end, machine_code, 4);
  } else {
    machine_code[4] = disp;
    emit_multiple_insn(wrapper_end, machine_code, 5);
  }
}

inline void emit_mov_int_rsp(char **wrapper_end, uint32_t val, const char disp)
{
  int mask, j;
  // [AT&T]  movl $val disp(%rsp)
  // [Intel] mov disp[rsp], dword ptr val
  char machine_code[8] = {0xc7, 0x44, 0x24, 0x00, 0x00, 0x00, 0x00, 0x00};

  machine_code[3] = disp;
  mask = (1<<8) - 1;
  for (j = 0; j < 4; j++){
    machine_code[4+j] = val & mask;
    val = val >> 8;
  }
  emit_multiple_insn(wrapper_end, machine_code, sizeof(machine_code));
}

inline void emit_push_addr(char **wrapper_end, char *addr)
{
  // push ($addr)
  emit_insn(wrapper_end, 0x68);
  emit_abs_address(wrapper_end, addr);
}

inline void emit_short_cond_jmp(char **wrapper_end, const char *cond,
                                size_t cond_size, char jmp_size){
  int i;
  for (i = 0; i < cond_size; i++) {
    emit_insn(wrapper_end, cond[i]);
  }

  // jnz jmp_size
  emit_insn(wrapper_end, 0x75);
  emit_insn(wrapper_end, jmp_size);
}

inline int is_call_insn(u8 *addr)
{
  return *addr == 0xe8;
}

inline int is_noop(u8 *addr)
{
  return (*addr == 0x90 || *addr == 0x0f || *addr == 0x1f || *addr == 0x66);
}

#endif
