#include "uart_printf.h"
#include "stm32f4xx_hal.h"
#include <stdarg.h>
#include <stdio.h>

#if 0
extern UART_HandleTypeDef huart4;

// FIXME - temporary hack for SC4-HSM
HAL_StatusTypeDef HAL_UART_Init(UART_HandleTypeDef *huart) {}

void uart_printf(const char *fmt, ...) {
    char buffer[512];
    int ret;
    va_list args;
    va_start(args, fmt);
    ret = vsnprintf(buffer, sizeof(buffer)-1, fmt, args);
    va_end(args);

    // FIXME - temporary hack for SC4-HSM
    buffer[ret] = 0;
    printf(buffer);
    return;

    if (ret > 0) {
        if(buffer[ret-1] == '\n')
            buffer[ret++] = '\r';
        //HAL_UART_Transmit(&huart4, (uint8_t*)buffer, ret, 0xFFFF);
    }
}
#endif
