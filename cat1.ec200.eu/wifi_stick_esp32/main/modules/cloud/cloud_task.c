#include "cloud_task.h"
#include "data_process.h"

#include "app_mqtt.h"
#include "stdint.h"
#include "stdio.h"
#include "string.h"
#include "mqueue.h"
#include <unistd.h>
#include "stdbool.h"
#include "inv_com.h"
#include "data_process.h"

/*this thread is mqtt connect and upload*/
extern void *pclient;
extern int update_inverter;
// extern int ap_enable;

static int reboot_sec = 0;
static int last_sec = 0;
static int last_mqtt_fail_sec = 0;
char g_imei[20] = {0};
char g_imsi[20] = {0};

static void datacall_reconn_check(void)
{
    int now_sec = 0;
    now_sec = get_second_sys_time();
    if (now_sec - last_sec > 60)
    {
        if (cat1_get_mqtt_status(0) == false)
        {
            printf("MEM left: %lu\n", esp_get_free_heap_size());
            check_data_call_status(); //fresh
            if (cat1_get_data_call_status() == false)
            {
                data_call();
            }
            else
            {
                reboot_sec = get_second_sys_time();
            }
        }
        else
        {
            reboot_sec = get_second_sys_time();
        }

        last_sec = now_sec;
    }

    now_sec = get_second_sys_time();
    if (now_sec - reboot_sec > 30 * 60) //30min
    {
        printf("reboot_sec=%d\n", reboot_sec);
        int res = cat1_hard_reboot(); //reboot
        if (res == 0)
        {
            reboot_sec = get_second_sys_time();
        }
        else
        {
            sleep(3);
        }
    }
}

void cat1_cloud_task(void)
{
    TDCEvent mqtt_state = dcIdle;
    int lost_data_flag = 0;
    int get_new_invdata = 0;
    Inv_data inv_data = {0};
#if 1
    int lost_data_flag_cst = 0;
    int get_new_invdata_cst = 0;
#endif
    int res = -1;
    int active_idx = 0;
    //printf("cloud task start ... kkjjll\n");
    /** 关AT指令回显*/
    cat1_echo_disable();

    /** 初始化拨号1次*/
    res = -1;
    while (res != 0)
    {
        reboot_30min_check();
        res = data_call();
        sleep(1);
    }
    /** 如果SN为空*/
    while (get_device_sn())
    {
        sleep(1);
    }

    cloud_setup();
    is_in_cloud_main_loop = 1;
    while (1)
    {
        switch (active_idx)
        {
        case 0:
            if (!update_inverter)
            {
                if (xSemaphoreTake(invdata_store_sema, portMAX_DELAY) == pdTRUE)
                {
                    res = recive_invdata(&get_new_invdata, &inv_data, &lost_data_flag);
                    xSemaphoreGive(invdata_store_sema);
                }
                if (res == 777) /** 消息队列接收到数据，且MQTT正常，则立即pub，不要被conn打断导致连续receive覆盖丢失*/
                    mqtt_state = dcalilyun_publish;
                switch (mqtt_state)
                {
                case dcIdle:
                    mqtt_state = get_net_state(); /* 检查拨号连接状态*/
                    break;
                case dcalilyun_authenticate:
                    mqtt_state = mqtt_connect();
                    break;
                case dcalilyun_publish:
                    mqtt_state = mqtt_publish(get_new_invdata, inv_data, &lost_data_flag); //1 sec spend
                    break;
                default:
                    break;
                }
                res = -1;
                res = cat1_mqtt_recv();
                fdbg_pub_rrpc(res);
                // recive_invdata(&get_new_invdata, &inv_data, &lost_data_flag);
                // printf("get_new_invdata=%d, lost_data_flag=%d ***************************\n", get_new_invdata, lost_data_flag);
                datacall_reconn_check();
            }
            break;

        case 1:
#if 1
            recive_invdata_cst(&get_new_invdata_cst, &inv_data, &lost_data_flag_cst); //recv, save
            custom_mqtt_task(get_new_invdata_cst, inv_data, &lost_data_flag_cst);     //pub
#endif
            break;

        default:
            break;
        }
        some_refresh();
        usleep(50 * 1000);
        active_idx = !active_idx; /* take turn between two mqtts*/
        active_idx = 0;           //only aswei mqtt works
        continue;
    }
}

/** CAT1 task ****************************************************************************************
 * 1.MQTT cloud
 * 2.ATE
 * 
*/
void pull_low(void)
{
    set_cat1_pwr_gpio(0);
}
void pull_high(void)
{
    set_cat1_pwr_gpio(1);
}

int cat1_hard_reboot(void)
{
    //prior use soft reboot
    if (cat1_echo_disable() == 0)
    {
        if (cat1_reboot() == 0)
        {
            return 0;
        }
    }
    //hard reboot, if soft reboot fail
    int cnt = 0;
    int dd = -1;
    while (1)
    {
        cnt++;
        if (cnt > 10)
            return -1;
        pull_high();
        sleep(5);
        pull_low();
        sleep(20);
        dd = cat1_echo_disable();
        if (dd != 0)
        {
            printf("OFFF: %d ------------\n", dd);
            break;
        }
    }

    cnt = 0;
    while (1)
    {
        cnt++;
        if (cnt > 10)
            return -1;
        pull_low();
        sleep(5);
        pull_high();
        sleep(20);
        dd = cat1_echo_disable();
        if (dd == 0)
        {
            printf("ONNN: %d ------------\n", dd);
            return 0;
        }
    }
}

char g_ccid[22] = {0};
char g_oper[30] = {0};
int g_4g_mode = 0;
void cloud_task(void *pvParameters)
{
    int res = -1;
    set_print_blue();

    /** 初始化串口*/
    if (serialport_init(CAT1_UART) == 0)
    {
        printf("cat1 uart ini succ\r\n");
    }
    else
    {
        printf("cat1 uart ini fail\r\n");
    }

    // sleep(3600 * 8);
    cat1_fast_reboot();
    res = cat1_echo_disable();
    while (res != 0)
    {
        reboot_30min_check();
        set_cat1_pwr_gpio(1);
        sleep(1);
        set_cat1_pwr_gpio(0);
        sleep(2); //1
        set_cat1_pwr_gpio(1);
        sleep(5); //5
        res = cat1_echo_disable();
    }

    cat1_fs_ls();

    res = -1;
    while (res != 0)
    {
        reboot_30min_check();
        res = cat1_get_ccid(g_ccid);
        cat1_gps_read();
        sleep(1);
    }
    printf("CCID: %s\n", g_ccid);
    cat1_get_imei(g_imei);
    cat1_get_imsi(g_imsi);

    // vTaskSuspend(NULL);

    /** 云端任务*/
    cat1_cloud_task();
}
