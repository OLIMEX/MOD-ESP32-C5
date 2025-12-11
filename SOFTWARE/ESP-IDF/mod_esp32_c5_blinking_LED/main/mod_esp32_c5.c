#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#define TAG "APP"

void app_main(void)
{

    esp_rom_gpio_pad_select_gpio(26);
    esp_rom_gpio_pad_select_gpio(27);

	gpio_set_direction(26, GPIO_MODE_OUTPUT);
	gpio_set_direction(27, GPIO_MODE_OUTPUT);
     
    gpio_set_level(26, 1);
    gpio_set_level(27, 1);

    vTaskDelay(pdMS_TO_TICKS(100));
    
    while (1)
    {
        gpio_set_level(26,0);
        gpio_set_level(27,1);
        vTaskDelay(pdMS_TO_TICKS(500));

        gpio_set_level(26,1);
        gpio_set_level(27,0);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

