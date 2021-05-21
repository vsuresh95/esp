# start sequence of the bootloader
#
#
#include <smp.h>
#define DRAM_BASE 0x80000000

  .section .text.init
  .option norvc
  .globl _prog_start
_prog_start:
  # smp_pause(s1, s2)
  csrr a0, mhartid
  slli a0, a0, 0x10
  li a1, 0x9ff00000
  sub a1, a1, a0
  mv sp, a1
  call main
  smp_resume(s1, s2)
  csrr a0, mhartid
  la a1, _dtb
  li s1, DRAM_BASE
  jr s1

  .section .dtb
  .globl _dtb
  .align 4, 0
_dtb:
  .incbin "riscv.dtb"
