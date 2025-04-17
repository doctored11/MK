
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "driverlib/debug.h"
#include "driverlib/fpu.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "driverlib/pwm.h"
#include "utils/uartstdio.h"


#define UART_INPUT_BUFFER_LENGTH 32
char uartInputBuffer[UART_INPUT_BUFFER_LENGTH];
uint8_t uartInputLength = 0;

volatile bool printReady = false;
volatile char logBuffer[64];

void SetupPWM()
{
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM1);

    GPIOPinConfigure(GPIO_PF1_M1PWM5);
    GPIOPinConfigure(GPIO_PF2_M1PWM6);
    GPIOPinConfigure(GPIO_PF3_M1PWM7);

    GPIOPinTypePWM(GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3);

    PWMGenConfigure(PWM1_BASE, PWM_GEN_2, PWM_GEN_MODE_DOWN | PWM_GEN_MODE_NO_SYNC);
    PWMGenConfigure(PWM1_BASE, PWM_GEN_3, PWM_GEN_MODE_DOWN | PWM_GEN_MODE_NO_SYNC);

    PWMGenPeriodSet(PWM1_BASE, PWM_GEN_2, 256);
    PWMGenPeriodSet(PWM1_BASE, PWM_GEN_3, 256);

    PWMGenEnable(PWM1_BASE, PWM_GEN_2);
    PWMGenEnable(PWM1_BASE, PWM_GEN_3);
}


void SetRGBColor(uint8_t red, uint8_t green, uint8_t blue)
{
    PWMPulseWidthSet(PWM1_BASE, PWM_OUT_5, red);
    PWMPulseWidthSet(PWM1_BASE, PWM_OUT_6, green);
    PWMPulseWidthSet(PWM1_BASE, PWM_OUT_7, blue);
    PWMOutputState(PWM1_BASE, PWM_OUT_5_BIT | PWM_OUT_6_BIT | PWM_OUT_7_BIT, true);
}


void UARTIntHandler(void)
{
    uint32_t status = MAP_UARTIntStatus(UART0_BASE, true);
    MAP_UARTIntClear(UART0_BASE, status);

    while (MAP_UARTCharsAvail(UART0_BASE))
    {
        char c = UARTCharGetNonBlocking(UART0_BASE);

        if (c == '\r' || c == '\n')
        {
            uartInputBuffer[uartInputLength] = '\0';


            uartInputLength = 0;

            uint32_t r = 0, g = 0, b = 0;
            int parsed = sscanf(uartInputBuffer, "%u-%u-%u", &r, &g, &b);

            if (parsed == 3 && r <= 255 && g <= 255 && b <= 255)
            {
                SetRGBColor((uint8_t)r, (uint8_t)g, (uint8_t)b);
                snprintf((char *)logBuffer, sizeof(logBuffer),
                         "Светодиод: (%u,%u,%u)\r\n", r, g, b);
            }
            else
            {
                snprintf((char *)logBuffer, sizeof(logBuffer),
                         "!Ошибка: используй формат R-B-G (0-255)\r\n");
            }

            printReady = true;
        }
        else if (uartInputLength < UART_INPUT_BUFFER_LENGTH - 1)
        {
            uartInputBuffer[uartInputLength++] = c;
        }
    }
}


void UARTSend(const uint8_t *pui8Buffer, uint32_t ui32Count)
{
    while (ui32Count--)
    {
        MAP_UARTCharPutNonBlocking(UART0_BASE, *pui8Buffer++);
    }
}


int main(void)
{
    MAP_FPUEnable();
    MAP_FPULazyStackingEnable();

    MAP_SysCtlClockSet(SYSCTL_SYSDIV_1 | SYSCTL_USE_OSC | SYSCTL_OSC_MAIN |
                       SYSCTL_XTAL_16MHZ);

    SetupPWM();

    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    // MAP_GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3);

    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    MAP_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    MAP_UARTConfigSetExpClk(UART0_BASE, MAP_SysCtlClockGet(), 115200,
                            (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
                             UART_CONFIG_PAR_NONE));

    UARTStdioConfig(0, 115200, SysCtlClockGet());

    IntMasterEnable();
    IntEnable(INT_UART0);
    UARTIntEnable(UART0_BASE, UART_INT_RX | UART_INT_RT);

    UARTprintf("\033[2J\nRGB UART контроллер запущен.\n");
    UARTprintf("Введите цвет как R-B-G, например: 255-100-0\r\n");

    SetRGBColor(255, 0, 255); 

    while (1)
    {
        if (printReady)
        {
            UARTprintf("%s", logBuffer);
            printReady = false;
        }
    }
}
