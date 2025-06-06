//*****************************************************************************
//
// usb_dev_keyboard.c - Main routines for the keyboard example.
//
// Copyright (c) 2011-2020 Texas Instruments Incorporated.  All rights reserved.
// Software License Agreement
//
// Texas Instruments (TI) is supplying this software for use solely and
// exclusively on TI's microcontroller products. The software is owned by
// TI and/or its suppliers, and is protected under applicable copyright
// laws. You may not combine this software with "viral" open-source
// software in order to form a larger program.
//
// THIS SOFTWARE IS PROVIDED "AS IS" AND WITH ALL FAULTS.
// NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT
// NOT LIMITED TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. TI SHALL NOT, UNDER ANY
// CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR CONSEQUENTIAL
// DAMAGES, FOR ANY REASON WHATSOEVER.
//
// This is part of revision 2.2.0.295 of the EK-TM4C123GXL Firmware Package.
//
//*****************************************************************************

#include <stdbool.h>
#include <stdint.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "inc/hw_sysctl.h"
#include "driverlib/debug.h"
#include "driverlib/fpu.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/uart.h"
#include "usblib/usblib.h"
#include "usblib/usbhid.h"
#include "usblib/device/usbdevice.h"
#include "usblib/device/usbdhid.h"
#include "usblib/device/usbdhidkeyb.h"
#include "drivers/buttons.h"
#include "utils/uartstdio.h"
#include "utils/ustdlib.h"
#include "usb_keyb_structs.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>USB HID Keyboard Device (usb_dev_keyboard)</h1>
//!
//! This example turns the EK-TM4C123GXL LaunchPad into a USB keyboard
//! supporting the Human Interface Device class.  When either the SW1/SW2
//! push button is pressed, a sequence of key presses is simulated to type a
//! string.  Care should be taken to ensure that the active window can safely
//! receive the text; enter is not pressed at any point so no actions are
//! attempted by the host if a terminal window is used (for example).  The
//! status LED is used to indicate the current Caps Lock state and is updated
//! in response to any other keyboard attached to the same USB host system.
//!
//! The device implemented by this application also supports USB remote wakeup
//! allowing it to request the host to reactivate a suspended bus.  If the bus
//! is suspended (as indicated on the application display), pressing the
//! push button will request a remote wakeup assuming the host has not
//! specifically disabled such requests.
//
//*****************************************************************************

//*****************************************************************************
//
// The system tick timer period.
//
//*****************************************************************************
#define SYSTICKS_PER_SECOND 100

//*****************************************************************************
//
// A mapping from the ASCII value received from the UART to the corresponding
// USB HID usage code.
//
//*****************************************************************************
static const int8_t g_ppi8KeyUsageCodes[][2] =
    {
        {0, HID_KEYB_USAGE_SPACE},                       //   0x20
        {HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_1},         // ! 0x21
        {HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_FQUOTE},    // " 0x22
        {HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_3},         // # 0x23
        {HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_4},         // $ 0x24
        {HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_5},         // % 0x25
        {HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_7},         // & 0x26
        {0, HID_KEYB_USAGE_FQUOTE},                      // ' 0x27
        {HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_9},         // ( 0x28
        {HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_0},         // ) 0x29
        {HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_8},         // * 0x2a
        {HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_EQUAL},     // + 0x2b
        {0, HID_KEYB_USAGE_COMMA},                       // , 0x2c
        {0, HID_KEYB_USAGE_MINUS},                       // - 0x2d
        {0, HID_KEYB_USAGE_PERIOD},                      // . 0x2e
        {0, HID_KEYB_USAGE_FSLASH},                      // / 0x2f
        {0, HID_KEYB_USAGE_0},                           // 0 0x30
        {0, HID_KEYB_USAGE_1},                           // 1 0x31
        {0, HID_KEYB_USAGE_2},                           // 2 0x32
        {0, HID_KEYB_USAGE_3},                           // 3 0x33
        {0, HID_KEYB_USAGE_4},                           // 4 0x34
        {0, HID_KEYB_USAGE_5},                           // 5 0x35
        {0, HID_KEYB_USAGE_6},                           // 6 0x36
        {0, HID_KEYB_USAGE_7},                           // 7 0x37
        {0, HID_KEYB_USAGE_8},                           // 8 0x38
        {0, HID_KEYB_USAGE_9},                           // 9 0x39
        {HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_SEMICOLON}, // : 0x3a
        {0, HID_KEYB_USAGE_SEMICOLON},                   // ; 0x3b
        {HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_COMMA},     // < 0x3c
        {0, HID_KEYB_USAGE_EQUAL},                       // = 0x3d
        {HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_PERIOD},    // > 0x3e
        {HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_FSLASH},    // ? 0x3f
        {HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_2},         // @ 0x40
        {HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_A},         // A 0x41
        {HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_B},         // B 0x42
        {HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_C},         // C 0x43
        {HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_D},         // D 0x44
        {HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_E},         // E 0x45
        {HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_F},         // F 0x46
        {HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_G},         // G 0x47
        {HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_H},         // H 0x48
        {HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_I},         // I 0x49
        {HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_J},         // J 0x4a
        {HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_K},         // K 0x4b
        {HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_L},         // L 0x4c
        {HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_M},         // M 0x4d
        {HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_N},         // N 0x4e
        {HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_O},         // O 0x4f
        {HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_P},         // P 0x50
        {HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_Q},         // Q 0x51
        {HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_R},         // R 0x52
        {HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_S},         // S 0x53
        {HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_T},         // T 0x54
        {HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_U},         // U 0x55
        {HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_V},         // V 0x56
        {HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_W},         // W 0x57
        {HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_X},         // X 0x58
        {HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_Y},         // Y 0x59
        {HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_Z},         // Z 0x5a
        {0, HID_KEYB_USAGE_LBRACKET},                    // [ 0x5b
        {0, HID_KEYB_USAGE_BSLASH},                      // \ 0x5c
        {0, HID_KEYB_USAGE_RBRACKET},                    // ] 0x5d
        {HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_6},         // ^ 0x5e
        {HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_MINUS},     // _ 0x5f
        {0, HID_KEYB_USAGE_BQUOTE},                      // ` 0x60
        {0, HID_KEYB_USAGE_A},                           // a 0x61
        {0, HID_KEYB_USAGE_B},                           // b 0x62
        {0, HID_KEYB_USAGE_C},                           // c 0x63
        {0, HID_KEYB_USAGE_D},                           // d 0x64
        {0, HID_KEYB_USAGE_E},                           // e 0x65
        {0, HID_KEYB_USAGE_F},                           // f 0x66
        {0, HID_KEYB_USAGE_G},                           // g 0x67
        {0, HID_KEYB_USAGE_H},                           // h 0x68
        {0, HID_KEYB_USAGE_I},                           // i 0x69
        {0, HID_KEYB_USAGE_J},                           // j 0x6a
        {0, HID_KEYB_USAGE_K},                           // k 0x6b
        {0, HID_KEYB_USAGE_L},                           // l 0x6c
        {0, HID_KEYB_USAGE_M},                           // m 0x6d
        {0, HID_KEYB_USAGE_N},                           // n 0x6e
        {0, HID_KEYB_USAGE_O},                           // o 0x6f
        {0, HID_KEYB_USAGE_P},                           // p 0x70
        {0, HID_KEYB_USAGE_Q},                           // q 0x71
        {0, HID_KEYB_USAGE_R},                           // r 0x72
        {0, HID_KEYB_USAGE_S},                           // s 0x73
        {0, HID_KEYB_USAGE_T},                           // t 0x74
        {0, HID_KEYB_USAGE_U},                           // u 0x75
        {0, HID_KEYB_USAGE_V},                           // v 0x76
        {0, HID_KEYB_USAGE_W},                           // w 0x77
        {0, HID_KEYB_USAGE_X},                           // x 0x78
        {0, HID_KEYB_USAGE_Y},                           // y 0x79
        {0, HID_KEYB_USAGE_Z},                           // z 0x7a
        {HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_LBRACKET},  // { 0x7b
        {HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_BSLASH},    // | 0x7c
        {HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_RBRACKET},  // } 0x7d
        {HID_KEYB_LEFT_SHIFT, HID_KEYB_USAGE_BQUOTE},    // ~ 0x7e
        //
        // Add characters outside of 0x20-0x7e here to avoid breaking the table
        // lookup calculations.
        //
        {0, HID_KEYB_USAGE_ENTER}, // LF 0x0A
};

//*****************************************************************************
//
// This global indicates whether or not we are connected to a USB host.
//
//*****************************************************************************
volatile bool g_bConnected = false;

//*****************************************************************************
//
// This global indicates whether or not the USB bus is currently in the suspend
// state.
//
//*****************************************************************************
volatile bool g_bSuspended = false;

//*****************************************************************************
//
// Global system tick counter holds elapsed time since the application started
// expressed in 100ths of a second.
//
//*****************************************************************************
volatile uint32_t g_ui32SysTickCount;

//*****************************************************************************
//
// The number of system ticks to wait for each USB packet to be sent before
// we assume the host has disconnected.  The value 50 equates to half a second.
//
//*****************************************************************************
#define MAX_SEND_DELAY 50

//*****************************************************************************
//
// This global is set to true if the host sends a request to set or clear
// any keyboard LED.
//
//*****************************************************************************
volatile bool g_bDisplayUpdateRequired;

//*****************************************************************************
//
// This enumeration holds the various states that the keyboard can be in during
// normal operation.
//
//*****************************************************************************
volatile enum {
    //
    // Unconfigured.
    //
    STATE_UNCONFIGURED,

    //
    // No keys to send and not waiting on data.
    //
    STATE_IDLE,

    //
    // Waiting on data to be sent out.
    //
    STATE_SENDING
} g_eKeyboardState = STATE_UNCONFIGURED;

//*****************************************************************************
//
// The error routine that is called if the driver library encounters an error.
//
//*****************************************************************************
#ifdef DEBUG
void __error__(char *pcFilename, uint32_t ui32Line)
{
}
#endif

//*****************************************************************************
//
// Handles asynchronous events from the HID keyboard driver.
//
// \param pvCBData is the event callback pointer provided during
// USBDHIDKeyboardInit().  This is a pointer to our keyboard device structure
// (&g_sKeyboardDevice).
// \param ui32Event identifies the event we are being called back for.
// \param ui32MsgData is an event-specific value.
// \param pvMsgData is an event-specific pointer.
//
// This function is called by the HID keyboard driver to inform the application
// of particular asynchronous events related to operation of the keyboard HID
// device.
//
// \return Returns 0 in all cases.
//
//*****************************************************************************
uint32_t
KeyboardHandler(void *pvCBData, uint32_t ui32Event, uint32_t ui32MsgData,
                void *pvMsgData)
{
    switch (ui32Event)
    {
    //
    // The host has connected to us and configured the device.
    //
    case USB_EVENT_CONNECTED:
    {
        g_bConnected = true;
        g_bSuspended = false;
        break;
    }

    //
    // The host has disconnected from us.
    //
    case USB_EVENT_DISCONNECTED:
    {
        g_bConnected = false;
        break;
    }

    //
    // We receive this event every time the host acknowledges transmission
    // of a report.  It is used here purely as a way of determining whether
    // the host is still talking to us or not.
    //
    case USB_EVENT_TX_COMPLETE:
    {
        //
        // Enter the idle state since we finished sending something.
        //
        g_eKeyboardState = STATE_IDLE;
        break;
    }

    //
    // This event indicates that the host has suspended the USB bus.
    //
    case USB_EVENT_SUSPEND:
    {
        g_bSuspended = true;
        break;
    }

    //
    // This event signals that the host has resumed signalling on the bus.
    //
    case USB_EVENT_RESUME:
    {
        g_bSuspended = false;
        break;
    }

    //
    // This event indicates that the host has sent us an Output or
    // Feature report and that the report is now in the buffer we provided
    // on the previous USBD_HID_EVENT_GET_REPORT_BUFFER callback.
    //
    case USBD_HID_KEYB_EVENT_SET_LEDS:
    {
        //
        // Set the LED to match the current state of the caps lock LED.
        //
        MAP_GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2,
                         (ui32MsgData & HID_KEYB_CAPS_LOCK) ? GPIO_PIN_2 : 0);

        break;
    }

    //
    // We ignore all other events.
    //
    default:
    {
        break;
    }
    }

    return (0);
}

//***************************************************************************
//
// Wait for a period of time for the state to become idle.
//
// \param ui32TimeoutTick is the number of system ticks to wait before
// declaring a timeout and returning \b false.
//
// This function polls the current keyboard state for ui32TimeoutTicks system
// ticks waiting for it to become idle.  If the state becomes idle, the
// function returns true.  If it ui32TimeoutTicks occur prior to the state
// becoming idle, false is returned to indicate a timeout.
//
// \return Returns \b true on success or \b false on timeout.
//
//***************************************************************************
bool WaitForSendIdle(uint_fast32_t ui32TimeoutTicks)
{
    uint32_t ui32Start;
    uint32_t ui32Now;
    uint32_t ui32Elapsed;

    ui32Start = g_ui32SysTickCount;
    ui32Elapsed = 0;

    while (ui32Elapsed < ui32TimeoutTicks)
    {
        //
        // Is the keyboard is idle, return immediately.
        //
        if (g_eKeyboardState == STATE_IDLE)
        {
            return (true);
        }

        //
        // Determine how much time has elapsed since we started waiting.  This
        // should be safe across a wrap of g_ui32SysTickCount.
        //
        ui32Now = g_ui32SysTickCount;
        ui32Elapsed = ((ui32Start < ui32Now) ? (ui32Now - ui32Start) : (((uint32_t)0xFFFFFFFF - ui32Start) + ui32Now + 1));
    }

    //
    // If we get here, we timed out so return a bad return code to let the
    // caller know.
    //
    return (false);
}

//*****************************************************************************
//
// Sends a string of characters via the USB HID keyboard interface.
//
//*****************************************************************************
void SendString(char *pcStr)
{
    uint32_t ui32Char;

    //
    // Loop while there are more characters in the string.
    //
    while (*pcStr)
    {
        //
        // Get the next character from the string.
        //
        ui32Char = *pcStr++;

        //
        // Skip this character if it is a non-printable character.
        //
        if ((ui32Char < ' ') || (ui32Char > '~'))
        {
            //
            // Allow LF to work with this example.
            //
            if (ui32Char != '\n')
            {
                continue;
            }
        }

        //
        // Check for LF and if there is one, assign the table value.
        // Otherwise, convert the character per the keyboard table.
        //
        if (ui32Char == '\n')
        {
            ui32Char = 0x5f;
        }
        else
        {
            //
            // Convert the character into an index into the keyboard usage code
            // table.
            //
            ui32Char -= ' ';
        }

        //
        // Send the key press message.
        //
        g_eKeyboardState = STATE_SENDING;
        if (USBDHIDKeyboardKeyStateChange((void *)&g_sKeyboardDevice,
                                          g_ppi8KeyUsageCodes[ui32Char][0],
                                          g_ppi8KeyUsageCodes[ui32Char][1],
                                          true) != KEYB_SUCCESS)
        {
            return;
        }

        //
        // Wait until the key press message has been sent.
        //
        if (!WaitForSendIdle(MAX_SEND_DELAY))
        {
            g_bConnected = 0;
            return;
        }

        //
        // Send the key release message.
        //
        g_eKeyboardState = STATE_SENDING;
        if (USBDHIDKeyboardKeyStateChange((void *)&g_sKeyboardDevice,
                                          0, g_ppi8KeyUsageCodes[ui32Char][1],
                                          false) != KEYB_SUCCESS)
        {
            return;
        }

        //
        // Wait until the key release message has been sent.
        //
        if (!WaitForSendIdle(MAX_SEND_DELAY))
        {
            g_bConnected = 0;
            return;
        }
    }
}

void SendArrowPress(char direction)
{
    uint8_t hidCode = 0;

    switch (direction)
    {
    case 'u':
        hidCode = 0x52;
        break;
    case 'd':
        hidCode = 0x51;
        break;
    default:
        return;
    }

    g_eKeyboardState = STATE_SENDING;
    if (USBDHIDKeyboardKeyStateChange((void *)&g_sKeyboardDevice, 0, hidCode, true) != KEYB_SUCCESS)
        return;

    if (!WaitForSendIdle(MAX_SEND_DELAY))
        g_bConnected = 0;
}

void SendArrowRelease(char direction)
{
    uint8_t hidCode = 0;

    switch (direction)
    {
    case 'u':
        hidCode = 0x52;
        break;
    case 'd':
        hidCode = 0x51;
        break;
    default:
        return;
    }

    g_eKeyboardState = STATE_SENDING;
    if (USBDHIDKeyboardKeyStateChange((void *)&g_sKeyboardDevice, 0, hidCode, false) != KEYB_SUCCESS)
        return;

    if (!WaitForSendIdle(MAX_SEND_DELAY))
        g_bConnected = 0;
}

//*****************************************************************************
//
// This is the interrupt handler for the SysTick interrupt.  It is used to
// update our local tick count which, in turn, is used to check for transmit
// timeouts.
//
//*****************************************************************************
void SysTickIntHandler(void)
{
    g_ui32SysTickCount++;
}

//*****************************************************************************
//
// Configure the UART and its pins.  This must be called before UARTprintf().
//
//*****************************************************************************
void ConfigureUART(void)
{
    //
    // Enable the GPIO Peripheral used by the UART.
    //
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

    //
    // Enable UART0.
    //
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);

    //
    // Configure GPIO Pins for UART mode.
    //
    MAP_GPIOPinConfigure(GPIO_PA0_U0RX);
    MAP_GPIOPinConfigure(GPIO_PA1_U0TX);
    MAP_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    //
    // Use the internal 16MHz oscillator as the UART clock source.
    //
    UARTClockSourceSet(UART0_BASE, UART_CLOCK_PIOSC);

    //
    // Initialize the UART for console I/O.
    //
    UARTStdioConfig(0, 115200, 16000000);
}

//*****************************************************************************
//
// This is the main loop that runs the application.
//
//*****************************************************************************
int main(void)
{
    uint_fast32_t ui32LastTickCount;
    bool bLastSuspend;

    //
    // Enable lazy stacking for interrupt handlers.  This allows floating-point
    // instructions to be used within interrupt handlers, but at the expense of
    // extra stack usage.
    //
    MAP_FPULazyStackingEnable();

    //
    // Set the clocking to run from the PLL at 50MHz.
    //
    MAP_SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |
                       SYSCTL_XTAL_16MHZ);

    //
    // Initialize the UART and display initial message.
    //
    ConfigureUART();
    UARTprintf("usb-dev-keyboard example\n\r");

    //
    // Configure the required pins for USB operation.
    //
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    MAP_GPIOPinTypeUSBAnalog(GPIO_PORTD_BASE, GPIO_PIN_4 | GPIO_PIN_5);

    //
    // Erratum workaround for silicon revision A1.  VBUS must have pull-down.
    //
    if (CLASS_IS_TM4C123 && REVISION_IS_A1)
    {
        HWREG(GPIO_PORTB_BASE + GPIO_O_PDR) |= GPIO_PIN_1;
    }

    //
    // Enable the GPIO that is used for the on-board LED.
    //
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    MAP_GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_2);
    MAP_GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, 0);

    //
    // Initialize the buttons driver.
    //
    ButtonsInit();

    //
    // Not configured initially.
    //
    g_bConnected = false;
    g_bSuspended = false;
    bLastSuspend = false;

    //
    // Initialize the USB stack for device mode.  We do not operate in USB
    // device mode with active monitoring of VBUS and therefore, we will
    // specify eUSBModeForceDevice as the operating mode instead of
    // eUSBModeDevice.  To use eUSBModeDevice, the EK-TM4C123GXL LaunchPad
    // must have the R28 and R29 populated with zero ohm resistors.
    //
    USBStackModeSet(0, eUSBModeForceDevice, 0);

    //
    // Pass our device information to the USB HID device class driver,
    // initialize the USB controller, and connect the device to the bus.
    //
    USBDHIDKeyboardInit(0, &g_sKeyboardDevice);

    //
    // Set the system tick to fire 100 times per second.
    //
    MAP_SysTickPeriodSet(MAP_SysCtlClockGet() / SYSTICKS_PER_SECOND);
    MAP_SysTickIntEnable();
    MAP_SysTickEnable();

    //
    // The main loop starts here.  We begin by waiting for a host connection
    // then drop into the main keyboard handling section.  If the host
    // disconnects, we return to the top and wait for a new connection.
    //
    bool prevButtonStateSW1 = true;
    bool prevButtonStateSW2 = true;
    while (1)
    {
        uint8_t ui8Buttons;
        uint8_t ui8ButtonsChanged;

        UARTprintf("Waiting for host...\n\r");

        //
        // Wait here until USB device is connected to a host.
        //
        while (!g_bConnected)
        {
        }

        UARTprintf("Host connected.\n\r");
        UARTprintf("Now press any button.\n\r");

        //
        // Enter the idle state.
        //
        g_eKeyboardState = STATE_IDLE;

        //
        // Assume that the bus is not currently suspended if we have just been
        // configured.
        //
        bLastSuspend = false;

        //
        // Keep transferring characters from the UART to the USB host for as
        // long as we are connected to the host.
        //
        while (g_bConnected)
        {
            //
            // Remember the current time.
            //
            ui32LastTickCount = g_ui32SysTickCount;

            //
            // Has the suspend state changed since last time we checked?
            //
            if (bLastSuspend != g_bSuspended)
            {
                //
                // Yes, update the state on the terminal.
                //
                bLastSuspend = g_bSuspended;
                if (bLastSuspend)
                {
                    UARTprintf("Bus suspended ...\n\r");
                }
                else
                {
                    UARTprintf("Host connected ...\n\r");
                }
            }

            //
            // See if the button was just pressed.
            //

            //-dino

            bool currSW1 = GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_4);
            bool currSW2 = GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_0);

            ui8Buttons = ButtonsPoll(&ui8ButtonsChanged, 0);
            if (currSW1 != prevButtonStateSW1)
            {
                if (!currSW1)
                    SendArrowPress('u');
                else
                    SendArrowRelease('u');
                prevButtonStateSW1 = currSW1;
            }

                  if (currSW2 != prevButtonStateSW2)
            {
                if (!currSW2)
                    SendArrowPress('d');
                else
                    SendArrowRelease('d');
                prevButtonStateSW2 = currSW2;
            }
            //--

            // if (BUTTON_PRESSED(LEFT_BUTTON, ui8Buttons,
            //                    ui8ButtonsChanged))
            // {
            //     //
            //     // If the bus is suspended then resume it.  Otherwise, type
            //     // out an instructional message.
            //     //
            //     if (g_bSuspended)
            //     {
            //         USBDHIDKeyboardRemoteWakeupRequest(
            //             (void *)&g_sKeyboardDevice);
            //     }
            //     else
            //     {
            //         // SendString("You have pressed the SW1 button.\n"
            //         //            "Try pressing the SW2 button.\n\n");
            //         SendArrow('u');
            //     }
            // }
            // else if (BUTTON_PRESSED(RIGHT_BUTTON, ui8Buttons,
            //                         ui8ButtonsChanged))
            // {
            //     //
            //     // If the bus is suspended then resume it.  Otherwise, type
            //     // out an instructional message.
            //     //
            //     if (g_bSuspended)
            //     {
            //         USBDHIDKeyboardRemoteWakeupRequest(
            //             (void *)&g_sKeyboardDevice);
            //     }
            //     else
            //     {
            //         // SendString("You have pressed the SW2 button.\n"
            //         //            "Try pressing the Caps Lock key on your "
            //         //            "keyboard and then press either button.\n\n");
            //         SendArrow('d');
            //     }
            // }

            //
            // Wait for at least 1 system tick to have gone by before we poll
            // the buttons again.
            //
            while (g_ui32SysTickCount == ui32LastTickCount)
            {
            }
        }
    }
}
