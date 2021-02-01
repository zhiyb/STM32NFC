#ifndef NFC_DEFS_H
#define NFC_DEFS_H

// From NTAG215 specs

#define NFC_REQA		0x26	// 7-bit
#define NFC_WUPA		0x52	// 7-bit
#define NFC_AC_CL1		0x93, 0x20
#define NFC_SEL_CL1		0x93, 0x70
#define NFC_AC_CL2		0x95, 0x20
#define NFC_SEL_CL2		0x95, 0x70
#define NFC_HLTA		0x50, 0x00

#define NFC_NTAG_GET_VERSION	0x60
#define NFC_NTAG_READ		0x30
#define NFC_NTAG_FAST_READ	0x3A
#define NFC_NTAG_WRITE		0xA2
#define NFC_NTAG_COMP_WRITE	0xA0
#define NFC_NTAG_READ_CNT	0x39
#define NFC_NTAG_PWD_AUTH	0x1B
#define NFC_NTAG_READ_SIG	0x3C

#endif // NFC_DEFS_H
