//lr3 цвета по кругу

#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"

#include "inc/tm4c123gh6pm.h"
#include "driverlib/interrupt.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"

uint8_t ui8PinData = 2;
bool direction = true;

uint8_t GetNextMask(uint8_t currentMask, bool direction)
{
    if (direction)
    {
        if (currentMask == 8)
        {
            return 2;
        }
        return currentMask * 2;
    }
    else
    {
        if (currentMask == 2)
        {
            return 8;
        }
        return currentMask / 2;
    }
}

void PortFIntHandler(void) // прерыванеи для кнопки
{
    GPIOIntClear(GPIO_PORTF_BASE, GPIO_INT_PIN_4);

    direction = !direction;

    SysCtlDelay(2000000);
}

int main(void)
{
    SysCtlClockSet(SYSCTL_SYSDIV_5 | SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ | SYSCTL_OSC_MAIN);

    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3);

    GPIOPinTypeGPIOInput(GPIO_PORTF_BASE, GPIO_PIN_4);
    GPIOPadConfigSet(GPIO_PORTF_BASE, GPIO_PIN_4,
                     GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);

    // прерыванеи кнопки на отрицательном фронте
    GPIOIntRegister(GPIO_PORTF_BASE, PortFIntHandler);
    GPIOIntTypeSet(GPIO_PORTF_BASE, GPIO_PIN_4, GPIO_FALLING_EDGE);
    GPIOIntEnable(GPIO_PORTF_BASE, GPIO_INT_PIN_4);
    IntEnable(INT_GPIOF);

    while (1)
    {

        ui8PinData = GetNextMask(ui8PinData, direction);

        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3, ui8PinData);
        SysCtlDelay(8000000);

        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3, 0x00);
        SysCtlDelay(2000000);
    }
}