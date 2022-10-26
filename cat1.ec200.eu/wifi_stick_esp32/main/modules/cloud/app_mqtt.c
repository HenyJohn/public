#include "app_mqtt.h"
#include "aiot_mqtt_sign.h"
// #include "mqtt_api.h"
#include "time.h"
#include "data_process.h"
#include "asw_mqtt.h"
#include "device_data.h"

// #include "asw_free.h"
#include "pthread.h"
// #include "led_app.h"

#include "inv_com.h"
#include "utility.h"
#include "mqueue.h"
#include "meter.h"
#include "asw_store.h"

extern pthread_mutex_t sqlite_mutex;
Setting_Para ini_para = {0};
SYNC_STATE sync_state = CLOUD_SYNC_MONITOR;
// extern iotx_mqtt_param_t mqtt_params;
char inv_info_sync[30] = {0};

extern void *pclient;

extern char DEMO_PRODUCT_KEY[IOTX_PRODUCT_KEY_LEN + 1];
extern char DEMO_DEVICE_NAME[IOTX_DEVICE_NAME_LEN + 1];
extern char DEMO_DEVICE_SECRET[IOTX_DEVICE_SECRET_LEN + 1];
extern Inv_arr_t inv_arr;
extern int inv_real_num;

extern int8_t monitor_state;

static unsigned short pcout = 3;
extern int netework_state;
extern Inv_arr_t inv_dev_info;
extern meter_setdata mtconfig;
extern char pub_topic[100];
extern char sub_topic[100];
extern char rrpc_res_topic[256];
extern char g_mqtt_pdk[128];

int clear_cloud_invinfo(void)
{
    memset(inv_info_sync, 0, sizeof(inv_info_sync));
    return 0;
}

int GetMinute1(int ihour, int iminute, int imsec)
{
    uint32_t dSeconds = (ihour * 60 + iminute + imsec);
    return (dSeconds);
}
// void Check_upload_unit32(const char *field, uint8_t value, char *body, uint16_t *datalen)
// {
//     uint16_t len = (*datalen);
//     if (value != 0xFFFFFFFF)
//     {
//         (*datalen) = len + sprintf(body + len, field, value);
//     }
// }
// void Check_upload_sint16(const char *field, uint16_t value, char *body, uint16_t *datalen)
// {
//     uint16_t len = (*datalen);
//     if (value != 0xFFFF)
//     {
//         (*datalen) = len + sprintf(body + len, field, value);
//     }
// }

/*this function generate the login key*/

void sn_to_reg(const char *sn, char *reg_key)
{
    uint32_t iLoop;

    if (NULL == sn)
    {
        return;
    }

    for (iLoop = 0; iLoop < 12; iLoop++)
    {
        reg_key[iLoop] = ((~(sn[iLoop] | 0xf0)) + 0x40);
    }
    for (iLoop = 12; iLoop < 16; iLoop++)
    {
        reg_key[iLoop] = (reg_key[iLoop - 10] & reg_key[iLoop - 4]);
    }
}

/*get time by zeversolar*/
TDCEvent gettime(void)
{
    struct timeval tv = {0};
    struct tm ptime = {0};
    static int last_time_ret = -5; //-5 means ini
    static int64_t cnnot_get_time = 0;
    static int fail_cnt = 0;

    Monitor monitor;

    char time_url[200] = {0};
    char reg_key[17] = {0};
    unsigned char ts[128] = {0};
    // G123456789AB
    // sn_to_reg(monitor.sn, reg_key);
    sn_to_reg(DEMO_DEVICE_NAME, reg_key);

    sprintf(time_url, "http://mqtt.aisweicloud.com/api/v1/updatetime?psno=%s&sign=%s", DEMO_DEVICE_NAME, reg_key);
    printf("timeurl %s\n", time_url);
    int ret = cat1_http_get_time(time_url, &ptime);

    if (ret != 0)
    {
        fail_cnt++;
        if (fail_cnt < 10)
        {
            sleep(1);
        }
        else
        {
            sleep(60);
        }

        if ((last_time_ret == 0 || last_time_ret == -5) && ret != 0)
        {
            network_log(",[err] get time");
            last_time_ret = ret;
        }

        if (!cnnot_get_time)
            cnnot_get_time = get_second_sys_time();
        //printf("cnnot get over 30mins reboot ooppp %lld \n", cnnot_get_time);
        if (get_second_sys_time() - cnnot_get_time > 1800)
        {
            printf("cnnot get over 30mins reboot \n");
            sleep(1);
            sent_newmsg();
        }

        printf("gettime error\r\n");
        return dcIdle;
    }
    else if (ptime.tm_year > 2019 && ptime.tm_year < 2050)
    {
        fail_cnt = 0;
        ptime.tm_year -= 1900;
        ptime.tm_mon -= 1;
        printf("[year] %d\n", ptime.tm_year);
        printf("[mon] %d\n", ptime.tm_mon);
        printf("[day] %d\n", ptime.tm_mday);
        printf("[hour] %d\n", ptime.tm_hour);
        printf("[min] %d\n", ptime.tm_min);
        printf("[sec] %d\n", ptime.tm_sec);

        tv.tv_sec = mktime(&ptime);
        settimeofday(&tv, NULL);
        //system("hwclock -w");
        //system("sync");
        sprintf(ts, "date %02d%02d%02d%02d%d", (ptime.tm_mon + 1), ptime.tm_mday, ptime.tm_hour, ptime.tm_min, (ptime.tm_year + 1900));
        printf("set time %s \n", ts);
        system(ts);
        system("sync");

        //display time we have set
        char display[20] = {0};
        get_time(display, 20);
        printf("system time: %s\n", display);
        // pthread_mutex_lock(&monitor_mutex);
        monitor_state |= SYNC_TIME_FROM_CLOUD_INDEX;
        // pthread_mutex_unlock(&monitor_mutex);
    }
    //printf("write time log %d \n", ((last_time_ret != 0 || last_time_ret == -5) && ret == 0 && ptime.tm_year > 2019 && ptime.tm_year < 2050));
    //printf("write ######### %d %d %d \n", last_time_ret, ret, ptime.tm_year);

    if ((last_time_ret != 0 || last_time_ret == -5) && ret == 0 && ptime.tm_year > 119 && ptime.tm_year < 150)
    {
        network_log(",[ok] get time");
        last_time_ret = ret;
    }
    return dcalilyun_authenticate;
}

int get_time_manual(void)
{
    struct timeval tv;
    struct tm ptime;

    char time_url[200] = {0};
    char reg_key[17] = {0};
    unsigned char ts[128] = {0};
    //G123456789AB
    // sn_to_reg(monitor.sn, reg_key);
    sn_to_reg(DEMO_DEVICE_NAME, reg_key);

    sprintf(time_url, "http://mqtt.aisweicloud.com/api/v1/updatetime?psno=%s&sign=%s", DEMO_DEVICE_NAME, reg_key);
    printf("[ url ] :\n%s\n", time_url);
    int ret = http_get_time(time_url, &ptime);
    if (ret != 0)
    {
        printf("gettime error\r\n");
        return -1;
    }
    else
    {
        ptime.tm_year -= 1900;
        ptime.tm_mon -= 1;

        printf("[year] %d\n", ptime.tm_year);
        printf("[mon] %d\n", ptime.tm_mon);
        printf("[day] %d\n", ptime.tm_mday);
        printf("[hour] %d\n", ptime.tm_hour);
        printf("[min] %d\n", ptime.tm_min);
        printf("[sec] %d\n", ptime.tm_sec);

        tv.tv_sec = mktime(&ptime);
        settimeofday(&tv, NULL);
        //system("hwclock -w");
        //system("sync");
        sprintf(ts, "date %02d%02d%02d%02d%d", (ptime.tm_mon + 1), ptime.tm_mday, ptime.tm_hour, ptime.tm_min, (ptime.tm_year + 1900));
        printf("set time %s \n", ts);
        system(ts);
        system("sync");
        return 0;
    }
}

/*get net state*/
int get_net_state(void)
{
    int ret = check_data_call_status();
    if (ret == 0)
    {
        //return (dcalilyun_authenticate);//gettime());
        return (gettime());
    }
    else
    {
        //printf("wifi not conn\n");
        return dcIdle;
    }
}

int cloud_setup(void)
{
    Setting_Para para = {0};
    general_query(NVS_ATE, &para);
    //g_mqtt_pdk
    //snprintf(my_uri, sizeof(my_uri), "AT+QMTOPEN=%d,\"%s.iot-as-mqtt.%s.aliyuncs.com\",%d", 0,pdk, server, port);
    memcpy(g_mqtt_pdk, para.product_key, strlen(para.product_key));
    set_mqtt_server(para.product_key, para.host, para.port);
    set_product_key(para.product_key);
    set_device_name(para.psn);
    set_device_secret(para.device_secret);

    //snprintf(outbuf, sizeof(outbuf), "%d", *adder);
    snprintf(pub_topic, 100, "/%s/%s/user/Action", para.product_key, para.psn);
    //snprintf(sub_topic, 100, "/a1tqX123itk/%s/user/get", para.psn);"/sys/a1tqX123itk/B80052030075/rrpc/request/+";
    snprintf(sub_topic, 100, "/sys/%s/%s/rrpc/request/+", para.product_key, para.psn);

    int res = asw_mqtt_setup();
    if (res == 0)
    {
        printf("[ ok ] mqtt connect >>>>>>>\n");
    }
    else
    {
        printf("[ err ] mqtt connect >>>>>>>\n");
    }

    return 0;
}

/*mqtt connect*/
int mqtt_connect(void)
{
    static int fail_cnt = 0;
    cat1_echo_disable();
    if (cat1_get_data_call_status() == 0)
        return dcalilyun_authenticate;

    if (0 == mqtt_app_start())
    {
        fail_cnt = 0;
        return dcalilyun_publish;
    }
    else
    {
        fail_cnt++;
        if (fail_cnt < 10)
        {
            sleep(1);
        }
        else
        {
            sleep(60);
        }
        return dcalilyun_authenticate;
    }
}
int check_sn_format(char *str)
{
    char i = 0, n = 0;
    while (str[i])
    {
        if (isalnum(str[i]))
            n++;
        i++;
    }
    printf("There are %d nums or characters in str\n", n);
    return n;
}

extern cloud_inv_msg trans_data;
int trans_resp_pub(cloud_inv_msg *resp, unsigned char *ws)
{
    char payload[512] = {0};
    char _data[512] = {0};
    HexToStr(_data, resp->data, resp->len);
    //snprintf(payload,1024,"{\"Action\":\"Full-TransResp\",\"ws\":\"%d\",\"data\":\"%s\"}",
    //resp->ws,_data);
    snprintf(payload, 1024, "{\"Action\":\"Full-TransResp\",\"ws\":\"%s\",\"data\":\"%s\"}",
             ws, _data);
    if (0 != asw_publish(payload))
    {
        printf("[err] publish trans resp\n");
        // write_to_log("[err] publish trans resp faild");
        return -1;
    }
    return 0;
}

static cld_rrpc_resp_t cld_resp_var = {0};
static int rrpc_resp_concel = 0;
int trans_resrrpc_pub(cloud_inv_msg *resp, unsigned char *ws, int len)
{
    char payload[1024] = {0};
    char _data[512] = {0};
    if (len > 0 && len < 255)
    {
        HexToStr(_data, resp->data, resp->len);
        snprintf(payload, 1024, "{\"dat\":\"ok\",\"data\":\"%s\"}", _data);
    }
    else
        snprintf(payload, 1024, "{\"dat\":\"err\",\"msg\":\"%s\"}", "no response");
    //memcpy(_data, "err", strlen("err"));

    //snprintf(payload,1024,"{\"Action\":\"Full-TransResp\",\"ws\":\"%d\",\"data\":\"%s\"}",
    //resp->ws,_data);
    //snprintf(payload, 1024, "{\"Action\":\"Full-TransResp\",\"ws\":\"%s\",\"data\":\"%s\"}",
    //         ws, _data);"{\"dat\":\"ok\"}";

    memset(&cld_resp_var, 0, sizeof(cld_resp_var));
    memcpy(cld_resp_var.topic, rrpc_res_topic, strlen(rrpc_res_topic));
    memcpy(cld_resp_var.payload, payload, strlen(payload));
    if (rrpc_resp_concel == 0)
    {
        cld_resp_var.active = 1;
    }
    else
    {
        cld_resp_var.active = 0;
        rrpc_resp_concel = 0;
    }

    // if (0 != asw_mqtt_publish(rrpc_res_topic, (uint8_t *)payload, strlen(payload), 0)) //res scan(payload))
    // {
    //     printf("[err] publish trans resp\n");
    //     // write_to_log("[err] publish trans resp faild");
    //     return -1;
    // }
    memset(rrpc_res_topic, 0, 256);
    return 0;
}

void fdbg_pub_rrpc(int recv_res)
{
    int cnt = 0;
    if (recv_res == 222)
    {
        if (cat1_get_mqtt_status(0) == 1)
        {
            while (cld_resp_var.active != 1)
            {
                usleep(50000);
                cnt++;
                if (cnt > 100) // 5 sec timeout
                {
                    rrpc_resp_concel = 1;
                    return;
                }
            }
            if (0 != asw_mqtt_publish(cld_resp_var.topic, (uint8_t *)cld_resp_var.payload, strlen(cld_resp_var.payload), 0)) //res scan(payload))
            {
                printf("[err] publish trans resp\n");
            }
            cld_resp_var.active = 0;
        }
    }
}

extern char g_ccid[22];
extern char g_oper[30];
extern char g_imei[20];
extern char g_imsi[20];
extern int g_4g_mode;

int mqtt_publish(int flag, Inv_data invdata, int *lost_flag)
{
    char payload[1000] = {0};
    Setting_Para monitor;
    InvRegister devicedata = {0};
    int ret;
    static int ncout, inv_sync_num = 0;
    static int publish_fail = 0;
    int64_t publish_fail_time = 0;
    static int inv_com_fail = 0, inv_com_time = 0;
    static int meter_sync = 0;

    int csq = 8;   //get_signal_csq();
    int mode = 4;  //get_network_mode();
    int state = 0; //get_network_state();
    char namex[5] = {0};
    char iccid[5] = {0};
    char imsi[5] = {0};
    char msisdn[5] = {0};
    char imei[5] = {0};

    char brand[16] = {0};
    char name[16] = {0};
    static int last_mqtt_time = 0;
    static int last_mqtt_state = 0;

    // printf("AAAE: 001: mqtt_publish------------\n");

    if (cat1_get_data_call_status() == 0)
    {
        return dcalilyun_publish;
    }

    int now_mqtt_sate = cat1_get_mqtt_status(0);
    if (last_mqtt_state == 1 && now_mqtt_sate == 0)
    {
        last_mqtt_time = get_second_sys_time();
    }
    last_mqtt_state = now_mqtt_sate;
    // printf("+++  %d %d ++++++++++++++++++++++++++++++++++\n", now_mqtt_sate, cat1_get_mqtt_status(0));
    if (now_mqtt_sate == 0)
    {
        if (get_second_sys_time() - last_mqtt_time > 60)
        {
            last_mqtt_time = get_second_sys_time();
            return dcalilyun_authenticate;
        }
        else
        {
            return dcalilyun_publish;
        }
    }

    memset(&monitor, 0, sizeof(monitor));
    /*recv message from subscribe*/
    if (0 == (monitor_state & INV_SYNC_PMU_STATE))
    {
        memset(g_oper, 0, sizeof(g_4g_mode));
        g_4g_mode = 0;
        cat1_get_oper_and_mode(g_oper, &g_4g_mode);
        if (g_4g_mode == 7)
            g_4g_mode = 4;
        else
        {
            g_4g_mode = 2;
        }
        general_query(NVS_ATE, &ini_para);
        char ap[30] = {0};
        char psw[50] = {0};
        // char apn[50] = {0};

        apn_para_t apn_para = {0};
        general_query(NVS_APN_PARA, &apn_para);
        get_ap_psw(ap, psw);
        sprintf(payload, "{\"Action\":\"SyncPmuInfo\","
                         "\"iccid\":\"%s\","
                         "\"imei\":\"%s\","
                         "\"imsi\":\"%s\","
                         "\"msisdn\":\"%s\","
                         "\"csq\":\"%d\","
                         "\"apn\":\"%s\","
                         "\"username\":\"%s\","
                         "\"password\":\"%s\","
                         "\"mode\":\"%d\","
                         "\"sn\":\"%s\","
                         "\"typ\":%d,"
                         "\"nam\":\"%s\","
                         "\"sw\":\"%s\","
                         "\"hw\":\"%s\","
                         "\"sys\":\"%s\","
                         "\"md1\":\"%s\","
                         "\"md2\":\"%s\","
                         "\"mod\":\"%s\","
                         "\"muf\":\" %s\","
                         "\"brd\":\"%s\","
                         "\"ap\":\"%s\","
                         "\"psw\":\"%s\","
                         "\"state\":\"%d\","
                         "\"name\":\"%s\","
                         "\"cmd\":%d}",
                g_ccid,
                g_imei,
                g_imsi,
                "",
                cat1_get_csq(),
                apn_para.apn,
                apn_para.username,
                apn_para.password,
                g_4g_mode,
                DEMO_DEVICE_NAME,
                ini_para.typ,
                ini_para.nam,
                FIRMWARE_REVISION,
                ini_para.hw,
                "freertos",
                ini_para.md1,
                CGI_VERSION, //ini_para.md2,//////////////////////////////V1.0
                ini_para.mod,
                ini_para.mfr,
                ini_para.brw,
                ap,
                psw,
                99,
                g_oper,
                5);
        printf("PUB MONITOR INFO %s **********************\n", payload);
        if (asw_publish(payload, pcout++) >= 0)
        {
            printf("[ asw_publish ] pmu info\n");
            monitor_state |= INV_SYNC_PMU_STATE;
        }
        else
        {
            // write_to_log("publish pmu info faild");
            // return dcalilyun_authenticate;
        }
    }
    memset(payload, 0, sizeof(payload));
    //memcpy(&devicedata, &(inv_arr[0].regInfo), sizeof(InvRegister));
    //printf("--%d---%d---%s---%s \n", inv_sync_num, inv_arr[inv_sync_num].regInfo.modbus_id, inv_arr[inv_sync_num].regInfo.sn, inv_arr[inv_sync_num].regInfo.mode_name);

    if ((monitor_state & INV_SYNC_PMU_STATE) && !inv_info_sync[29])
    {
        //printf("--%d---%d---%s---%s \n", inv_sync_num, inv_arr[inv_sync_num].regInfo.modbus_id, inv_arr[inv_sync_num].regInfo.sn, inv_arr[inv_sync_num].regInfo.mode_name);
        if (!inv_info_sync[inv_sync_num] && inv_arr[inv_sync_num].regInfo.modbus_id > 0 && strlen(inv_arr[inv_sync_num].regInfo.sn) > 0 && strlen(inv_arr[inv_sync_num].regInfo.msw_ver) > 0)
        {
            //memcpy(&devicedata, &(inv_arr[inv_sync_num].regInfo), sizeof(InvRegister));
            memcpy(&devicedata, &(inv_dev_info[inv_sync_num].regInfo), sizeof(InvRegister));
            //printf("inv sn %s invfo %s \n", inv_arr[inv_sync_num].regInfo.mode_name, devicedata.msw_ver);
            if (check_sn_format(devicedata.sn) > 0)
            {
                sprintf(payload, "{\"Action\":\"SyncInvInfo\","
                                 "\"psn\":\"%s\","
                                 "\"typ\":\"%d\","
                                 "\"sn\":\"%s\","
                                 "\"mod\":\"%s\","
                                 "\"sft\":\"%d\","
                                 "\"rtp\":\"%d\","
                                 "\"mst\":\"%s\","
                                 "\"slv\":\"%s\","
                                 "\"sfv\":\"%s\","
                                 "\"cmv\":\"%s\","
                                 "\"muf\":\"%s\","
                                 "\"brd\":\"%s\","
                                 "\"mty\":\"%d\","
                                 "\"MPT\":\"%d\","
                                 "\"str\":\"%d\","
                                 //  "\"phs\":\"%d\","
                                 "\"Imc\":\"%d\"}",
                        DEMO_DEVICE_NAME,
                        devicedata.type - 0x30,
                        devicedata.sn,
                        devicedata.mode_name,
                        devicedata.safety_type,
                        devicedata.rated_pwr,
                        devicedata.msw_ver,
                        devicedata.ssw_ver,
                        devicedata.tmc_ver,
                        devicedata.protocol_ver,
                        devicedata.mfr,
                        devicedata.brand,
                        devicedata.mach_type,
                        devicedata.pv_num,
                        devicedata.str_num,
                        // "machine type",
                        devicedata.modbus_id);
                printf("PUB  %s inv info**************\n", payload);
                if (asw_publish(payload, pcout++) > -1)
                {
                    //monitor_state |= INV_SYNC_INV_STATE;
                    // inv_sync_num++;
                    inv_info_sync[inv_sync_num] = 1;
                    printf("[ asw_publish ] %d inv info\n", inv_sync_num);
                    printf("invinfo sync: sn=%s msw=%s\n", devicedata.sn, devicedata.msw_ver);
                }
                else
                {
                    //write_to_log("publish inv info faild");
                    // return dcalilyun_authenticate;
                }
                memset(payload, 0, sizeof(payload));
            }
            else
            {
                inv_info_sync[inv_sync_num] = 1;
                printf("invinfo sync extra: sn=%s msw=%s\n", devicedata.sn, devicedata.msw_ver);
            }
        }
        inv_sync_num++;
        if ((inv_sync_num >= inv_real_num))
        {
            inv_sync_num = 0;
            char i = 0, j = 0; //j is count sn err num
            for (; i < inv_real_num; i++)
                if (inv_info_sync[i] == 1)
                {
                    //i++;
                    j++;
                    printf("rht %d %dinvter info upload\n", i, j);
                }

            if (j && j >= inv_real_num)
            {
                printf("all invter info upload\n");
                inv_info_sync[29] = 1;
            }
        }
    }
    memset(payload, 0, sizeof(payload));

    if (mtconfig.enb > 0 && meter_sync == 0)
    {
        memset(brand, 0, sizeof(brand));
        memset(name, 0, sizeof(name));
        get_meter_info(mtconfig.mod, brand, name);
        sprintf(payload, "{\"Action\":\"SyncMeterInfo\",\"psn\":\"%s\",\"sn\":\"%s%s\","
                         "\"enb\":\"%d\","
                         "\"exp_m\":\"%d\","
                         "\"regulate\":\"%d\","
                         "\"enb_PF\":\"%d\","
                         "\"target_PF\":\"%d\","
                         "\"abs\":\"%d\","
                         "\"offset\":\"%d\","
                         "\"typ\":\"%d\",\"mod\":\"%d\",\"nam\":\"%s\"}",
                DEMO_DEVICE_NAME, brand, DEMO_DEVICE_NAME,
                mtconfig.enb,
                mtconfig.target,
                mtconfig.reg > 0x99 ? 10 : 5,
                mtconfig.enb_pf,
                mtconfig.target_pf,
                mtconfig.reg > 0x100 ? 1 : 0,
                mtconfig.reg > 0x100 ? mtconfig.reg - 0x100 : 0,
                1, mtconfig.mod, name);
        if (asw_publish(payload) >= 0)
        {
            pcout++;
            meter_sync = 1;
        }
    }
    memset(payload, 0, sizeof(payload));

    if (mtconfig.enb > 0 && meter_sync == 1) //monitor.data &&
    {
        meter_cloud_data buf;

        int ret;
        char spid[32] = {0};
        memset(&buf, 0, sizeof(buf));

        if (xQueueReceive(mq2, &buf, (TickType_t)10) == pdPASS)
        {
            struct tm curr_date;
            time_t t = time(NULL);
            localtime_r(&t, &curr_date);
            curr_date.tm_year += 1900;
            curr_date.tm_mon += 1;

            memset(brand, 0, sizeof(brand));
            memset(name, 0, sizeof(name));
            get_meter_info(mtconfig.mod, brand, name);
            sprintf(payload, "{\"Action\":\"UpdateMeterData\",\"psn\":\"%s\",\"sn\":\"%s%s\","
                             "\"smp\": %d,"
                             "\"tim\":\"%04d-%02d-%02d %02d:%02d:%02d\","
                             "\"pow\": %d,\"ied\": %d,\"oed\":%d,"
                             "\"iet\":%d,\"oet\":%d,"
                             "\"itv\":%d,"
                             "\"rpw\":%d,\"apw\":%d,\"phs\":%d,\"pf\":%d}", //phs:phase angle
                    DEMO_DEVICE_NAME, brand, DEMO_DEVICE_NAME, 5,
                    curr_date.tm_year, curr_date.tm_mon, curr_date.tm_mday, curr_date.tm_hour, curr_date.tm_min, curr_date.tm_sec,
                    buf.data.pac, buf.data.con_stu, buf.data.e_today,
                    buf.data.e_total, buf.data.h_total,
                    GetMinute1(curr_date.tm_hour, curr_date.tm_min, 0),
                    buf.data.qac, buf.data.sac, buf.data.bus_voltg, buf.data.cosphi);
            if (asw_publish(payload) >= 0)
                pcout++;
        }
    }

    //Inv_data_msg_t inv_data_msg = {0};

    // int flag = 0;
    // int is_db = 0;

    // if (mq0 != NULL)
    // {
    //     if (xQueueReceive(mq0,
    //                       &invdata,
    //                       (TickType_t)10) == pdPASS)
    //     {
    //         flag = 1;
    //         is_db = 0;
    //     }
    //     else
    //     {
    //         if(inv_data_query(&invdata)>=0)
    //         {
    //             flag = 1;
    //             printf("invdata.time ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^%s \n" ,invdata.time);
    //             is_db = 1;
    //         }
    //     }
    // }

    if (*lost_flag == 1)
    {
        char buff[300] = {0};
        memcpy(buff, (char *)&invdata, sizeof(Inv_data));

        int i = 0;
        printf("recv lost data invdata %d crc=%d \n", sizeof(Inv_data), crc16_calc(buff, sizeof(Inv_data)));
        for (; i < sizeof(Inv_data); i++)
            printf("%02x ", buff[i]);
        printf("reacv lost end\n");
    }

    if (flag == 1)
    {
        // memcpy(&devicedata, &(inv_arr[0].regInfo), sizeof(InvRegister));
        // time_t t = time(NULL);
        // struct tm currtime;
        // localtime_r(&t, &currtime);
        // currtime.tm_year += 1900;
        // currtime.tm_mon += 1;
        memset(payload, 0, sizeof(payload));

        int16_t datlen = sprintf(payload, "{\"Action\":\"UpdateInvData\",\"psn\":\"%s\",\"sn\":\"%s\","
                                          "\"csq\":%d,"
                                          "\"mty\":%d,"
                                          "\"stu\":%d,"
                                          "\"smp\":5,\"tim\":\"%s\","
                                          "\"tmstp\":%d000,"
                                          "\"fac\":%d,\"pac\":%d,\"er\":%d,"
                                          "\"pf\":%d,\"cf\":%d,\"eto\":%d,\"etd\":%d,\"hto\":%d,",
                                 DEMO_DEVICE_NAME, invdata.psn,
                                 cat1_get_csq(),
                                 get_mach_type_by_psn(invdata.psn), //mty
                                 invdata.status,
                                 invdata.time,
                                 invdata.rtc_time,
                                 invdata.fac, invdata.pac, invdata.error,
                                 invdata.cosphi, invdata.col_temp, invdata.e_total, invdata.e_today, invdata.h_total);
        /* check 1~10 MPPT Parameters*/
        int i;
        for (i = 0; i < 10; i++)
        {

            if ((invdata.PV_cur_voltg[i].iVol != 0xFFFF) && (invdata.PV_cur_voltg[i].iCur != 0xFFFF))
            {
                datlen += sprintf(payload + datlen, "\"v%d\":%d,\"i%d\":%d,",
                                  i + 1, invdata.PV_cur_voltg[i].iVol,
                                  i + 1, invdata.PV_cur_voltg[i].iCur);
            }
        }
        /* check 1~3 ac parameters*/
        for (i = 0; i < 3; i++)
        {
            if ((invdata.AC_vol_cur[i].iVol != 0xFFFF) && (invdata.AC_vol_cur[i].iCur != 0xFFFF))
            {
                datlen += sprintf(payload + datlen, "\"va%d\":%d,\"ia%d\":%d,",
                                  i + 1, invdata.AC_vol_cur[i].iVol,
                                  i + 1, invdata.AC_vol_cur[i].iCur);
            }
        }
        /* check 1~20 string current parameters*/
        for (i = 0; i < 20; i++)
        {
            if ((invdata.istr[i] != 0xFFFF))
            {

                datlen += sprintf(payload + datlen, "\"s%d\":%d,", i + 1, invdata.istr[i]);
            }
        }
        //Check_upload_unit16
        Check_upload_unit32("\"prc\":%u,", invdata.qac, payload, &datlen);
        Check_upload_unit32("\"sac\":%u,", invdata.sac, payload, &datlen);
        Check_upload_sint16("\"tu\":%d,", invdata.U_temp, payload, &datlen);
        Check_upload_sint16("\"tv\":%d,", invdata.V_temp, payload, &datlen);
        Check_upload_sint16("\"tw\":%d,", invdata.W_temp, payload, &datlen);
        Check_upload_sint16("\"cb\":%d,", invdata.contrl_temp, payload, &datlen);
        Check_upload_sint16("\"bv\":%d,", invdata.bus_voltg, payload, &datlen);
        //printf("[ payload 5 ]\n%s\n", payload);
        /*check 1~10 warn*/
        for (i = 0; i < 10; i++)
        {
            if (invdata.warn[i] != 0xFF)
            {
                datlen += sprintf(payload + datlen, "\"wn%d\":%d,", i, invdata.warn[i]);
            }
        }
        /* itv field as the end of the body*/
        datlen += sprintf(payload + datlen, "\"itv\":%d}", GetMinute1((invdata.time[11] - 0x30) * 10 + invdata.time[12] - 0x30, (invdata.time[14] - 0x30) * 10 + invdata.time[15] - 0x30, 0));
        printf("payload   %s\n", payload);
        if (asw_publish(payload) >= 0)
        {
            // netlog_write(ncout);
            printf("publish succeful ncout:%d\r\n", ncout++);
            //if (is_db == 1)
            //    inv_data_delete_one();
            // inv_data_earliest_del();
            printf("[ asw_publish ] inv data\n");
            publish_fail = 0;
            pcout++;
            netework_state = 0;
            if (*lost_flag == 1)
                *lost_flag = 2; /** 接下来：读取下一条缓存*/
        }
        else
        {
            // netlog_write(-1);
            netework_state = 1;
            // printf("AAAE: lost_flag = %d ------------0\n", *lost_flag);
            if (*lost_flag != 1) /** 读的不是缓存*/
                *lost_flag = 3;  //do save

            printf("publish invdata fail\n");

            // printf("AAAE: lost_flag = %d ------------1\n", *lost_flag);
            // write_to_log("publish invinfo faild");
            if (!publish_fail)
                publish_fail_time = esp_timer_get_time();
            publish_fail++;
            if (esp_timer_get_time() - publish_fail_time > 200 * 1000000)
            {
                // write_to_log("publish faild 3 mins exit");
                //exit(-3);
                return dcalilyun_authenticate;
            }
            sleep(2);
        }
    }
END:
    // usleep(50000);//1sec
    return dcalilyun_publish;
}

#include "stdbool.h"
char *g_cst_pub_topic = NULL;
extern int g_mqtt_conn_disable;
int custom_mqtt_task(int cstflag, Inv_data invdata, int *cstlost_flag)
{
    int res = -1;
    static started = 0;
    static _3rd_mqtt_para_t setting = {0};

    /** setup, success only once*********************/
    if (started == 0)
    {
        general_query(NVS_3RD_MQTT_PARA, &setting);
        if (strlen(setting.host) > 0 && strlen(setting.pub_topic) > 0)
        {
            g_cst_pub_topic = setting.pub_topic;
            started = 1;
        }
        else
            return;
    }

    /** connect, also is reconnect *********************/
    if (cat1_get_mqtt_status(1) == false && g_mqtt_conn_disable == 0) //fortest
    {
        if (cat1_get_data_call_status() == true)
        {
            res = cat1_mqtt_conn(1, setting.host, setting.port, setting.client_id, setting.username, setting.password);
            if (res == 0)
            {
                _3rd_pmu_info_publish(1, setting.pub_topic);
                _3rd_inv_info_publish(1, setting.pub_topic);
            }
        }
    }

    if (*cstlost_flag == 1)
    {
        char buff[300] = {0};
        memcpy(buff, (char *)&invdata, sizeof(Inv_data));

        int i = 0;
        printf("cst recv lost data invdata %d crc=%d \n", sizeof(Inv_data), crc16_calc(buff, sizeof(Inv_data)));
        for (; i < sizeof(Inv_data); i++)
            printf("%02x ", buff[i]);
        printf("cst reacv lost end\n");
    }

    if (cstflag == 1)
    {
        char payload[1200] = {0};
        get_invdata_payload(payload, invdata);

        res = cat1_mqtt_pub(1, setting.pub_topic, payload);
        printf("++++++++++++++++ res=%d, cstlost_flag=%d\n", res, *cstlost_flag);
        if (res == 0)
            if (*cstlost_flag == 1)
                *cstlost_flag = 2;

        if (res == -1)
            if (*cstlost_flag != 1)
                *cstlost_flag = 3;
    }

    return res;
    /** publish: pmuInfo, invInfo, invData*********************/
    //_3rd_inv_data_publish(1, setting.pub_topic);
}

void reboot_30min_check(void)
{
    if (get_second_sys_time() > 1800)
    {
        esp_restart();
    }
}