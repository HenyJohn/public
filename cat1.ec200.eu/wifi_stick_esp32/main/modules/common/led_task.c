/* GPIO Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp32_time.h"
#include "data_process.h"

#define BLUE_LED_GPIO 22  //blue
#define GREEN_LED_GPIO 23 //green
#define CAT1_PWR_GPIO 21  //pwr
#define GPIO_OUTPUT_PIN_SEL ((1ULL << BLUE_LED_GPIO) | (1ULL << GREEN_LED_GPIO) | (1ULL << CAT1_PWR_GPIO))

#define SLOW_BLINK_PERIOD_MS 1000
#define FAST_BLINK_PERIOD_MS 100
extern int work_mode;
// extern mqtt_connect_state;
extern netework_state;

// int ap_enable = 1;

enum Light_Type
{
    LIGHT_OFF,
    LIGHT_SLOW_BLINK,
    LIGHT_FAST_BLINK,
    LIGHT_ON
};

int check_inv_states();

/** LED灯刷新，无延时*/
void fresh_led(int gpio_num, int light_type)
{
    static int64_t blue_last_time_us = 0;
    static int64_t green_last_time_us = 0;
    static uint32_t blue_last_state = 0;
    static uint32_t green_last_state = 0;

    int64_t *p_last_time_us = NULL;
    uint32_t *p_last_state = NULL;
    int64_t now = esp_timer_get_time();
    switch (gpio_num)
    {
    case BLUE_LED_GPIO:
        p_last_time_us = &blue_last_time_us;
        p_last_state = &blue_last_state;
        break;
    case GREEN_LED_GPIO:
        p_last_time_us = &green_last_time_us;
        p_last_state = &green_last_state;
        break;
    default:
        usleep(50000);
        return;
        break;
    }

    switch (light_type)
    {
    case LIGHT_OFF:
        gpio_set_level(gpio_num, 0);
        break;
    case LIGHT_ON:
        gpio_set_level(gpio_num, 1);
        break;
    case LIGHT_FAST_BLINK:
        if (now - *p_last_time_us > FAST_BLINK_PERIOD_MS * 1000)
        {
            gpio_set_level(gpio_num, !(*p_last_state));
            *p_last_time_us = now;
            *p_last_state = !(*p_last_state);
        }
        break;
    case LIGHT_SLOW_BLINK:
        if (now - *p_last_time_us > SLOW_BLINK_PERIOD_MS * 1000)
        {
            gpio_set_level(gpio_num, !(*p_last_state)); /** 逻辑取反*/
            *p_last_time_us = now;
            *p_last_state = !(*p_last_state);
        }
        break;

    default:
        usleep(50000);
        return;
        break;
    }
}

void set_cat1_pwr_gpio(int var)
{
    gpio_set_level(CAT1_PWR_GPIO, var);
}

void gpio_ini(void)
{
    gpio_config_t io_conf;
    //disable interrupt
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    //set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    //bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //disable pull-up mode
    io_conf.pull_up_en = 1;
    //configure GPIO with the given settings
    gpio_config(&io_conf);
}

void mqtt_fail_reboot_check(void)
{
    static int last = 0;
    int now = get_second_sys_time();
    if (cat1_get_mqtt_status(0) == 1 && cat1_get_data_call_status() == 1)
        last = now;
    else if (now - last > 1800)
    {
        if (xSemaphoreTake(invdata_store_sema, portMAX_DELAY) == pdTRUE)
        {
            cat1_fast_reboot();
            sleep(15);
            esp_restart();
        }
    }
}

#include "stdbool.h"
void led_task(void *pvParameters)
{
    int sts = 0;
    int sta_disconnect_time = 0, ap_disconnect_time = 0;
    int re_conn = 0;
    int sta_ap_notexist = 0;

    fresh_led(GREEN_LED_GPIO, LIGHT_OFF);
    fresh_led(BLUE_LED_GPIO, LIGHT_OFF);
    while (1)
    {
        /** mqtt异常持续超过30分钟则重启 ********/
        mqtt_fail_reboot_check();
        /** 逆变器：绿灯********************************************/
        sts = check_inv_states();
        /** 全部逆变器通信中断 */
        if (sts == -1)
            fresh_led(GREEN_LED_GPIO, LIGHT_OFF);
        /** 部分逆变器通信中断 */
        else if (sts == 0)
        {
            fresh_led(GREEN_LED_GPIO, LIGHT_SLOW_BLINK);
        }
        /** 全部逆变器通信正常 */
        else if (sts == 1)
        {
            fresh_led(GREEN_LED_GPIO, LIGHT_ON);
        }

        /** 蓝灯***************************************************/
        int reg_stat = cat1_get_cereg_status() || cat1_get_cgreg_status();
        int data_stat = cat1_get_data_call_status();
        if (reg_stat == true)
        {
            if (data_stat == false)
            {
                fresh_led(BLUE_LED_GPIO, LIGHT_FAST_BLINK);
            }
            else
            {
                if (cat1_get_mqtt_status(0) == false)
                    fresh_led(BLUE_LED_GPIO, LIGHT_SLOW_BLINK);
                else
                {
                    if (cat1_get_net_mode_value() == 7)
                        fresh_led(BLUE_LED_GPIO, LIGHT_ON);
                    else
                    {
                        fresh_led(BLUE_LED_GPIO, LIGHT_SLOW_BLINK);
                    }
                }
            }
        }
        else
        {
            fresh_led(BLUE_LED_GPIO, LIGHT_OFF);
        }

        usleep(10000); //10ms 100
        // re_conn++;
    }
}
