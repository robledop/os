ENTRY(_start)
OUTPUT_FORMAT(binary)
SECTIONS
{
  . = 0x200000; /* If changing this, you must also change config.h  */
  .text : ALIGN(4096)
  {
    *(.text)
  }

  .asm : ALIGN(4096)
  {
    *(.asm)
  }

  .rodata : ALIGN(4096)

  {
    *(.rodata)
  }

  .data : ALIGN(4096)
  {
    *(.data)
  }

  .bss : ALIGN(4096)
  {
    *(COMMON)
    *(.bss)
  }

}