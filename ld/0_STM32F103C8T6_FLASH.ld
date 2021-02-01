/* Specify the memory areas */
MEMORY
{
    RAM(xrw)    : ORIGIN = 0x20000000      , LENGTH = 20k

    /* BOOT(xr)	: ORIGIN = 0x08000000      , LENGTH = 4k */
    PROG(xr)	: ORIGIN = 0x08000000 +  0k, LENGTH = 64k
}
