#include "app_mqtt.h"
#include "save_inv_data.h"
#include "kaco.h"
#include "kaco_history_file_manager.h"

static const char *TAG = "app_mqtt.c";

int connect_stage = 0;
// static unsigned short pcout = 3;

// kaco-lanstick 20220802 +
int g_publish_fail = 0;

uint8_t inv_info_sync[30] = {0};
EXT_RAM_ATTR Setting_Para ini_para = {0};

int clear_cloud_invinfo(void)
{
    memset(inv_info_sync, 0, sizeof(inv_info_sync));
    return 0;
}
//----------------------------------------//

// int check_sn_format(char *str)
// {
//     return strlen(str);
// }

//---------------------------------------//

int GetMinute1(int ihour, int iminute, int imsec)
{
    uint32_t dSeconds = (ihour * 60 + iminute + imsec);
    return (dSeconds);
}

//---------------------------------------//

int get_connect_stage(void)
{
    return connect_stage;
}

//--------------------------------------------//
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
//----------------------------------------//
// kaco-lanstick 20220803 +-
/*get time by zeversolar*/
TDCEvent gettime(void)
{
    struct timeval tv = {0};
    struct tm ptime = {0};
    static int last_time_ret = -5; //-5 means ini
    static int64_t cnnot_get_time = 0;
    static uint8_t url_switch = 0;

    int ret;

    // Monitor monitor;

    char *time_url = "http://ws.meteocontrol.de/api/public/time";
    char *https_time_url = "https://ws.meteocontrol.de/api/public/time";

    // char time_url[200] = {0};
    // char reg_key[17] = {0};
    char ts[128] = {0};
    // G123456789AB
    // sn_to_reg(monitor.sn, reg_key);
    // sn_to_reg(DEMO_DEVICE_NAME, reg_key);

    // sprintf(time_url, "http://mqtt.aisweicloud.com/api/v1/updatetime?psno=%s&sign=%s", DEMO_DEVICE_NAME, reg_key);
    // ESP_LOGI(TAG, "timeurl %s\n", time_url);
    // int ret = http_get_time(time_url, &ptime);
    if (url_switch == 0)
    {
        ret = http_get_time(time_url, &ptime);
    }
    else
    {
        ret = http_get_time(https_time_url, &ptime);
    }

    if (ret != 0)
    {
        printf("\n=============  get time ==================\n");
        printf("get time,ret:%d,url_type:%d\n", ret, url_switch);
        printf("\n=============  get time ==================\n");
        url_switch = !url_switch;
        if ((last_time_ret == 0 || last_time_ret == -5) && ret != 0)
        {
            network_log(",[err] get time");
            last_time_ret = ret;
        }

        if (!cnnot_get_time)
            cnnot_get_time = get_second_sys_time();

        // network_log("tm get er");
        // sleep(16);
        // printf("cnnot get over 30mins reboot ooppp %lld \n", cnnot_get_time);
        if (get_second_sys_time() - cnnot_get_time > 1800)
        {
            ESP_LOGE(TAG, "cnnot get over 30mins reboot \n");
            sleep(1);
            sent_newmsg();
        }

        printf("gettime error\r\n");
        return dcIdle;
    }

    else if (ptime.tm_year > 2019 && ptime.tm_year < 2050)
    {
        // ptime.tm_year -= 1900;
        // ptime.tm_mon -= 1;
        ESP_LOGI(TAG, "[year] %d\n", ptime.tm_year);
        ESP_LOGI(TAG, "[mon] %d\n", ptime.tm_mon);
        ESP_LOGI(TAG, "[day] %d\n", ptime.tm_mday);
        ESP_LOGI(TAG, "[hour] %d\n", ptime.tm_hour);
        ESP_LOGI(TAG, "[min] %d\n", ptime.tm_min);
        ESP_LOGI(TAG, "[sec] %d\n", ptime.tm_sec);

        sprintf(ts, "%04d-%02d-%02d %02d:%02d:%02d", ptime.tm_year, ptime.tm_mon, ptime.tm_mday, ptime.tm_hour, ptime.tm_min, ptime.tm_sec);

        ESP_LOGI(TAG, "set time %s \n", ts);
        // system(ts);
        // system("sync");

        int tmz = 0;
        general_query(TM_ZONE_SETFLG, &tmz);

        // display time we have set
        char display[20] = {0};

        set_time_from_string(ts, tmz);
        get_time(display, 20);
        ESP_LOGI(TAG, "system time: %s\n", display);
        // pthread_mutex_lock(&monitor_mutex);
        g_monitor_state |= SYNC_TIME_FROM_CLOUD_INDEX;
        // pthread_mutex_unlock(&monitor_mutex);
        connect_stage = 1;
    }

    if ((last_time_ret != 0 || last_time_ret == -5) && ret == 0 && ptime.tm_year > 119 && ptime.tm_year < 150)
    {
        network_log(",[ok] get time");
        last_time_ret = ret;
    }
    return dcalilyun_publish; // dcalilyun_authenticate;
}

//---------------------------------------//

/*get net state*/
int get_net_state(void)
{
    int ret0 = get_eth_connect_status();
    int ret1 = get_wifi_sta_connect_status(); // v2.0.0 add
    if (ret0 == 0 || ret1 == 0)
    {
        if (ret0 == 0)
            ESP_LOGW(TAG, "stick ETH CONNECT IS OK ");
        else
            ESP_LOGW(TAG, "stick WIFI CONNECT IS OK ");

        return (gettime());
    }
    else
    {
        // ESP_LOGE(TAG,"wifi not conn...");
        return dcIdle;
    }
}

//--------------------------------------//
int cloud_setup(void)
{
    Setting_Para para = {0};
    general_query(NVS_ATE, &para);

    set_product_key(para.product_key);
    set_device_name(para.psn);
    set_device_secret(para.device_secret);

    return 0;
}
//-------------------------------------------//
/*mqtt connect*/
int mqtt_connect(void)
{
    static int mqtt_create_fail = 0;
    static int mqtt_create_fail_time = 0;

    // v2.0.0 change && add
    /*
      Lan 模式下，无网络连接 ，AP模式下，不启用mqtt 连接
    */
    if (g_stick_run_mode == Work_Mode_LAN && get_eth_connect_status() != 0)
        return dcalilyun_authenticate;

    if (g_stick_run_mode == Work_Mode_AP_PROV)
        return dcalilyun_authenticate;
    //  if (g_state_ethernet_connect && 0 == mqtt_app_start()) //网络连接成功状态下 去启动mqtt client
    if ((get_eth_connect_status() == 0 || get_wifi_sta_connect_status() == 0) && 0 == mqtt_app_start()) //网络连接成功状态下 去启动mqtt client
    {
        connect_stage = 2;
        return dcalilyun_publish;
    }
    else
    {
        if (!mqtt_create_fail)
            mqtt_create_fail_time = get_second_sys_time(); // esp_timer_get_time();
        mqtt_create_fail++;
        if (get_second_sys_time() - mqtt_create_fail_time > 600) // esp_timer_get_time() - mqtt_create_fail_time > 300 * 1000000)
        {

            // set_resetnet_reboot();[ change]
            net_com_reboot_flg = 1;
        }
        network_log("mq recon er");
        sleep(16);
        return dcalilyun_authenticate;
    }
}

//------------------------------
void Check_upload_unit32(const char *field, uint32_t value, char *body, uint16_t *datalen) //[ mark] uint8_t->uint32_t
{
    uint16_t len = (*datalen);
    if (value != 0xFFFFFFFF)
    {
        (*datalen) = len + sprintf(body + len, field, value);
    }
}

void Check_upload_sint16(const char *field, uint16_t value, char *body, uint16_t *datalen)
{
    uint16_t len = (*datalen);
    if (value != 0xFFFF)
    {
        (*datalen) = len + sprintf(body + len, field, value);
    }
}

int trans_resrrpc_pub(cloud_inv_msg *resp, unsigned char *ws, int len) //[  mark] ws未用到
{
    char payload[1024] = {0};
    char _data[512] = {0};

    if (len > 0 && len < 255)
    {
        HexToStr((unsigned char *)_data, (unsigned char *)resp->data, resp->len);
        snprintf(payload, 1024, "{\"dat\":\"ok\",\"data\":\"%s\"}", _data);
    }
    else
        snprintf(payload, 1024, "{\"dat\":\"err\",\"msg\":\"%s\"}", "no response");

    if (0 != asw_mqtt_publish(rrpc_res_topic, (char *)payload, strlen(payload), 0)) // res scan(payload))
    {
        printf("[err] publish trans resp\n");

        return -1;
    }
    memset(rrpc_res_topic, 0, 256);
    return 0;
}

//---------------kaco-lanstick 20220802 -------------------//
int parse_inv_sn(void)
{
    if (strlen(inv_dev_info[0].regInfo.sn) > 10)
        return 0;
    return -1;
}

/* 1.获取psn，以及是否含电表信息--->拼成文件名
   2.比较文件名，找到继续，未找到返回0
   3.追加数据到此文件
   4.修改文件名xxx_data.csv到ftp服务器



    0---无历史数据
    1---历史数据--追加--发送
    2--- 发送成功
    -1 -- 发送失败
    */
const char *kaco2_dev_psn[] =
    {
        "30.0NX312004373",
        "3.7NX12002145",
        "SP001000221C0007",
        "5.5.0NX12000033",
        "RG500001119C0001"};

const char *kaco2_conv_psn[] =
    {
        "30.0NX312004373",
        "5.0NX12000010",
        "5.0NX12000087",
        "5.0NX312001410",
        "10.0NX312001413"}; // 0NX312001413

int kaco_handle_data_to_where(kaco_inv_data_t kc_invdata, Inv_data meterdata)
{
    char *pFileList[20] = {0}; // MARK 30???
    char cPsnFileName[70] = {0};
    char fileName[70] = {0};
    int mFilesNum = 0;
    uint8_t i = 0;
    uint8_t bIsFoundMeter = 0;

    //获取文件列表csv文件的第一个文件并上传此文件
    // if (asw_has_history_data())
    // {
    //     // 获取psn，以及是否含电表信息--->拼成文件名

    //     ESP_LOGI("\n==============  AAAAAAAAAAAAAAAAAAAAAA add to history data =============\n");

    // ///////////////////////////////////////////////////

    //----------------kaco test------------------//
    for (uint8_t i = 0; i < 5; i++)
    {
        if (strcmp(kc_invdata.crt_inv_data.psn, kaco2_dev_psn[i]) == 0)
        {
            memset(kc_invdata.crt_inv_data.psn, 0, sizeof(kc_invdata.crt_inv_data.psn));

            memcpy(kc_invdata.crt_inv_data.psn, kaco2_conv_psn[i], strlen(kaco2_conv_psn[i]) + 2);
        }
    }

    ////////////////////////////////////////////////////

    if (0 == strncmp(meterdata.psn, "SDM", 3))
        bIsFoundMeter = 1;
    /*psn+_meter1---含电表
      psn+_meter0---不含电表*/
    if (bIsFoundMeter)
    {
        sprintf(fileName, "/inv/%s_meter1.csv", kc_invdata.crt_inv_data.psn);
    }
    else
    {
        sprintf(fileName, "/inv/%s_meter0.csv", kc_invdata.crt_inv_data.psn);
    }

    // if (asw_get_history_info(fileName) == 1)
    if (asw_kaco_get_filename(fileName, "/inv") == 1)
    {

        ESP_LOGI("---Kaco File Manager Info ---", " Get the %s is exited , history data will be upoad.", fileName);

        // add data into history file and upload

        if (asw_kaco_add_datato_file(bIsFoundMeter, &kc_invdata, &meterdata, fileName) != 0)
        {
            ESP_LOGE("--- Kaco Write Lost Data ERRO --", " failed write lost data to csv file!! ");

            return -1;
        }

        usleep(10000); // 10ms

        int res = asw_upload_history_to_ftp(fileName);
        if (res == 0)
        {
            /*  成功，从文件列表中删除此条 */
            // asw_delete_fileName_from_historyfiles(fileName);
            asw_kao_delete_fileName_from_path(fileName, "/inv");
            return 1;
        }
        else
        {
            /*  失败，返回失败标志，不删除 */
            ESP_LOGE("-- Kaco upload ftp ERRO --", " upload history csv file to ftp failed.");
            return -1;
        }
    }
    else
    {
        ESP_LOGI("---Kaco File Manager Info ---", "  the %s is note exited ,there is not history file.", fileName);

        return 0;
    }
}

/**
 * @brief
 *       根据flag标志
--- flag:0,历史数据写入（更新）无网络数据存储到历史数据文件,无操作

--- flag:1实时数据，检测是否有历史数据
    ---  无历史数据，进行正常发送流


    ----2.1 发送成功，正常返回；
    ----2.2 发送失败，存储到历史数据中
       --- 2.2.1 写入成功，并更新历史文件列表（在写入函数中实现）
       --- 2.2.2 写入失败，数据丢失。失败计数，到了数据失败阈值？？（重启？？重置文件系统？？）
       ---2.2.3 返回到网络状态检测，idle

     ---- 有历史数据，追加到历史数据文件，发送历史数据文件
         --- 判断是否存在csv文件
    ---3.1 存在，获取文件列表csv文件的第一行，上传此文件
      --- 2.1.1 成功，从文件列表中删除此条；
      --- 2.1.2 失败，返回失败标志，不删除。返回到 idle对网络进行监测

--- flag:2,历史数据写入失败，数据丢失，失败计数，失败计数，到了数据失败阈值？？



    ---3.2 不存在，正常返回

 * @param flag
 * @param invdata
 * @param meterdata
 * @param lost_flag
 * @return int
 */
int asw_data_publish(int *flag, kaco_inv_data_t kc_invdata, Inv_data meterdata, int *lost_flag)
{
    int ret = -3;
    static int pub_gap = 0;

    if ((get_second_sys_time() - pub_gap > 300 && parse_inv_sn() < 0) || *flag == 1 || (*flag == 3 && (get_second_sys_time() - pub_gap > 360))) // 300
    {
        pub_gap = get_second_sys_time();
    }
    else
    {
        return dcalilyun_publish;
    }

    /* flag:1 实时数据，获取打包发送 */
    if (*flag == 1)
    {

        *lost_flag = 0;

        ESP_LOGW(TAG, "orgdata %s %s %d \n", kc_invdata.crt_inv_data.psn, kc_invdata.crt_inv_data.time, kc_invdata.crt_inv_data.h_total);

        int res = kaco_handle_data_to_where(kc_invdata, meterdata);

        if (res == 0)
        {

            ret = asw_pub_invs_data(kc_invdata, meterdata, *flag);
            /*发送成功，正常返回*/
            if (ret == 0)
            {
                g_state_mqtt_connect = 0;
                g_publish_fail = 0;

                *flag = 0;
            }
            /*发送失败，存储到历史数据*/
            else if (ret == -1)
            {
                g_publish_fail++;
                g_state_mqtt_connect = -1;

                if (asw_kaco_write_lost_data(&kc_invdata, &meterdata) != 0)
                {
                    ESP_LOGE("--- Kaco Write Lost Data ERRO --", " failed write lost data to csv file!! ");
                    *flag = 2;
                }
                else
                {
                    *flag = 0;
                    ESP_LOGW("-- Kaco Publish Data Warn --", "publish failed num: %d, netsts %d\n", g_publish_fail, g_state_mqtt_connect);
                }

                return dcIdle;
            }
        }
        else if (res == -1)
        {
            ESP_LOGE("--- Kaco Write Lost Data ERRO --", " failed write lost data to upload and  csv file!! ");
            *flag = 2;
        }
        // ret = pub_invs_data(invdata, meterdata, flag);
    }
    /*历史数据写入失败，数据丢失处理 TODO*/
    else if (*flag == 2)
    {
        g_num_failed_write_csv++; //历史数据写失败

        if (g_num_failed_write_csv >= 10) // MARK TODO
        {
            ESP_LOGE("-- Kaco Data Lost ERRO --", "failed  %d write history. ", g_num_failed_write_csv);
            *flag = 0;
        }

        // g_publish_fail++;  //mark TODO ???
    }
    /*消息队列没有数据，发送历史数据*/
    // else if (*flag == 3)
    // {

    //     // char pFileName[70] = {0};
    //     // /*获取历史数据文件名*/
    //     // int res = asw_get_fileName_from_historyfiles(pFileName);
    //     // //获取文件列表csv文件的第一个文件并上传此文件
    //     // if (res == 0)
    //     // {
    //     //     printf("/n================== HHHHHHHHHHHHHHHH get history filename:%s ============\n", pFileName);

    //     //     int res = asw_upload_history_to_ftp(pFileName);
    //     //     if (res == 0)
    //     //     {
    //     //         /*  成功，从文件列表中删除此条 */
    //     //         asw_delete_fileName_from_historyfiles(pFileName);
    //     //         g_state_mqtt_connect = 0;
    //     //         g_publish_fail = 0;
    //     //     }
    //     //     else
    //     //     {
    //     //         /*  失败，返回失败标志，不删除 */
    //     //         ESP_LOGE("-- Kaco upload ftp ERRO --", " upload history csv file to ftp failed.");

    //     //         g_publish_fail++;
    //     //         g_state_mqtt_connect = -1;

    //     //         // 返回 idle对网络进行监测
    //     //         return dcIdle;
    //     //     }
    //     // }
    //     // else
    //     // {
    //     //     *flag = 0; //无状态
    //     // }
    // }
    return dcalilyun_publish;
}
