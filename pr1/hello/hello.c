#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "inc/hw_memmap.h"
#include "inc/hw_gpio.h"
#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "drivers/buttons.h"
#include "drivers/rgb.h"

int main(void)
{

    ButtonsInit();
    RGBInit(1);

    uint32_t colorBlue[3] = {0, 0, 0xFFFF};
    uint32_t colorRed[3] = {0xFFFF, 0, 0};
    uint32_t colorGreen[3] = {0, 0xffff, 0};
    uint32_t colorYellow[3] = {0xffff, 0xffff, 0};

    uint32_t defaultColor[3];
    memcpy(defaultColor, colorBlue, sizeof(colorBlue));

    RGBColorSet(defaultColor);

    uint8_t changed, buttons;
    while (1)
    {
        buttons = ButtonsPoll(&changed, NULL);

        if ((buttons & ALL_BUTTONS) == ALL_BUTTONS)
        {
            RGBColorSet(colorYellow);
        }
        else if (BUTTON_PRESSED(LEFT_BUTTON, buttons, changed))
        {
            RGBColorSet(colorGreen);
        }
        else if (BUTTON_PRESSED(RIGHT_BUTTON, buttons, changed))
        {
            RGBColorSet(colorRed);
        }
        else if (changed && buttons == 0)
        {
            RGBColorSet(defaultColor);
        }
    }
}