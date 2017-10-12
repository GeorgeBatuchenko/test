#include "lpc32xx_clkpwr.h"
#include "lpc32xx_emc.h"
#include "setup.h"

#define EMCDynamicConfig_2x16Mx16_4b_13r_9c         (0x8D << 7)
#define EMCDynamicRasCas_CAS3                       (3 << 8)    // то же самое что 6 << 7
#define EMCDynamicRasCas_RAS3                       3

volatile UNS_32 * const SDRAM0_MODE_REGISTER = (UNS_32*)(0x80000000 | (0x30 << (9 + 2 + 2)));

void mem_setup()
{
    volatile int delay;
    volatile UNS_32 val;

    // Enable EMC clocking, select SDR memory and select SDRAM slew rate
    CLKPWR->clkpwr_sdramclk_ctrl =
      CLKPWR_SDRCLK_SLOWSLEW_CLK | CLKPWR_SDRCLK_SLOWSLEW |
      CLKPWR_SDRCLK_SLOWSLEW_DAT;

    // After the reset EMC is in the SR mode, clear SR bit and send
    // the command to exit self refresh
    EMC->emcdynamiccontrol = EMC_DYN_DIS_INV_MEMCLK;

    // For writing registers the EMC should be in the idle state
    CLKPWR->clkpwr_pwr_ctrl &= ~CLKPWR_SDRAM_SELF_RFSH;
    CLKPWR->clkpwr_pwr_ctrl &= ~CLKPWR_AUTO_SDRAM_SELF_RFSH;
    CLKPWR->clkpwr_pwr_ctrl |= CLKPWR_UPD_SDRAM_SELF_RFSH;
    CLKPWR->clkpwr_pwr_ctrl &= ~CLKPWR_UPD_SDRAM_SELF_RFSH;
    // Wait for idle state
    while (EMC->emcstatus & EMC_DYN_CTRL_BUSY_BIT);

    // Enable the EMC interface and set EMC endian-ness
    EMC->emccontrol = EMC_DYN_SDRAM_CTRL_EN;
    EMC->emcconfig = 0;

    // Set a long period for the dynamic refresh rate
    EMC->emcdynamicrefresh = 0x7FF;

    // Enable EMC clocking, set the HCLK command delay to 7,
    // select SDR memory and select SDRAM slew rate
    CLKPWR->clkpwr_sdramclk_ctrl = CLKPWR_SDRCLK_HCLK_DLY(7) |
            CLKPWR_SDRCLK_SLOWSLEW_CLK | CLKPWR_SDRCLK_SLOWSLEW |
            CLKPWR_SDRCLK_SLOWSLEW_DAT;

    // Setup address mapping
    EMC->emcdynamicconfig0 = EMC_DYN_DEV_LP_SDR_SDRAM |
            EMCDynamicConfig_2x16Mx16_4b_13r_9c;

    // Setup RAS and CAS latencies
    EMC->emcdynamicrascas0 = EMCDynamicRasCas_CAS3 |
            EMCDynamicRasCas_RAS3;

    // Select the SDRAM command and read strategy
    EMC->emcdynamicreadconfig = EMC_SDR_READCAP_POS_POL |
            EMC_SDR_CLK_NODLY_CMD_DEL;

    // Setup interface timing
    EMC->emcdynamictrfc = 8;    // 66 ns = 9 CLKs = 8 + 1
    EMC->emcdynamictrp = 2;     // 20 ns = 3 CLKs = 2 + 1
    EMC->emcdynamictras = 5;    // 44 ns = 6 CLKs = 5 + 1
    EMC->emcdynamictsrex = 9;   // Looks like the same with tXSR
                                // 75 ns = 10 CLKs = 9 + 1
    EMC->emcdynamictwr = 1;     // 15 ns = 2 CLKs = 1 + 1
    EMC->emcdynamictrc = 8;     // 66 ns = 9 CLKs = 8 + 1
    EMC->emcdynamictxsr = 9;    // 75 ns = 10 CLKs = 9 + 1
    EMC->emcdynamictrrd = 1;    // 15 ns = 2 CLKs = 1 + 1
    EMC->emcdynamictmrd = 1;    // 15 ns = 2 CLKs = 1 + 1
    EMC->emcdynamictcdlr = 0;   // 1 CLKs = 0 + 1

    // Enable SDRAM clocks continously
    EMC->emcdynamiccontrol = EMC_DYN_CLK_ALWAYS_ON |
            EMC_DYN_DIS_INV_MEMCLK;
    // Wait for some time to allow the RAM perform
    // internal initialization with having clock signal
    for (delay = 260; delay > 0; --delay);        // ~ 100 us / 7.69 ns

    // Send NOP command
    EMC->emcdynamiccontrol = EMC_DYN_CLKEN_ALWAYS_ON |
            EMC_DYN_CLK_ALWAYS_ON | EMC_DYN_DIS_INV_MEMCLK |
            EMC_DYN_NOP_MODE;
    for (delay = 260; delay > 0; --delay);        // ~ 100 us / 7.69 ns

    // Issue precharge-all command
    EMC->emcdynamiccontrol = EMC_DYN_CLKEN_ALWAYS_ON |
            EMC_DYN_CLK_ALWAYS_ON | EMC_DYN_DIS_INV_MEMCLK |
            EMC_DYN_PALL_MODE;
    EMC->emcdynamicrefresh = 4;
    for (delay = 0x40; delay > 0; --delay);         // ~ 10 us / 7.69 ns

    // Set normal dynamic refresh timing
    EMC->emcdynamicrefresh = 0x208;     // (64 * 10E-6 * 130 * 10E6) / 16 = 520

    // Program mode register
    EMC->emcdynamiccontrol = EMC_DYN_CLKEN_ALWAYS_ON |
            EMC_DYN_CLK_ALWAYS_ON | EMC_DYN_DIS_INV_MEMCLK |
            EMC_DYN_CMD_MODE;
    val = *SDRAM0_MODE_REGISTER;
    (void)val;
    // Здесь требование подождать 2 клока, но оставим как есть
    for (delay = 3 ; delay > 0; --delay);    // ~ 1 us / 7.5 ns

    // Enter normal operational mode
    EMC->emcdynamiccontrol = EMC_DYN_DIS_INV_MEMCLK |
            EMC_DYN_NORMAL_MODE;
    for (delay = 3 ; delay > 0; --delay);    // ~ 1 us / 7.5 ns

    /* Enable buffers in AHB ports */
    EMC->emcahn_regs[0].emcahbcontrol = EMC_AHB_PORTBUFF_EN;
    EMC->emcahn_regs[3].emcahbcontrol = EMC_AHB_PORTBUFF_EN;
    EMC->emcahn_regs[4].emcahbcontrol = EMC_AHB_PORTBUFF_EN;

    /* Enable port timeouts */
    EMC->emcahn_regs[0].emcahbtimeout = EMC_AHB_SET_TIMEOUT(64);
    EMC->emcahn_regs[3].emcahbtimeout = EMC_AHB_SET_TIMEOUT(64);
    EMC->emcahn_regs[4].emcahbtimeout = EMC_AHB_SET_TIMEOUT(64);

    // Configure access to NOR Flash
    EMC->emcstatic_regs[0].emcstaticconfig  = EMCSTATIC0CONFIG;
    EMC->emcstatic_regs[0].emcstaticwaitwen = EMCSTATIC0WAITWEN_CLKS;
    EMC->emcstatic_regs[0].emcstaticwait0en = EMCSTATIC0WAITOEN_CLKS;
    EMC->emcstatic_regs[0].emcstaticwaitrd  = EMCSTATIC0WAITRD_CLKS;
    EMC->emcstatic_regs[0].emcstaticpage    = EMCSTATIC0WAITPAGE_CLKS;
    EMC->emcstatic_regs[0].emcstaticwr      = EMCSTATIC0WAITWR_CLKS;
    EMC->emcstatic_regs[0].emcstaticturn    = EMCSTATIC0WAITTURN_CLKS;

    // Configure access to FPGA
    // ADC FIFO
    EMC->emcstatic_regs[1].emcstaticconfig  = EMCSTATIC1CONFIG;
    EMC->emcstatic_regs[1].emcstaticwaitwen = EMCSTATIC1WAITWEN_CLKS;
    EMC->emcstatic_regs[1].emcstaticwait0en = EMCSTATIC1WAITOEN_CLKS;
    EMC->emcstatic_regs[1].emcstaticwaitrd  = EMCSTATIC1WAITRD_CLKS;
    EMC->emcstatic_regs[1].emcstaticpage    = EMCSTATIC1WAITPAGE_CLKS;
    EMC->emcstatic_regs[1].emcstaticwr      = EMCSTATIC1WAITWR_CLKS;
    EMC->emcstatic_regs[1].emcstaticturn    = EMCSTATIC1WAITTURN_CLKS;

    // FPGA Control
    EMC->emcstatic_regs[2].emcstaticconfig  = EMCSTATIC2CONFIG;
    EMC->emcstatic_regs[2].emcstaticwaitwen = EMCSTATIC2WAITWEN_CLKS;
    EMC->emcstatic_regs[2].emcstaticwait0en = EMCSTATIC2WAITOEN_CLKS;
    EMC->emcstatic_regs[2].emcstaticwaitrd  = EMCSTATIC2WAITRD_CLKS;
    EMC->emcstatic_regs[2].emcstaticpage    = EMCSTATIC2WAITPAGE_CLKS;
    EMC->emcstatic_regs[2].emcstaticwr      = EMCSTATIC2WAITWR_CLKS;
    EMC->emcstatic_regs[2].emcstaticturn    = EMCSTATIC2WAITTURN_CLKS;

    /* Mirror IRAM at address 0x0 */
    CLKPWR->clkpwr_bootmap = CLKPWR_BOOTMAP_SEL_BIT;
}
