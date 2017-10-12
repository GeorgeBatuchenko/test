#include "setup.h"
#include "lpc32xx_uart.h"

void clock_setup();
void mem_setup();

void board_hw_init(void)
{
    /* Place HSUARTs into loopback mode to help protect against RX
       lockup */
    UARTCNTL->loop |= UART_LPBACK_ENABLED(1);
    UARTCNTL->loop |= UART_LPBACK_ENABLED(2);
    UARTCNTL->loop |= UART_LPBACK_ENABLED(7);

    /* Setup system clocks and run mode */
    clock_setup();

    /* Setup memory */
    mem_setup();
}
