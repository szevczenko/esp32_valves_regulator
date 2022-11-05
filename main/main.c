#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "config.h"

#include "wifidrv.h"
#include "cmd_server.h"
#include "but.h"
#include "fast_add.h"
#include "ssd1306.h"
#include "intf/i2c/ssd1306_i2c.h"
#include "menu.h"
#include "keepalive.h"
#include "menu_param.h"
#include "cmd_client.h"
#include "measure.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "driver/uart.h"
#include "pcf8574.h"
#include "battery.h"
#include "esp_attr.h"
#include "buzzer.h"
#include "esp_sleep.h"
#include "sleep_e.h"
#include "server_controller.h"
#include "power_on.h"
#include "error_valve.h"
#include "oled.h"

extern void ultrasonar_start(void);

static gpio_config_t io_conf;
static uint32_t blink_pin = GPIO_NUM_23;
portMUX_TYPE portMux = portMUX_INITIALIZER_UNLOCKED;

static bool check_i2c_communication(void)
{
    ssd1306_i2cInitEx(I2C_EXAMPLE_MASTER_SCL_IO, I2C_EXAMPLE_MASTER_SDA_IO, SSD1306_I2C_ADDR);
    uint8_t s_i2c_addr = 0x3C;
    int ret;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, ( s_i2c_addr << 1 ) | I2C_MASTER_WRITE, 0x1);
    i2c_master_write_byte(cmd, 0x00, 0x1);
    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(I2C_NUM_1, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    printf("I2C TEST %d\n\r", ret);
    return ret == ESP_OK;
}

void debug_function_name(const char *name)
{
}

void debug_last_task(char *task_name)
{
}

void debug_last_out_task(char *task_name)
{
}

void graphic_init(void)
{
    sh1106_128x64_init();
    ssd1306_clearScreen();
    ssd1306_fillScreen(0x00);
    ssd1306_flipHorizontal(1);
    ssd1306_flipVertical(1);
    oled_init();
}

static void checkDevType(void)
{
    pcf8574_init();
    bool read_i2c_value = check_i2c_communication();
    if (read_i2c_value)
    {
        config.wifi_type = T_WIFI_TYPE_CLIENT;
    }
    else
    {
        config.wifi_type = T_WIFI_TYPE_SERVER;
    }
}

void app_main()
{
    configInit();
    checkDevType();

    if (config.wifi_type != T_WIFI_TYPE_SERVER)
    {
        graphic_init();
        battery_init();
        init_leds();
        osDelay(10);

        buzzer_init();
        power_on_init();
        
        /* Wait to measure voltage */
        while (!battery_is_measured())
        {
            osDelay(10);
        }

        float voltage = battery_get_voltage();
        power_on_enable_system();
        init_buttons();
        menuParamInit();

        if (voltage > 3.2)
        {
            init_menu(MENU_DRV_NORMAL_INIT);
            wifiDrvInit();
            keepAliveStartTask();
            dictionary_init();
            fastProcessStartTask();
            power_on_start_task();
            // init_sleep();
        }
        else
        {
            init_menu(MENU_DRV_LOW_BATTERY_INIT);
            power_on_disable_system();
        }
    }
    else
    {
        wifiDrvInit();
        keepAliveStartTask();
        menuParamInit();

        measure_start();
        srvrControllStart();
        ultrasonar_start();
        //WYLACZONE

        errorStart();

        //LED on
        io_conf.intr_type = GPIO_INTR_DISABLE;
        io_conf.mode = GPIO_MODE_OUTPUT;
        io_conf.pin_bit_mask = (1 << blink_pin);
        io_conf.pull_down_en = 0;
        io_conf.pull_up_en = 0;
        gpio_config(&io_conf);
        gpio_set_level(blink_pin, 1);
    }

    config_printf(PRINT_DEBUG, PRINT_DEBUG, "[MENU] ------------START SYSTEM-------------");
    while (1)
    {
        vTaskDelay(MS2ST(250));
        if ((config.wifi_type == T_WIFI_TYPE_SERVER) && !cmdServerIsWorking())
        {
            gpio_set_level(blink_pin, 0);
        }

        vTaskDelay(MS2ST(750));

        if (config.wifi_type == T_WIFI_TYPE_SERVER)
        {
            gpio_set_level(blink_pin, 1);
        }
    }
}
