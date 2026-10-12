OUTPUT_FORMAT(elf32-s390)
OUTPUT_ARCH(s390:31-bit)
ENTRY(cms_xl_main)
SECTIONS
{
  . = 0;
  .text : {  *(.text .text.*) }
  .rodata : ALIGN(8) { *(.rodata .rodata.*) }
  .init_array : ALIGN(8) { __init_array_start = .; KEEP(*(SORT(.init_array.*))) KEEP(*(.init_array .ctors)) __init_array_end = .; }
  .data : ALIGN(8) { *(.data .data.*) }
  .bss : ALIGN(8) { *(.bss .bss.*) *(COMMON) }
  /DISCARD/ : { *(.note.GNU-stack) *(.comment) *(.eh_frame) }
}
