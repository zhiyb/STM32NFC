/* Entry Point */
ENTRY(Reset_Handler)

/* Highest address of the user mode stack */
__stack_end__ = 0x20000000 + 20 * 1024;     /* end of DATA */
__heap_size__ = 4 * 1024;                   /* Heap size */

/* Flash disk regions */
__prog_start__      = ORIGIN(PROG);
__prog_end__        = ORIGIN(PROG) + LENGTH(PROG);

/* DATA regions */

/* Define output sections */
SECTIONS
{
  /* The startup code goes first into TEXT */
  .isr_vector :
  {
    . = ALIGN(4);
    KEEP(*(.isr_vector)) /* Startup code */
    . = ALIGN(4);
  } >TEXT AT >LOAD

  /* The program code and other data goes into TEXT */
  .text :
  {
    . = ALIGN(4);
    *(.text)           /* .text sections (code) */
    *(.text*)          /* .text* sections (code) */
    *(.glue_7)         /* glue arm to thumb code */
    *(.glue_7t)        /* glue thumb to arm code */
    *(.eh_frame)

    KEEP(*(.init))
    KEEP(*(.fini))

    . = ALIGN(4);
    _etext = .;        /* define a global symbols at end of code */
  } >TEXT AT >LOAD

  /* Constant data goes into TEXT */
  .rodata :
  {
    . = ALIGN(4);
    *(.rodata)         /* .rodata sections (constants, strings, etc.) */
    *(.rodata*)        /* .rodata* sections (constants, strings, etc.) */

    . = ALIGN(4);
  } >TEXT AT >LOAD

  .ARM.extab   : { *(.ARM.extab* .gnu.linkonce.armextab.*) } >TEXT
  .ARM : {
    __exidx_start = .;
    *(.ARM.exidx*)
    __exidx_end = .;
  } >TEXT AT >LOAD

  .preinit_array     :
  {
    PROVIDE_HIDDEN (__preinit_array_start = .);
    KEEP(*(.preinit_array*))
    PROVIDE_HIDDEN (__preinit_array_end = .);
  } >TEXT AT >LOAD
  .init_array :
  {
    PROVIDE_HIDDEN (__init_array_start = .);
    KEEP(*(SORT(.init_array.*)))
    KEEP(*(.init_array*))
    PROVIDE_HIDDEN (__init_array_end = .);
  } >TEXT AT >LOAD
  .fini_array :
  {
    PROVIDE_HIDDEN (__fini_array_start = .);
    KEEP(*(SORT(.fini_array.*)))
    KEEP(*(.fini_array*))
    PROVIDE_HIDDEN (__fini_array_end = .);
  } >TEXT AT >LOAD

  /* Initialized data sections goes into DATA, load LMA copy after code */
  . = ALIGN(4);
  /* used by the startup to initialize data */
  __data_load__ = LOADADDR(.data);

  .data :
  {
    __data_start__ = .;
    . = ALIGN(4);
    *(.data)           /* .data sections */
    *(.data*)          /* .data* sections */
    /* Special program code inside DATA */
    *(.ram)
    *(.ram*)

    . = ALIGN(4);
    __data_end__ = .;
  } >DATA AT >LOAD

  /* Uninitialized data section */
  . = ALIGN(4);
  .bss :
  {
    /* This is used by the startup in order to initialize the .bss secion */
    __bss_start__ = .;
    *(.bss)
    *(.bss*)
    *(COMMON)

    . = ALIGN(4);
    __bss_end__ = .;
  } >DATA

  /* Heap section */
  . = ALIGN(4);
  .heap :
  {
    . = ALIGN(4);
    __heap_start__ = .;
    /* Reserve space */
    . += __heap_size__;
    . = ALIGN(4);
    __heap_end__ = .;
  } >DATA

  /* Remove information from the standard libraries */
  /DISCARD/ :
  {
    libc.a ( * )
    libm.a ( * )
    libgcc.a ( * )
    *(.olist.*)
  }

  .ARM.attributes 0 : { *(.ARM.attributes) }
}
