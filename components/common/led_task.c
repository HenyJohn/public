/* GPIO Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
    CONDITIONS OF ANY KIND, either express or implied.
*/
#include "led_task.h"
#include "kaco_tcp_server.h"

#define OPEN_AP_AFTER_STA_FAIL_SECOND 10 // 90
static const char *TAG = "led_task.c";
int g_ap_enable = 0;

#define BLUE_LED_GPIO 14 // 12// 34blue [    lan]change
#define GREEN_LED_GPIO 4 // green
#define GPIO_OUTPUT_PIN_SEL ((1ULL << BLUE_LED_GPIO) | (1ULL << GREEN_LED_GPIO))

#define SLOW_BLINK_PERIOD_MS 1000
#define FAST_BLINK_PERIOD_MS 200
uint32_t net_disconnect_time = 0;
// int g_ap_enable=0;

enum Light_Type
{
    LIGHT_OFF,
    LIGHT_SLOW_BLINK,
    LIGHT_FAST_BLINK,
    LIGHT_ON
};
//****************************/
int check_inv_states();

//--------------------------------//
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
//     mark  拆分led初始化
void led_gpio_init()
{
    gpio_config_t io_conf;
    // disable interrupt
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    // set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    // bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
    // disable pull-down mode
    io_conf.pull_down_en = 0;
    // disable pull-up mode
    io_conf.pull_up_en = 1;
    // configure GPIO with the given settings
    gpio_config(&io_conf);

    fresh_led(GREEN_LED_GPIO, LIGHT_OFF);
    fresh_led(BLUE_LED_GPIO, LIGHT_OFF);
}

static uint8_t test_Blue_State_flag = 0;
uint8_t g_stick_run_mode_stat_bak = Work_Mode_IDLE;
uint8_t old_stick_run_mode = Work_Mode_IDLE;
static int re_conn = 0;

//-------kaco-lanstick 20220803 ---------------------------
static void net_disconnect_led_handle(bool is_kaco_tcp)
{
    if (!net_disconnect_time)
        net_disconnect_time = get_second_sys_time();

    if ((get_second_sys_time() - net_disconnect_time) > 6)//6s
    {
        if (test_Blue_State_flag != 2)
        {
            test_Blue_State_flag = 2;
            ASW_LOGW("LED Blue OFF , mqtt & net is disconnected!!");
            net_disconnect_time = get_second_sys_time();

            fresh_led(BLUE_LED_GPIO, LIGHT_OFF);
        }

        if (g_stick_run_mode != Work_Mode_AP_PROV && g_stick_run_mode != Work_Mode_LAN /* v2.0.0 add */
            && net_disconnect_time)
        {
            ESP_LOGW(TAG, "sta mode:  set mode wifi ap config");

            // stop_http_server();
            // asw_mqtt_client_stop();

            // g_ap_enable = 1;

            g_stick_run_mode_stat_bak = g_stick_run_mode; // 保存更改之前状态;

            g_stick_run_mode = Work_Mode_AP_PROV;

            net_disconnect_time = get_second_sys_time();

            asw_wifi_manager(WIFI_JOB_SCAN);
            sleep(2);
            asw_wifi_manager(WIFI_JOB_AP_SCAN);

            usleep(100);

            network_log(",[ok] ap on");
        }
    }
    //重连 3s间隔
    if (g_stick_run_mode == Work_Mode_STA && re_conn > 300)
    {
        check_wifi_reconnect();
        re_conn = 0;
        printf("wifi_reconnect 3s \n");
    }
    // AP 模式下 30min 连不上 重启
    if (g_stick_run_mode == Work_Mode_AP_PROV && (get_second_sys_time() - net_disconnect_time) > 1800)
    {
        printf("scan sta ap \n");
        if (find_now_apname() == 0)
        {
            sleep(1);

            printf(" stick restart by no net for 30min in AP mode \n");

            sent_newmsg();
        }

        net_disconnect_time = get_second_sys_time();
    }
}

static void net_recover_led_handle(bool is_kaco_tcp)
{
    net_disconnect_time = get_second_sys_time();

    // v2.0.0 add 恢复状态
    if (g_stick_run_mode == Work_Mode_AP_PROV && g_stick_run_mode_stat_bak != Work_Mode_IDLE)
    {
        g_stick_run_mode = g_stick_run_mode_stat_bak;
    }

    if (test_Blue_State_flag != 3)
    {
        test_Blue_State_flag = 3;
        ASW_LOGW("LED Blue ON , mqtt & net both connected!!");

        fresh_led(BLUE_LED_GPIO, LIGHT_ON);

        net_disconnect_time = get_second_sys_time();
    }
}

//--------------------------------//
void led_task(void *pvParameters)
{

    int sts = 0;

    uint32_t kaco_tcp_disconnect_time = 0;

    // int ap_disconnect_time = 0;

    g_state_mqtt_connect = -1; //[    add] for init blue led state

    fresh_led(GREEN_LED_GPIO, LIGHT_OFF);
    fresh_led(BLUE_LED_GPIO, LIGHT_OFF);

    ESP_LOGW(TAG, "Led task is begined !!");

    while (1)
    {

        /** 逆变器：绿灯*/
        sts = check_inv_states();
        /** 全部逆变器通信中断 */
        if (sts == -1)
        {
            fresh_led(GREEN_LED_GPIO, LIGHT_OFF);
            // ASW_LOGW("LED GREEN OFF ");
        }

        /** 部分逆变器通信中断 */
        else if (sts == 0)
        {
            fresh_led(GREEN_LED_GPIO, LIGHT_FAST_BLINK);
            // ASW_LOGW("LED GREEN BLink , inv connected is error!!");
        }
        /** 全部逆变器通信正常 */
        else if (sts == 1)
        {
            fresh_led(GREEN_LED_GPIO, LIGHT_ON);
            // ASW_LOGW("LED GREEN ON ,inv connected is OK!!");
        }

        /*mqtt 连接失败，30s 后蓝灯灭*/

        if (g_kaco_run_mode == 1)
        {
            if (g_state_mqtt_connect == -1) //蓝灯
            {
                if ((g_stick_run_mode == Work_Mode_LAN && get_eth_connect_status() == 0) || (g_stick_run_mode == Work_Mode_STA && get_wifi_sta_connect_status() == 0)) //
                {
                    fresh_led(BLUE_LED_GPIO, LIGHT_FAST_BLINK);
                    net_disconnect_time = get_second_sys_time();
                    // ASW_LOGW("LED Blue BLink , mqtt is disconnected & eth is connected!!");

                    if (test_Blue_State_flag != 1)
                    {
                        test_Blue_State_flag = 1;

                        ASW_LOGW("LED Blue BLink , mqtt is disconnected & net is connected!!");
                    }
                }
                /* 没有连接到外网，同时lan口没有获取到ip，wlan也没有连接到路由器的状态下，开启ap*/
                else /* if ((get_second_sys_time() - net_disconnect_time) > 15)*/
                {
                    net_disconnect_led_handle(0);
                }
            }

            else
            {
                net_recover_led_handle(0);
            }
        }
        else // kaco-lanstick 20220802 + tcp servet
        {
            if (kaco_tcp_disconnect_time == 0)
                kaco_tcp_disconnect_time = get_second_sys_time();

            tcp_damon(&kaco_tcp_disconnect_time);

            if (g_tcp_conn_state == 1) //异常
            {
                net_recover_led_handle(1);
            }
            else //正常
            {
                net_disconnect_led_handle(1);
                kaco_tcp_disconnect_time = get_second_sys_time();
            }
        }

        //检测到获取ip后,开启lan+ap配网模式  v2.0.0 add
        if (get_eth_connect_status() == 0 && g_stick_run_mode != Work_Mode_LAN)
        {
            printf("\n===LED TASK:   run mode change to Lan \n");
            g_stick_run_mode = Work_Mode_LAN; // Lan 模式
            asw_wifi_manager(WIFI_JOB_SCAN);
            sleep(1);
            asw_wifi_manager(WIFI_JOB_AP_SCAN);
        }

        if (old_stick_run_mode != g_stick_run_mode)
        {
            ESP_LOGW("Stick Run Mode", "- %d -[0:IDLE,1:AP-Prov,2:WIFI-STA,3:LAN]", g_stick_run_mode);
            old_stick_run_mode = g_stick_run_mode;
        }

        usleep(10000); // 10ms 100
        re_conn++;
    }
}
