#include "lpc32xx_clkpwr.h"

void clock_setup()
{
    // We don't switch to RTC clock, but it is recommended
    // to write default value into register
    CLKPWR->clkpwr_sysclk_ctrl =
            (CLKPWR->clkpwr_sysclk_ctrl & (~CLKPWR_SYSCTRL_BP_MASK)) |
            CLKPWR_SYSCTRL_BP_TRIG(0x50);

    // Normally, we are in DIRECT mode, and before
    // switching to RUN mode with maximum performance
    // configure dividers to be sure it leaves in
    // allowed range after CLOCK multiplying
    CLKPWR->clkpwr_hclk_div =
            CLKPWR_HCLKDIV_DDRCLK_STOP |
            CLKPWR_HCLKDIV_PCLK_DIV(19) |       // ARM CLK / 20 (13MHz)
            CLKPWR_HCLKDIV_DIV_2POW(1);         // ARM_CLK / 2 (130MHz)

    // Configure HCLK PLL on 266 MHz
    CLKPWR->clkpwr_hclkpll_ctrl =
            CLKPWR_HCLKPLL_POWER_UP |
            CLKPWR_HCLKPLL_POSTDIV_BYPASS |
            CLKPWR_HCLKPLL_FDBK_SEL_FCLK |      // Use PLL output CLK for feedback
                                                //  - non-integer mode
            CLKPWR_HCLKPLL_POSTDIV_2POW(0) |
            CLKPWR_HCLKPLL_PREDIV_PLUS1(0) |
            CLKPWR_HCLKPLL_PLLM(19);            // Multiply by 20 (13MHz * 20 = 260)
    // Wait for clock stabilizing
    while (!(CLKPWR->clkpwr_hclkpll_ctrl & CLKPWR_HCLKPLL_PLL_STS));
    // Now the HCLKPLL clock output could be used as clock source
    CLKPWR->clkpwr_pwr_ctrl &= ~CLKPWR_STOP_MODE_CTRL;
    CLKPWR->clkpwr_pwr_ctrl &= ~CLKPWR_CTRL_FORCE_PCLK;
    CLKPWR->clkpwr_pwr_ctrl |= CLKPWR_SELECT_RUN_MODE;
}
