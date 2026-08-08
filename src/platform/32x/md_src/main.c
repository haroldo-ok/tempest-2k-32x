/*
 * WIREFIGHT 32X - minimal 68000 main
 *
 * The 32X adapter BIOS has already initialized the console by the
 * time we are entered.  Our jobs:
 *   1) handshake with the two SH2s over the COMM ports:
 *        - wait for the master SH2 boot rom to post "M_OK" on COMM0:2
 *        - wait for the slave  SH2 boot rom to post "S_OK" on COMM4:6
 *        - set FM so the SH2s own the MARS hardware
 *        - clear COMM0:2 (which releases the master SH2's pri_start;
 *          the master then clears COMM4:6 for the slave)
 *   2) poll the two joypads forever and publish them on COMM12/COMM14
 *      so the SH2 side can read them instantly at any time.
 *      (COMM8/10 are off-limits: the 32X boot rom posts the ROM
 *      checksum there and PicoDrive's HLE mirrors that behavior.)
 */

#include <stdint.h>

#define REG_PAD1_DATA   (*(volatile uint8_t *)0xA10003)
#define REG_PAD2_DATA   (*(volatile uint8_t *)0xA10005)
#define REG_PAD1_CTRL   (*(volatile uint8_t *)0xA10009)
#define REG_PAD2_CTRL   (*(volatile uint8_t *)0xA1000B)

#define REG_ADAPTER_CTL (*(volatile uint16_t *)0xA15100)
#define REG_COMM0L      (*(volatile uint32_t *)0xA15120)
#define REG_COMM4L      (*(volatile uint32_t *)0xA15124)
#define REG_COMM8       (*(volatile uint16_t *)0xA15128)
#define REG_COMM10      (*(volatile uint16_t *)0xA1512A)
#define REG_COMM12      (*(volatile uint16_t *)0xA1512C)
#define REG_COMM14      (*(volatile uint16_t *)0xA1512E)

#define SEGA_CTRL_TYPE3 0x0000
#define SEGA_CTRL_TYPE6 0x1000

#define SH2_M_OK 0x4D5F4F4BUL   /* "M_OK" */
#define SH2_S_OK 0x535F4F4BUL   /* "S_OK" */

static uint16_t th_control_phase(volatile uint8_t *pb)
{
    uint16_t val;

    *pb = 0x00;                 /* TH (select) low */
    __asm volatile ("nop");
    __asm volatile ("nop");
    val = *pb;                  /* TH=0 : - 0 s a 0 0 d u */
    *pb = 0x40;                 /* TH (select) high */
    val = (uint16_t)(val << 8);
    val |= *pb;                 /* TH=1 : - 1 c b r l d u */
    return val;
}

static uint16_t read_pad(int port)
{
    volatile uint8_t *pb = (port == 0) ? &REG_PAD1_DATA : &REG_PAD2_DATA;
    uint16_t v1, v2, val;
    uint16_t type;

    v1  = th_control_phase(pb); /* - 0 s a 0 0 d u - 1 c b r l d u */
    (void)th_control_phase(pb);
    v2  = th_control_phase(pb); /* - 0 s a 0 0 0 0 - 1 c b m x y z */
    val = th_control_phase(pb); /* - 0 s a 1 1 x x - 1 c b r l d u */

#ifdef PAD_DEBUG
    *(volatile uint16_t *)0xA15128 = v1;
    *(volatile uint16_t *)0xA1512A = (uint16_t)((v2 << 4) | (val & 0x000F));
#endif

    if (((v2 & 0x0F00) != 0x0000) || ((val & 0x0C00) == 0x0000)) {
        /* three-button pad detection */
        v2 = SEGA_CTRL_TYPE3;
        type = SEGA_CTRL_TYPE3;
    } else {
        v2 = (uint16_t)(SEGA_CTRL_TYPE6 | ((v2 & 0x000F) << 8));
        type = SEGA_CTRL_TYPE6;
    }

    val = (uint16_t)(((v1 & 0x3000) >> 6) | (v1 & 0x003F)); /* s a c b r l d u */
    val = (uint16_t)(val | v2);                             /* 0 0 0 1 m x y z s a c b r l d u */

    /* make active high; a 3-button pad never drives the ZYXM lines,
     * so those bits must stay clear instead of flipping to "pressed" */
    if (type == SEGA_CTRL_TYPE3)
        val = (uint16_t)((val ^ 0x00FF) & 0x00FF);
    else
        val = (uint16_t)((val ^ 0x0FFF) & 0x0FFF);

    return (uint16_t)(type | val);
}

int main(void)
{
    /* init joyports */
    REG_PAD1_CTRL = 0x40;
    REG_PAD2_CTRL = 0x40;
    REG_PAD1_DATA = 0x40;
    REG_PAD2_DATA = 0x40;

    /* wait for both SH2 boot roms to report in */
    while (REG_COMM0L != SH2_M_OK)
        ;
    while (REG_COMM4L != SH2_S_OK)
        ;

    /* give the MARS hardware to the SH2s */
    REG_ADAPTER_CTL |= 0x8000;  /* FM - allow SH2 access to MARS hardware */

    REG_COMM0L = 0;             /* release master SH2 (it frees the slave) */

    for (;;) {
        REG_COMM12 = read_pad(0);
        REG_COMM14 = read_pad(1);
    }

    return 0;
}
