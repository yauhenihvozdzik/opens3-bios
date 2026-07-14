/* tools/payload.ld — Linker script for OpenS3 payloads (Xtensa ESP32-S3) */
OUTPUT_ARCH(xtensa)
ENTRY(payload_main)

SECTIONS
{
    . = 0x0;

    .text : ALIGN(4)
    {
        *(.text.entry)
        *(.text)
        *(.text.*)
        *(.rodata)
        *(.rodata.*)
    }

    .data : ALIGN(4)
    {
        *(.data)
        *(.data.*)
    }

    .bss : ALIGN(4)
    {
        *(.bss)
        *(.bss.*)
        *(COMMON)
    }

    /DISCARD/ :
    {
        *(.comment)
        *(.eh_frame)
        *(.debug_*)
    }
}