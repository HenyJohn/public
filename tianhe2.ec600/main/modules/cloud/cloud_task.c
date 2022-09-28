#include "cloud_task.h"
#include "data_process.h"

#include "app_mqtt.h"
#include "stdint.h"
#include "stdio.h"
#include "string.h"
#include "mqueue.h"
#include <unistd.h>
#include "stdbool.h"

/*this thread is mqtt connect and upload*/
extern void *pclient;
extern int update_inverter;
// extern int ap_enable;

static int last_mqtt_fail_sec = 0;
char g_imei[20] = {0};
char g_imsi[20] = {0};
int g_ate_net_status = -1;

static void datacall_reconn_check(void)
{
    static int reboot_sec = 0;
    static int last_sec = 0;
    int now_sec = 0;
    now_sec = get_second_sys_time();
    if (now_sec - last_sec > 60)
    {
        last_sec = now_sec;
        if (cat1_get_tcp_status() == false)
        {
            // printf("MEM left: %lu\n", esp_get_free_heap_size());
            check_data_call_status(); //fresh
            cat1_tcp_state_check();   //fresh
            if (cat1_get_data_call_status() == false)
            {
                data_call(""); //ctnet
            }
            else
            {
                reboot_sec = now_sec;
            }
        }
        else
        {
            reboot_sec = now_sec;
        }
    }

    if (now_sec - reboot_sec > 30 * 60) //30min
    {
        printf("reboot_sec=%d\n", reboot_sec);
        int res = cat1_hard_reboot(); //reboot
        if (res == 0)
        {
            esp_restart();
        }
        else
        {
            sleep(3);
        }
    }
}

extern char g_pmu_sn[25];
extern char g_inv_sn[25];
extern Inv_arr_t cgi_inv_arr;
extern InvRegister g_inv_info;
void cat1_cloud_task(void)
{
    char is_inv_sn_lost = 0;
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
        res = data_call(""); //ctnet
        sleep(1);
    }

    g_ate_net_status = ate_check_net();

    /** 如果PSN为空*/
    while (get_device_sn(g_pmu_sn))
    {
        sleep(1);
    }

    aliyun_para_check();

    while (strlen(cgi_inv_arr[0].invdata.psn) <= 0)
    {
        reboot_30min_check();
        sleep(1);
        if (get_second_sys_time() > 120)
        {
            is_inv_sn_lost = 1;
            break;
        }
    }

    while (1)
    {
        reboot_30min_check();
        if (strlen(g_inv_info.msw_ver) > 0)
            break;
        sleep(1);
    }

    memset(g_inv_sn, 0, sizeof(g_inv_sn));
    memcpy(g_inv_sn, cgi_inv_arr[0].invdata.psn, 24);

    printf("ini: g_pmu_sn:%s\n", g_pmu_sn);
    printf("ini: g_inv_sn:%s\n", g_inv_sn);

    cloud_setup();

    tianhe_ini();
    is_in_cloud_main_loop = 1;
    while (1)
    {
        if (is_inv_sn_lost == 1)
        {
            active_idx = 1;
        }
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
                tianhe_keepalive_sec();
                server_request_handler();
            }
            break;

        case 1:
#if 1
            res = cat1_mqtt_recv();
            // uptime_print("EEE-555");
            aliyun_fdbg_res_pub(res);
            recive_invdata_cst(&get_new_invdata_cst, &inv_data, &lost_data_flag_cst); //recv, save
            custom_mqtt_task(get_new_invdata_cst, inv_data, &lost_data_flag_cst);     //pub
#endif
            break;

        default:
            break;
        }
        cat1_fresh();
        datacall_reconn_check();
        usleep(50 * 1000);
        active_idx = !active_idx; /** take turn run tianhe, aliyun mqtts*/
        // active_idx = 0;           /** only tianhe mqtt works */
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

#include "inv_com.h"
char g_lac[4] = {0};
char g_ci[4] = {0};
char g_ccid[22] = {0};
char g_oper[30] = {0};
int g_4g_mode = 0;
int g_lac_len = 0;
int g_ci_len = 0;
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

    cat1_fast_reboot();
    res = cat1_echo_disable();
    while (res != 0)
    {
        reboot_30min_check();
        set_cat1_pwr_gpio(1);
        sleep(1);
        set_cat1_pwr_gpio(0);
        sleep(1);
        set_cat1_pwr_gpio(1);
        sleep(8);
        res = cat1_echo_disable();
    }

    res = -1;
    while (res != 0)
    {
        reboot_30min_check();
        res = cat1_get_ccid(g_ccid);
        sleep(1);
    }
    printf("CCID: %s\n", g_ccid);
    cat1_get_imei(g_imei);
    cat1_get_imsi(g_imsi);
    while (1)
    {
        reboot_30min_check();
        cat1_set_greg();
        res = cat1_get_lac_ci(g_lac, g_ci, &g_lac_len, &g_ci_len);
        if (res == 0)
        {
            int i = 0;
            printf("lac byte:\n");
            for (i = 0; i < 4; i++)
            {
                printf("%02X ", (uint8_t)g_lac[i]);
            }
            printf("ci byte:\n");
            for (i = 0; i < 4; i++)
            {
                printf("%02X ", (uint8_t)g_ci[i]);
            }
            break;
        }
        else
        {
            printf("cat1_get_lac_ci: fail");
        }

        sleep(1);
    }

    /** 云端任务*/
    cat1_cloud_task();
}
