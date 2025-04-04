#include <stdint.h>
#include <stdbool.h>
#include "inc/tm4c123gh6pm.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h"
#include "driverlib/gpio.h"
#include "driverlib/timer.h"

volatile uint32_t g_ui32Counter = 0; //счетчик секунд
volatile bool g_bButtonPressed = false;
volatile uint32_t g_ui32BlinkCount = 0;

void Timer0A_Handler(void);
void Timer1A_Handler(void);
void GPIOF_Handler(void);
void SetLED(uint8_t red, uint8_t blue, uint8_t green);

int main(void)
{
    uint32_t ui32Period;

    SysCtlClockSet(SYSCTL_SYSDIV_5 | SYSCTL_USE_PLL |
                   SYSCTL_XTAL_16MHZ | SYSCTL_OSC_MAIN);

    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE,
                          GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3);

    GPIOPinTypeGPIOInput(GPIO_PORTF_BASE, GPIO_PIN_4);
    GPIOPadConfigSet(GPIO_PORTF_BASE, GPIO_PIN_4,
                     GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);

    GPIOIntRegister(GPIO_PORTF_BASE, GPIOF_Handler);
    GPIOIntTypeSet(GPIO_PORTF_BASE, GPIO_PIN_4, GPIO_FALLING_EDGE);
    GPIOIntEnable(GPIO_PORTF_BASE, GPIO_INT_PIN_4);
    IntEnable(INT_GPIOF);

    //  1 сек
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
    TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);
    ui32Period = SysCtlClockGet(); // 1
    TimerLoadSet(TIMER0_BASE, TIMER_A, ui32Period - 1);
    IntRegister(INT_TIMER0A, Timer0A_Handler);
    IntEnable(INT_TIMER0A);
    TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
    TimerEnable(TIMER0_BASE, TIMER_A);

    // 0.1 сек
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER1);
    TimerConfigure(TIMER1_BASE, TIMER_CFG_PERIODIC);
    TimerLoadSet(TIMER1_BASE, TIMER_A,
                 (SysCtlClockGet() / 10) - 1); // 0.1 
    IntRegister(INT_TIMER1A, Timer1A_Handler);
    IntEnable(INT_TIMER1A);
    TimerIntEnable(TIMER1_BASE, TIMER_TIMA_TIMEOUT);

    
    IntMasterEnable();

    while (1)
    {
        if (!g_bButtonPressed)
        {
            if (g_ui32Counter % 2 == 0)
                SetLED(1, 0, 0); 
            else
                SetLED(0, 1, 0); 
        }
    }
}

void SetLED(uint8_t red, uint8_t blue, uint8_t green)
{
    uint8_t out = 0;
    if (red) out |= GPIO_PIN_1;
    if (blue) out |= GPIO_PIN_2;
    if (green) out |= GPIO_PIN_3;
    GPIOPinWrite(GPIO_PORTF_BASE,
                 GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3,
                 out);
}

void Timer0A_Handler(void)
{
    TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
    if (!g_bButtonPressed)
        g_ui32Counter++;
}

void Timer1A_Handler(void)
{
    static bool bLedState = false;
    TimerIntClear(TIMER1_BASE, TIMER_TIMA_TIMEOUT);

    g_ui32BlinkCount++;

    uint8_t current = GPIOPinRead(GPIO_PORTF_BASE,
                                  GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3);
    current &= ~GPIO_PIN_3; 

    if (g_ui32BlinkCount >= 20)
    {
        TimerDisable(TIMER1_BASE, TIMER_A);
        g_ui32BlinkCount = 0;
        g_bButtonPressed = false;
        GPIOPinWrite(GPIO_PORTF_BASE,
                     GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3,
                     current);
    }
    else
    {
        bLedState = !bLedState;
        if (bLedState)
            current |= GPIO_PIN_3;
        GPIOPinWrite(GPIO_PORTF_BASE,
                     GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3,
                     current);
    }
}

void GPIOF_Handler(void)
{
    GPIOIntClear(GPIO_PORTF_BASE, GPIO_INT_PIN_4);
    if (!g_bButtonPressed)
    {
        g_bButtonPressed = true;
        g_ui32BlinkCount = 0;
        TimerEnable(TIMER1_BASE, TIMER_A);
    }
}
