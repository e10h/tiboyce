SECTIONS {
 . = 0xD1E881;
 .text : { KEEP(*(.text.loader)) *(.text .text.*) }
 .rodata : { *(.rodata .rodata.*) }
 .data : { *(.data .data.*) }
 .bss : { *(.bss .bss.* COMMON) }
 _module_end = .;
 /DISCARD/ : { *(.comment) *(.note*) }
}
