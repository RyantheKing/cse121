#include "driver/i2c.h"
#include "DFRobot_LCD.h"

extern "C" void app_main(void)
{
    DFRobot_LCD lcd(16, 2, LCD_ADDRESS, RGB_ADDRESS);

    while (true) {
        lcd.init();
        lcd.setRGB(0, 255, 0);
        lcd.printstr("Hello CSE121!");
        lcd.setCursor(0, 1);
        lcd.printstr("King");

        // delay 1 second
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
