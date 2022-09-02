#include "asw_protocol_nearby.h"

#include "data_process.h"
#include "asw_nvs.h"
#include "utility.h"
#include "meter.h"
#include "wifi_sta_server.h"
#include "asw_job_http.h"

#include "kaco.h"

// #include "asw_ap_prov.h"
// extern cJSON *scan_ap_res;
static const char *TAG = "asw_protocol_nearby";
// struct ate_command
// {
//     const char *cmd;                  // ATE command
//     void (*hook)(int fd, char *body); // hook function
// };
//--------------------------------------------------------------------------------------
static char *getdev_handle_type_1_fun()
{
    char buf[20] = {0};
    char *msgBuf = NULL;

    Setting_Para setting = {0};
    general_query(NVS_ATE, &setting);

    cJSON *res = cJSON_CreateObject();

    cJSON_AddStringToObject(res, "psn", setting.psn);
    cJSON_AddStringToObject(res, "key", setting.reg_key);
    cJSON_AddNumberToObject(res, "typ", setting.typ);
    cJSON_AddStringToObject(res, "nam", setting.nam);
    cJSON_AddStringToObject(res, "mod", setting.mod);
    cJSON_AddStringToObject(res, "muf", setting.mfr);
    cJSON_AddStringToObject(res, "brd", setting.brw);
    cJSON_AddStringToObject(res, "hw", setting.hw);
    cJSON_AddStringToObject(res, "sw", FIRMWARE_REVISION);
    cJSON_AddStringToObject(res, "wsw", MONITOR_MODULE_VERSION);
    get_time(buf, sizeof(buf));
    cJSON_AddStringToObject(res, "tim", buf);
    cJSON_AddStringToObject(res, "pdk", setting.product_key);
    cJSON_AddStringToObject(res, "ser", setting.device_secret);
    cJSON_AddStringToObject(res, "protocol", CGI_VERSION);

    cJSON_AddStringToObject(res, "ali_ip", setting.host);
    cJSON_AddNumberToObject(res, "ali_port", setting.port);
    cJSON_AddNumberToObject(res, "meter_en", setting.meter_en);
    cJSON_AddNumberToObject(res, "meter_mod", setting.meter_mod);
    cJSON_AddNumberToObject(res, "meter_add", setting.meter_add);

    cJSON_AddNumberToObject(res, "status", g_state_mqtt_connect);

    msgBuf = cJSON_PrintUnformatted(res);

    cJSON_Delete(res);
    return msgBuf;
}
//-------------------------------------------------------
/* 逆变器 */
static char *getdev_handle_type_2_fun()
{
    cJSON *myArr = NULL;
    char *msgBuf = NULL;
    int i = 0;
    int cnt = 0;
    cJSON *res = cJSON_CreateObject();
    myArr = cJSON_AddArrayToObject(res, "inv");

    // for (i = 0; i < g_num_real_inv; i++)  //220726 -
    for (i = 0; i < INV_NUM; i++) // 220726 +
    {
        if (strlen(cgi_inv_arr[i].regInfo.sn) < 1)
            continue;
        cJSON *item = cJSON_CreateObject();

        cJSON_AddStringToObject(item, "isn", cgi_inv_arr[i].regInfo.sn);
        cJSON_AddNumberToObject(item, "add", cgi_inv_arr[i].regInfo.modbus_id);
        cJSON_AddNumberToObject(item, "safety", cgi_inv_arr[i].regInfo.safety_type);
        cJSON_AddNumberToObject(item, "rate", cgi_inv_arr[i].regInfo.rated_pwr);
        cJSON_AddStringToObject(item, "msw", cgi_inv_arr[i].regInfo.msw_ver);
        cJSON_AddStringToObject(item, "ssw", cgi_inv_arr[i].regInfo.ssw_ver);
        cJSON_AddStringToObject(item, "tsw", cgi_inv_arr[i].regInfo.tmc_ver);
        cJSON_AddNumberToObject(item, "pac", cgi_inv_arr[i].invdata.pac);
        cJSON_AddNumberToObject(item, "etd", cgi_inv_arr[i].invdata.e_today);
        cJSON_AddNumberToObject(item, "eto", cgi_inv_arr[i].invdata.e_total);
        cJSON_AddNumberToObject(item, "err", cgi_inv_arr[i].invdata.error);
        cJSON_AddStringToObject(item, "cmv", cgi_inv_arr[i].regInfo.protocol_ver);
        cJSON_AddNumberToObject(item, "mty", cgi_inv_arr[i].regInfo.type);

        int invset = 0;
        general_query(NVS_INV_SETFLG, &invset);
        cJSON_AddNumberToObject(item, "psb_eb", invset);

        cJSON_AddItemToArray(myArr, item);
        cnt++;
    }
    cJSON_AddNumberToObject(res, "num", cnt);

    msgBuf = cJSON_PrintUnformatted(res);

    cJSON_Delete(res);

    return msgBuf;
}

//------------------------------------------------------------------------//
/* 防逆流设备 */
static char *getdev_handle_type_3_fun()
{
    cJSON *res = cJSON_CreateObject();
    char *msgBuf = NULL;
    uint8_t i = 0;
    int all_pac = 0;
    int all_prc = 0;
    int reg_tmp = 5;
    int abs = 0;
    int offset = 0;
    meter_setdata meter = {0};

    general_query(NVS_METER_SET, (void *)&meter);

    ASW_LOGE("-----mod %d,enb %d,target %d,reg %d---", meter.mod, meter.enb, meter.target, meter.reg);

    cJSON_AddNumberToObject(res, "mod", meter.mod);
    cJSON_AddNumberToObject(res, "enb", meter.enb);

    cJSON_AddNumberToObject(res, "exp_m", meter.target);

    /* kaco-mark TODO.... ???? */
    // if (meter.reg > 0x99)
    //     reg_tmp = 10;

    // cJSON_AddNumberToObject(res, "regulate", reg_tmp);
    cJSON_AddNumberToObject(res, "regulate", meter.reg);
    cJSON_AddNumberToObject(res, "enb_PF", meter.enb_pf);
    cJSON_AddNumberToObject(res, "target_PF", meter.target_pf);

    /* kaco-mark TODO....--*/
    // add abs, offset
    // if (meter.reg > 0x100)
    // {
    //     abs = 1;
    //     offset = meter.reg - 0x100;
    // }
    // cJSON_AddNumberToObject(res, "abs", abs);
    // cJSON_AddNumberToObject(res, "abs_offset", offset);

    for (i = 0; i < g_num_real_inv; i++)
    {
        if (strlen((char *)cgi_inv_arr[i].regInfo.sn) < 1)
            continue;
        all_pac += cgi_inv_arr[i].invdata.pac;
        all_prc += cgi_inv_arr[i].regInfo.rated_pwr;
    }
    cJSON_AddNumberToObject(res, "total_pac", all_pac);
    cJSON_AddNumberToObject(res, "total_fac", all_prc);
    cJSON_AddNumberToObject(res, "meter_pac", (int)g_inv_meter.invdata.pac);

    msgBuf = cJSON_PrintUnformatted(res);
    cJSON_Delete(res);

    return msgBuf;
}

//------------------------------------------------------------------//
static char *getdev_handle_type_4_fun()
{
    char *msgBuf = NULL;
    cJSON *res = cJSON_CreateObject();

    cJSON_AddStringToObject(res, "isn", "xxx");
    cJSON_AddNumberToObject(res, "stu_r", 0);
    cJSON_AddNumberToObject(res, "mod_r", 0);
    cJSON_AddNumberToObject(res, "stu_c", 0);
    cJSON_AddNumberToObject(res, "cha", 0);
    cJSON_AddNumberToObject(res, "muf", 0);
    cJSON_AddNumberToObject(res, "mod", 0);
    cJSON_AddNumberToObject(res, "num", 0);
    cJSON_AddNumberToObject(res, "fir_r", 0);

    msgBuf = cJSON_PrintUnformatted(res);

    cJSON_Delete(res);

    return msgBuf;
}
//----------------------------------------//
// kaco-lanstick 20220803 +
static char *getdev_handle_type_90_fun()
{
    cJSON *res = cJSON_CreateObject();
    char *msgBuf = NULL;

    int kaco_mode = 0;
    kaco_mode = get_kaco_run_mode();

    cJSON_AddNumberToObject(res, "runningmode", kaco_mode);

    msgBuf = cJSON_PrintUnformatted(res);
    // printf("res is: %s\n", msgBuf);

    cJSON_Delete(res);
    return msgBuf;
}

//------------------------------------------------------------------//
static char *getdev_handle_type_default_fun()
{
    cJSON *res = cJSON_CreateObject();
    char *msg = NULL;
    char buf[20] = {0};

    Setting_Para setting = {0};
    general_query(NVS_ATE, &setting);

    cJSON_AddStringToObject(res, "psn", setting.psn);
    cJSON_AddStringToObject(res, "key", setting.reg_key);
    cJSON_AddNumberToObject(res, "typ", setting.typ);
    cJSON_AddStringToObject(res, "nam", setting.nam);
    cJSON_AddStringToObject(res, "mod", setting.mod);
    cJSON_AddStringToObject(res, "muf", setting.mfr);
    cJSON_AddStringToObject(res, "brd", setting.brw);
    cJSON_AddStringToObject(res, "hw", setting.hw);
    cJSON_AddStringToObject(res, "sw", FIRMWARE_REVISION);
    cJSON_AddStringToObject(res, "wsw", MONITOR_MODULE_VERSION);
    get_time(buf, sizeof(buf));
    cJSON_AddStringToObject(res, "tim", buf);
    cJSON_AddStringToObject(res, "pdk", setting.product_key);
    cJSON_AddStringToObject(res, "ser", setting.device_secret);
    cJSON_AddStringToObject(res, "protocol", CGI_VERSION);

    cJSON_AddStringToObject(res, "ali_ip", setting.host);
    cJSON_AddNumberToObject(res, "ali_port", setting.port);

    cJSON_AddNumberToObject(res, "status", g_state_mqtt_connect); // change

    msg = cJSON_PrintUnformatted(res);

    cJSON_Delete(res);
    return msg;
}
//--------------------------------------------------------------------------------
static char *getdevdata_handle_type_2_fun(char *inv_sn)
{
    char *msg = NULL;
    uint8_t i = 0;

    for (i = 0; i < g_num_real_inv; i++)
    {
        // printf("\n==========get devdata debug========\n");
        printf("current sn=%s\n", inv_sn);
        printf(" sn[%d]=%s", i, cgi_inv_arr[i].regInfo.sn);
        // printf("\n==========get devdata debug========\n");
        if (strcmp(cgi_inv_arr[i].regInfo.sn, inv_sn) == 0)
            break;
    }

    if (i >= g_num_real_inv)
    {
        ASW_LOGW("device not found");
        return NULL;
    }

    // printf("\n===========inv data print==============\n");
    printf("cgi_inv_arr[%d].PV_cur_voltg[0].iVol=%d\n", i, cgi_inv_arr[i].invdata.PV_cur_voltg[0].iVol);
    // printf("\n===========inv data print==============\n");
    cJSON *res = cJSON_CreateObject();

    cJSON_AddNumberToObject(res, "flg", cgi_inv_arr[i].invdata.status);
    char time_tmp[64] = {0};
    fileter_time(cgi_inv_arr[i].invdata.time, time_tmp);
    cJSON_AddStringToObject(res, "tim", time_tmp);
    cJSON_AddNumberToObject(res, "tmp", cgi_inv_arr[i].invdata.col_temp);
    cJSON_AddNumberToObject(res, "fac", cgi_inv_arr[i].invdata.fac);
    cJSON_AddNumberToObject(res, "pac", cgi_inv_arr[i].invdata.pac);
    cJSON_AddNumberToObject(res, "sac", cgi_inv_arr[i].invdata.sac);
    cJSON_AddNumberToObject(res, "qac", cgi_inv_arr[i].invdata.qac);
    cJSON_AddNumberToObject(res, "eto", cgi_inv_arr[i].invdata.e_total);
    cJSON_AddNumberToObject(res, "etd", cgi_inv_arr[i].invdata.e_today);
    cJSON_AddNumberToObject(res, "hto", cgi_inv_arr[i].invdata.h_total);
    cJSON_AddNumberToObject(res, "pf", cgi_inv_arr[i].invdata.cosphi);
    cJSON_AddNumberToObject(res, "wan", cgi_inv_arr[i].invdata.warn[0]);
    cJSON_AddNumberToObject(res, "err", cgi_inv_arr[i].invdata.error);

    cJSON *myarr = NULL;
    int j;
    int ac_num = cgi_inv_arr[i].regInfo.type - 0X30; // inv_ptr->type
    int pv_num = cgi_inv_arr[i].regInfo.pv_num;
    int str_num = cgi_inv_arr[i].regInfo.str_num;

    myarr = cJSON_AddArrayToObject(res, "vac");
    for (j = 0; j < ac_num; j++)
    {
        int var = cgi_inv_arr[i].invdata.AC_vol_cur[j].iVol;
        if (var != 0xFFFF)
            cJSON_AddNumberToObject(myarr, "", var);
    }

    myarr = cJSON_AddArrayToObject(res, "iac");
    for (j = 0; j < ac_num; j++)
    {
        int var = cgi_inv_arr[i].invdata.AC_vol_cur[j].iCur;
        if (var != 0xFFFF)
            cJSON_AddNumberToObject(myarr, "", var);
    }

    myarr = cJSON_AddArrayToObject(res, "vpv");
    for (j = 0; j < pv_num; j++)
    {
        int var = cgi_inv_arr[i].invdata.PV_cur_voltg[j].iVol;
        if (var != 0xFFFF)
            cJSON_AddNumberToObject(myarr, "", var);
    }

    myarr = cJSON_AddArrayToObject(res, "ipv");
    for (j = 0; j < pv_num; j++)
    {
        int var = cgi_inv_arr[i].invdata.PV_cur_voltg[j].iCur;
        if (var != 0xFFFF)
            cJSON_AddNumberToObject(myarr, "", var);
    }

    myarr = cJSON_AddArrayToObject(res, "str");
    for (j = 0; j < str_num; j++)
    {
        int var = cgi_inv_arr[i].invdata.istr[j];
        if (var != 0xFFFF)
            cJSON_AddNumberToObject(myarr, "", var);
    }

    msg = cJSON_PrintUnformatted(res);

    cJSON_Delete(res);
    return msg;
}

//----------------------------------------------
static char *getdevdata_handle_type_3_fun()
{
    cJSON *res = cJSON_CreateObject(); // Inverter g_inv_meter = {0};
    char *msg = NULL;

    cJSON_AddNumberToObject(res, "flg", g_inv_meter.invdata.status);
    cJSON_AddStringToObject(res, "tim", g_inv_meter.invdata.time);
    // printf("g_inv_meter.invdata.pac %d \n", g_inv_meter.invdata.pac);
    cJSON_AddNumberToObject(res, "pac", (int)g_inv_meter.invdata.pac);
    cJSON_AddNumberToObject(res, "itd", g_inv_meter.invdata.con_stu);
    cJSON_AddNumberToObject(res, "otd", g_inv_meter.invdata.e_today);
    cJSON_AddNumberToObject(res, "iet", g_inv_meter.invdata.e_total); // h_total); //总买电量
    cJSON_AddNumberToObject(res, "oet", g_inv_meter.invdata.h_total); // e_total); //总卖电量
    cJSON_AddNumberToObject(res, "mod", g_merter_config.mod);
    cJSON_AddNumberToObject(res, "enb", g_merter_config.enb);

    msg = cJSON_PrintUnformatted(res);

    cJSON_Delete(res);
    return msg;
}
//----------------------------------------------
static char *getdevdata_handle_type_4_fun()
{
    cJSON *res = cJSON_CreateObject();
    char *msg = NULL;

    //[  lan]都是零，没有作用，删?
    cJSON_AddNumberToObject(res, "flg", 0);
    cJSON_AddNumberToObject(res, "ppv", 0);
    cJSON_AddNumberToObject(res, "etdpv", 0);
    cJSON_AddNumberToObject(res, "etopv", 0);
    cJSON_AddNumberToObject(res, "cst", 0);
    cJSON_AddNumberToObject(res, "bst", 0);
    cJSON_AddNumberToObject(res, "eb1", 0);
    cJSON_AddNumberToObject(res, "wb1", 0);
    cJSON_AddNumberToObject(res, "vb", 0);
    cJSON_AddNumberToObject(res, "cb", 0);
    cJSON_AddNumberToObject(res, "pb", 0);
    cJSON_AddNumberToObject(res, "tb", 0);
    cJSON_AddNumberToObject(res, "soc", 0);
    cJSON_AddNumberToObject(res, "soh", 0);
    cJSON_AddNumberToObject(res, "cli", 0);
    cJSON_AddNumberToObject(res, "clo", 0);
    cJSON_AddNumberToObject(res, "ebi", 0);
    cJSON_AddNumberToObject(res, "ebo", 0);
    cJSON_AddNumberToObject(res, "eaci", 0);
    cJSON_AddNumberToObject(res, "eaco", 0);
    cJSON_AddNumberToObject(res, "vesp", 0);
    cJSON_AddNumberToObject(res, "cesp", 0);
    cJSON_AddNumberToObject(res, "fesp", 0);
    cJSON_AddNumberToObject(res, "pesp", 0);
    cJSON_AddNumberToObject(res, "rpesp", 0);
    cJSON_AddNumberToObject(res, "etdesp", 0);
    cJSON_AddNumberToObject(res, "etoesp", 0);

    msg = cJSON_PrintUnformatted(res);

    cJSON_Delete(res);
    return msg;
}
//----------------------------------------------------//
static char *wlanget_handler_info_1_fun()
{
    wifi_ap_para_t ap_para = {0};
    general_query(NVS_AP_PARA, &ap_para);

    cJSON *res = cJSON_CreateObject();
    char *msg = NULL;

    cJSON_AddStringToObject(res, "mode", "AP");
    cJSON_AddStringToObject(res, "sid", (char *)ap_para.ssid);
    cJSON_AddStringToObject(res, "psw", (char *)ap_para.password);
    cJSON_AddStringToObject(res, "ip", "160.190.0.1");

    msg = cJSON_PrintUnformatted(res);
    cJSON_Delete(res);
    return msg;
}
//-----------------------------------------------------//
static char *wlanget_handler_info_2_fun() //[  mark]TODO这段需要改为ethernet
{
    if (g_stick_run_mode == Work_Mode_AP_PROV)
        return NULL;

    cJSON *res = cJSON_CreateObject();
    char *msg = NULL;
    char *tmp;

    tmp = get_ssid();
    cJSON_AddStringToObject(res, "sid", tmp);
    free(tmp);
    cJSON_AddNumberToObject(res, "srh", get_rssi());
    cJSON_AddStringToObject(res, "DHCP", NULL);

    if (g_stick_run_mode == Work_Mode_LAN)
    {
        cJSON_AddStringToObject(res, "mode", "ETH"); //   mark ethernet

        if (strlen(my_eth_ip) > 0)
            cJSON_AddStringToObject(res, "ip", my_eth_ip);
        else
            cJSON_AddStringToObject(res, "ip", NULL);

        if (strlen(my_eth_gw) > 0)
            cJSON_AddStringToObject(res, "gtw", my_eth_gw);
        else
            cJSON_AddStringToObject(res, "gtw", NULL);

        if (strlen(my_eth_mk) > 0)
            cJSON_AddStringToObject(res, "msk", my_eth_mk);
        else
            cJSON_AddStringToObject(res, "msk", NULL);

        cJSON_AddStringToObject(res, "dns_a", NULL);
        cJSON_AddStringToObject(res, "dns", NULL);
    }
    else if (g_stick_run_mode == Work_Mode_STA)
    {
        cJSON_AddStringToObject(res, "mode", "STA");
        if (strlen(my_sta_ip) > 0)
            cJSON_AddStringToObject(res, "ip", my_sta_ip);

        if (strlen(my_sta_gw) > 0)
            cJSON_AddStringToObject(res, "gtw", my_sta_gw);
        else
            cJSON_AddStringToObject(res, "gtw", NULL);

        if (strlen(my_sta_mk) > 0)
            cJSON_AddStringToObject(res, "msk", my_sta_mk);
        else
            cJSON_AddStringToObject(res, "msk", NULL);
    }

    msg = cJSON_PrintUnformatted(res);
    cJSON_Delete(res);
    return msg;
}
//----------------------------------------------------//
static char *wlanget_handler_info_3_fun()
{
    cJSON *res = cJSON_CreateObject();
    char *msg = NULL;

    // V2.0.0 change

    if (g_stick_run_mode == Work_Mode_AP_PROV)
        cJSON_AddStringToObject(res, "mode", "AP");
    else if (g_stick_run_mode == Work_Mode_STA)
    {
        cJSON_AddStringToObject(res, "mode", "STA");
        if (strlen(my_sta_ip) > 0)
            cJSON_AddStringToObject(res, "ip", my_sta_ip);

        if (strlen(my_sta_gw) > 0)
            cJSON_AddStringToObject(res, "gtw", my_sta_gw);
        else
            cJSON_AddStringToObject(res, "gtw", NULL);

        if (strlen(my_sta_mk) > 0)
            cJSON_AddStringToObject(res, "msk", my_sta_mk);
        else
            cJSON_AddStringToObject(res, "msk", NULL);
    }
    else if (g_stick_run_mode == Work_Mode_LAN)
    {

        cJSON_AddStringToObject(res, "mode", "ETH");
        if (strlen(my_eth_ip) > 0)
            cJSON_AddStringToObject(res, "ip", my_eth_ip);

        if (strlen(my_eth_gw) > 0)
            cJSON_AddStringToObject(res, "gtw", my_eth_gw);
        else
            cJSON_AddStringToObject(res, "gtw", NULL);

        if (strlen(my_eth_mk) > 0)
            cJSON_AddStringToObject(res, "msk", my_eth_mk);
        else
            cJSON_AddStringToObject(res, "msk", NULL);
    }
    msg = cJSON_PrintUnformatted(res);
    cJSON_Delete(res);
    return msg;
}
//-----------------------------------------//
static char *wlanget_handler_info_4_fun()
{
    cJSON *res = NULL;
    char *msg = NULL;

    scan_ap_once();
    if (scan_ap_res != NULL)
    {
        res = scan_ap_res;
    }
    msg = cJSON_PrintUnformatted(res);
    cJSON_Delete(res);
    return msg;
}
//--------------------------------------------//

#define NUM_SETTING_OPRT 4
// kaco-lanstick 20220803 +
int write_daylight_data(int tmz, char *tmdata)
{
    remove("/inv/timezone");
    char buf[16] = {0};
    sprintf(buf, "%d\r\n", tmz);

    printf("write timezone and summer time:%s--%s \n", buf, tmdata);

    if (strlen(buf) <= 0)
        return -1;

    FILE *fp = fopen("/inv/timezone", "w"); //"wb"
    if (fp == NULL)
    {
        ESP_LOGE("tmz", "Failed to open file for writing");
        return -2;
    }

    fseek(fp, 0, SEEK_SET);

    int res = -1;
    res = fwrite(buf, sizeof(char), strlen(buf), fp);
    printf("tmz1  write %d \n", res);
    res = fwrite(tmdata, sizeof(char), strlen(tmdata), fp);
    printf("tmz2 write %d \n", res);

    fclose(fp);
    return 0;
}

static char valueJson[NUM_SETTING_OPRT][20] = {
    "reboot",
    "factory_reset",
    "ufs_format",
    "setscan"};

static int16_t OpCodeCmd[NUM_SETTING_OPRT] = {CMD_REBOOT, CMD_RESET, CMD_FORMAT, CMD_SCAN};

static int16_t setting_handler_device_1_fun(char *action, cJSON *value)
{
// kaco-lanstick 20220803 -
#if 0 
    if (strcmp(action, "settime") == 0)
    {
        char buf[20] = {0};
        getJsonStr(buf, "time", sizeof(buf), value);
        ASW_LOGI("recv time: %s", buf); // recv time: 20201022140813
        if (strlen(buf) > 10)
        {
            set_time_cgi(buf);
        }
        return 0;
    }
#endif
    // kaco-lanstick 20220803 +
    if (strcmp(action, "settime") == 0)
    {
        int tmz = 0;
        getJsonNum(&tmz, "tzmoffset", value);
        tmz = 0 - tmz;
        printf("time zone off %d \n", tmz);

        general_add(TM_ZONE_SETFLG, &tmz);

        char buf[20] = {0};
        getJsonStr(buf, "time", sizeof(buf), value);
        printf("recv time: %s\n", buf);

        char sumtm[512] = {0};
        getJsonStr(sumtm, "daylight", sizeof(sumtm), value);
        printf("sum time: %s\n", sumtm);

        write_daylight_data(tmz, sumtm);

        if (strlen(buf) == 19)
        {
            set_time_from_string(buf, tmz);
        }
        return 0;
    }
    else if (strcmp(action, "setdev") == 0)
    {
        char buf[100] = {0};
        Setting_Para setting = {0};
        wifi_ap_para_t ap_para = {0};
        general_query(NVS_ATE, &setting);

        getJsonStr(buf, "psn", sizeof(buf), value); //[mark]
        ASW_LOGI("psn: %s\n", buf);
        memset(setting.psn, 0, sizeof(setting.psn));
        memcpy(setting.psn, buf, strlen(buf));

        // kaco-lanstick 20220803 -
#if 0
        if (strlen(setting.psn) > 0)
        {
            sprintf((char *)ap_para.ssid, "%s%s", "AISWEI-", setting.psn + strlen(setting.psn) - 4);
        }
        else
        {
            memset(ap_para.ssid, 0, sizeof(ap_para.ssid));
        }
#endif

        memset(buf, 0, sizeof(buf));
        getJsonStr(buf, "key", sizeof(buf), value);
        ASW_LOGI("key: %s\n", buf);
        memset(setting.reg_key, 0, sizeof(setting.reg_key));
        memcpy(setting.reg_key, buf, strlen(buf));

        // kaco-lanstick 20220803 -
#if 0
        if (strlen(setting.reg_key) > 0)
        {
            sprintf((char *)ap_para.password, "%s", setting.reg_key);
        }
        else
        {
            memset(ap_para.ssid, 0, sizeof(ap_para.ssid));
        }
        general_add(NVS_AP_PARA, &ap_para);

#endif
        /* kaco-lanstick 20220808 + */
        // wifi_ap_para_t ap_para = {0};
        getJsonStr((char *)ap_para.ssid, "psn", sizeof(ap_para.ssid), value);
        getJsonStr((char *)ap_para.password, "key", sizeof(ap_para.password), value);
        general_add(NVS_AP_PARA, &ap_para);
        /* kaco-lanstick 20220808 */

        memset(buf, 0, sizeof(buf));
        getJsonNum(&setting.typ, "typ", value);
        printf("typ: %d\n", setting.typ);

        memset(buf, 0, sizeof(buf));
        getJsonStr(buf, "nam", sizeof(buf), value);
        ASW_LOGI("nam: %s\n", buf);
        memset(setting.nam, 0, sizeof(setting.nam));
        memcpy(setting.nam, buf, strlen(buf));

        memset(buf, 0, sizeof(buf));
        getJsonStr(buf, "mod", sizeof(buf), value);
        ASW_LOGI("mod: %s\n", buf);
        memset(setting.mod, 0, sizeof(setting.mod));
        memcpy(setting.mod, buf, strlen(buf));

        memset(buf, 0, sizeof(buf));
        getJsonStr(buf, "muf", sizeof(buf), value);
        ASW_LOGI("muf: %s\n", buf);
        memset(setting.mfr, 0, sizeof(setting.mfr));
        memcpy(setting.mfr, buf, strlen(buf));

        memset(buf, 0, sizeof(buf));
        getJsonStr(buf, "brd", sizeof(buf), value);
        ASW_LOGI("brd: %s\n", buf);
        memset(setting.brw, 0, sizeof(setting.brw));
        memcpy(setting.brw, buf, strlen(buf));

        memset(buf, 0, sizeof(buf));
        getJsonStr(buf, "hw", sizeof(buf), value);
        ASW_LOGI("hw: %s\n", buf);
        memset(setting.hw, 0, sizeof(setting.hw));
        memcpy(setting.hw, buf, strlen(buf));

        memset(buf, 0, sizeof(buf));
        getJsonStr(buf, "wsw", sizeof(buf), value);
        ASW_LOGI("wsw: %s\n", buf);
        memset(setting.wsw, 0, sizeof(setting.wsw));
        memcpy(setting.wsw, buf, strlen(buf));

        memset(buf, 0, sizeof(buf));
        getJsonStr(buf, "pdk", sizeof(buf), value);
        ASW_LOGI("pdk: %s\n", buf);
        memset(setting.product_key, 0, sizeof(setting.product_key));
        memcpy(setting.product_key, buf, strlen(buf));

        memset(buf, 0, sizeof(buf));
        getJsonStr(buf, "ser", sizeof(buf), value);
        ASW_LOGI("ser: %s\n", buf);
        memset(setting.device_secret, 0, sizeof(setting.device_secret));
        memcpy(setting.device_secret, buf, strlen(buf));

        // kaco-lanstick 20220803 -
#if 0 
        memset(buf, 0, sizeof(buf));
        getJsonStr(buf, "ali_ip", sizeof(buf), value);
        ASW_LOGI("ali_ip: %s\n", buf);
        memset(setting.host, 0, sizeof(setting.host));
        memcpy(setting.host, buf, strlen(buf));

        getJsonNum(&setting.port, "ali_port", value);
        getJsonNum(&setting.meter_en, "meter_en", value);
        getJsonNum(&setting.meter_add, "meter_add", value);
        getJsonNum(&setting.meter_mod, "meter_mod", value);

#endif
        general_add(NVS_ATE, &setting);
        if (legal_check(setting.psn) == ASW_OK)
        {
            xSemaphoreGive(g_semar_psn_ready);
        }
        // v2.0.0 add
        int work_mode_ate = Work_Mode_AP_PROV;
        general_add(NVS_WORK_MODE, &work_mode_ate);

        return 0;
    }
    else if (strcmp(action, "test") == 0)
    {
        char buf[100] = {0};

        getJsonStr(buf, "test_meter", sizeof(buf), value);
        ASW_LOGI("test_meter: %s\n", buf);

        memset(buf, 0, sizeof(buf));
        getJsonStr(buf, "test_inv", sizeof(buf), value);
        ASW_LOGI("test_inv: %s\n", buf);

        memset(buf, 0, sizeof(buf));
        getJsonStr(buf, "test_drm", sizeof(buf), value);
        ASW_LOGI("test_drm: %s\n", buf);
        return 0;
    }
    else if (strcmp(action, "operation") == 0)
    {
        ASW_LOGW("=====come in operation.....");
        uint8_t i = 0;
        int num;
        for (i = 0; i < NUM_SETTING_OPRT; i++)
        {
            num = 0;
            getJsonNum(&num, valueJson[i], value);
            ASW_LOGE("=====come in %s,%d.....", valueJson[i], num);
            if (num == 1)
            {
                return OpCodeCmd[i];
            }
            else if (i == 3 && g_scan_stop == 0)
            {
                g_scan_stop = 1;
            }
        }
        return 0;
    }
    else
    {
        return -1;
    }
}

//----------------------------------------------------//
static int16_t setting_handler_device_2_fun(char *action, cJSON *value)
{
    if (strcmp(action, "setpower") == 0) //[  mark] 没有实际操作，只是显示
    {
        int num = 0;

        getJsonNum(&num, "first_start", value);
        ASW_LOGI("first_start: %d\n", num);

        getJsonNum(&num, "power", value);
        ASW_LOGI("power: %d\n", num);

        getJsonNum(&num, "on_off", value);
        ASW_LOGI("on_off: %d\n", num);
        return 0;
    }
    /* kaco-lanstick 20220803 + --- >   */
    else if (strcmp(action, "firstsetting") == 0)
    {
        char buf[100] = {0};
        int num = 0;
        if (value)
        {
            getJsonStr(buf, "isn", sizeof(buf), value);
            printf("isn %s \n", buf);
            getJsonNum(&num, "psd_eb", value);
            printf("psd %d\n", num);
        }
        general_add(NVS_INV_SETFLG, &num);
        return 0;
    }
    else if (strcmp(action, "comsettings") == 0)
    {
        int addr = 0;

        /* kaco-lanstick 20220811 */
        // getJsonNum(&addr, "address", value);
        // cJSON *value_inv = cJSON_GetObjectItemCaseSensitive(value, "value");

        if (value)
        {
            getJsonNum(&addr, "address", value);
            printf("new kaco inv addr %d\n", addr);
        }
        if (addr >= 1 && addr <= 255)
        {
            g_kaco_set_addr = addr;
            event_group_0 &= ~KACO_SET_ADDR_DONE; // clear resp
            event_group_0 |= KACO_SET_ADDR;
            int start_sec = get_second_sys_time();
            while (1)
            {
                usleep(50000);
                int now_sec = get_second_sys_time();
                if (now_sec - start_sec > 6)
                {
                    event_group_0 &= ~KACO_SET_ADDR;
                    return -1;
                }
                if (event_group_0 & KACO_SET_ADDR_DONE)
                {
                    event_group_0 &= ~KACO_SET_ADDR_DONE;
                    kaco_change_modbus_addr_then();
                    // general_add(NVS_KACO_ADDR, &addr);
                    break;
                }
            }
        }
        else
        {
            return -1;
        }

        // if (value)
        // {
        //     getJsonNum(&addr, "address", value);
        //     printf("new kaco inv addr %d\n", addr);
        // }
        // return 0;
        // general_add(NVS_INV_SETFLG, &num);
    }
    /*  <----- kaco-lanstick 20220803 +   */

    else
    {
        return -1;
    }

    return 0;
}
//------------------------------------//
static int16_t setting_handler_device_3_fun(char *action, cJSON *value)
{
    if (strcmp(action, "setmeter") == 0)
    {
        meter_setdata meter = {0};
        general_query(NVS_METER_SET, (void *)&meter);

        getJsonNum(&meter.enb, "enb", value);
        ASW_LOGI("enb: %d\n", meter.enb);

        getJsonNum(&meter.mod, "mod", value);
        ASW_LOGI("mod: %d\n", meter.mod);

        getJsonNum(&meter.target, "target", value);
        ASW_LOGI("target: %d\n", meter.target);

        getJsonNum(&meter.reg, "regulate", value);
        ASW_LOGI("regulate: %d\n", meter.reg);

        getJsonNum(&meter.enb_pf, "enb_PF", value);
        ASW_LOGI("enb_PF: %d\n", meter.enb_pf);

        getJsonNum(&meter.target_pf, "target_PF", value);
        ASW_LOGI("target_PF: %d\n", meter.target_pf);

        /* kaco-lanstick 20220811 - */
        // int abs0 = 0;
        // int offs = 0;
        // getJsonNum(&abs0, "abs", value);
        // ASW_LOGI("abs0: %d\n", abs0);
        // getJsonNum(&offs, "offset", value);
        // ASW_LOGI("offs: %d\n", offs);

        // if (meter.reg == 10)
        // {
        //     meter.reg = 0X100;
        //     if (abs0 == 1 && meter.target == 0)
        //     {
        //         if (offs <= 0 || offs > 100)
        //             offs = 5;

        //         meter.reg |= offs;
        //     }
        // }
        meter.date = get_current_days();

        ASW_LOGI("cgi ------enb:%d, mod:%d, regulate:%d, target:%d, date:%d enabpf:%d tarpf:%d \n",

                 meter.enb, meter.mod, meter.reg, meter.target, meter.date, meter.enb_pf, meter.target_pf);

        general_add(NVS_METER_SET, (void *)&meter);

        send_cgi_msg(60, "clearmeter", 9, NULL);
        return 0;
    }
    else
    {
        return -1;
    }
}
//----------------------------------------------//
static int16_t setting_handler_device_4_fun(char *action, cJSON *value)
{
    if (strcmp(action, "setbattery") == 0) //[  mark] 没有实际操作，只是显示
    {
        // char buf[100] = {0};
        int num = 0;

        getJsonNum(&num, "mod_r", value);
        ASW_LOGI("mod_r: %d\n", num);

        getJsonNum(&num, "muf", value);
        ASW_LOGI("muf: %d\n", num);

        getJsonNum(&num, "mod", value);
        ASW_LOGI("mod: %d\n", num);

        getJsonNum(&num, "num", value);
        ASW_LOGI("num: %d\n", num);
        return 0;
    }
    else
    {
        return -1;
    }
}

/* kaco-lanstick 20220808  */
static int16_t setting_handler_device_90_fun(char *action, cJSON *cJson)
{

    if (strcmp(action, "kaco_runningmode") == 0)
    {
        int old_mode = 0;
        int new_mode = 0;
        old_mode = get_kaco_run_mode();

        getJsonNum(&new_mode, "value", cJson);

        printf("kaco new run mode:%d, old_mode:%d\n", new_mode, old_mode);

        if (new_mode != old_mode)
        {
            set_kaco_run_mode(new_mode);

            printf("\n======  set mode:%d\n", new_mode);
        }

        // httpd_resp_send(req, POST_RES_OK, strlen(POST_RES_OK));
        // sleep(5);
        // esp_restart();

        return CMD_REBOOT;
    }
    else
    {
        return -1;
    }
}

//------------------------------------------------
static int8_t wlanset_handler_mode_ap_fun(cJSON *value)
{
    wifi_ap_para_t _ap_para = {0};
    getJsonStr((char *)_ap_para.ssid, "ssid", sizeof(_ap_para.ssid), value);
    ASW_LOGW("*******888 ap ssid: %s\n", _ap_para.ssid);

    if (strlen((char *)_ap_para.ssid) > 0)
    {
        wifi_ap_para_t ap_para_tmp = {0};
        general_query(NVS_AP_PARA, &ap_para_tmp);
        memcpy(_ap_para.password, ap_para_tmp.password, strlen((char *)ap_para_tmp.password));

        //   ASW_LOGW("*******888 ap ps: %s\n", (char *)ap_para_tmp.password);

        general_add(NVS_AP_PARA, &_ap_para);

        return 0; // httpd_resp_send(req, POST_RES_OK, strlen(POST_RES_OK));
    }
    else
    {
        return -1; //  httpd_resp_send(req, POST_RES_ERR, strlen(POST_RES_ERR));
    }
}
//------------------------------------------------
static int8_t wlanset_handler_mode_state_fun(cJSON *value)
{
    wifi_sta_para_t _sta_para = {0};
    // ESP_LOGW("Wlan Set", "Wifi sta is not usable!!");
    getJsonStr((char *)_sta_para.ssid, "ssid", sizeof(_sta_para.ssid), value);
    ASW_LOGI("ssid-------> %s\n", _sta_para.ssid);

    getJsonStr((char *)_sta_para.password, "psw", sizeof(_sta_para.password), value);
    ASW_LOGI("psw--------> %s\n", _sta_para.password);

    if (strlen((char *)_sta_para.ssid) > 0)
    {
        general_add(NVS_STA_PARA, &_sta_para);
        int work_mode = Work_Mode_STA;
        general_add(NVS_WORK_MODE, &work_mode);

        return CMD_REBOOT;
    }
    else
    {
        return -1; // httpd_resp_send(req, POST_RES_ERR, strlen(POST_RES_ERR));
    }
}

//-------------------------------------------------------//
static int8_t wlanset_handler_mode_easylink_fun(cJSON *value)
{
    int modex = 0;
    getJsonNum(&modex, "enable", value);
    ASW_LOGI("enable: %d\n", modex);
    // if (modex == 1)
    // {
    //     int work_mode = Work_Mode_SMART_PROV;
    //     general_add(NVS_WORK_MODE, &work_mode);

    //     return CMD_REBOOT;
    // }
    // if (modex == 2)
    // {
    /* v2.0.0 change */
    int work_mode = Work_Mode_AP_PROV;
    general_add(NVS_WORK_MODE, &work_mode);
    return CMD_REBOOT;
    // }

    return 0;
}

//---------------------------------------------//
static int8_t wlanset_handler_mode_prov_fun(cJSON *value)
{
    int modex = 0;
    getJsonNum(&modex, "enable", value);
    ASW_LOGI("prov %d\n", modex);
    if (modex == 1) /*  || modex == 3)  //v2.0.0 change */
    {
        general_add(NVS_WORK_MODE, &modex);
        return CMD_REBOOT;
    }

    return 0;
}

/********************************
 * @brief getdev_handle
 *
 * @param devecitType
 * @param isSecret
 * @return char *
 *********************************/
char *protocol_nearby_getdev_handler(uint16_t devecitType, uint16_t isSecret) //   mark 返回的指针需要 free
{
    char *msg = NULL;
    char *sendBuf = NULL;

    ASW_LOGW(" Secret:%d,deviceType:%d", isSecret, devecitType);

    switch (devecitType)
    {
    case 1:
        if (isSecret == 1)
        {
            msg = getdev_handle_type_1_fun();
        }
        break;

    case 2:
        msg = getdev_handle_type_2_fun();
        break;

    case 3:
        msg = getdev_handle_type_3_fun();
        break;

    case 4:
        msg = getdev_handle_type_4_fun();
        break;

    case 90:
        msg = getdev_handle_type_90_fun();
        break;

    default:
        msg = getdev_handle_type_default_fun();
        break;
    }

    if (msg != NULL)
    {
        // sendBuf = (char *)malloc(sizeof(char) * (strlen(msg) + 1));
        // memset(sendBuf,'\0',strlen(msg) + 1);
        sendBuf = (char *)calloc((strlen(msg) + 1), sizeof(char));
        memcpy(sendBuf, msg, strlen(msg) + 1);
        free(msg);
        return sendBuf;
    }
    else
    {
        return NULL;
    }
}

/*****************************
 * @brief getdevdata_handler
 *
 * @param devecitType
 * @param inv_sn
 * @return char *
 ****************************/
char *protocol_nearby_getdevdata_handler(uint16_t devecitType, char *inv_sn)
{
    char *msg = NULL;
    char *sendBuf = NULL;

    ASW_LOGW("--- inv sn: %s", inv_sn);

    switch (devecitType)
    {
    case 2:
        msg = getdevdata_handle_type_2_fun(inv_sn);
        break;
    case 3:
        msg = getdevdata_handle_type_3_fun();
        break;
    case 4:
        msg = getdevdata_handle_type_4_fun();
        break;
    default:
        break;
    }

    if (msg != NULL)
    {

        sendBuf = (char *)calloc((strlen(msg) + 1), sizeof(char));
        memcpy(sendBuf, msg, strlen(msg) + 1);
        free(msg);
        return sendBuf;
    }
    else
    {
        return NULL;
    }
}

/*****************************************
 * @brief protocol_nearby_wlanget_handler
 *
 * @param info
 * @return char*
 *******************************************/

char *protocol_nearby_wlanget_handler(uint16_t info)
{
    char *msg = NULL;
    char *sendBuf = NULL;

    ASW_LOGW("--- inv sn: %d", info);

    switch (info)
    {
    case 1:
        msg = wlanget_handler_info_1_fun();
        break;
    case 2:
        msg = wlanget_handler_info_2_fun();
        break;
    case 3:
        msg = wlanget_handler_info_3_fun();
        break;
    case 4:
        msg = wlanget_handler_info_4_fun();
        break;
    default:
        break;
    }

    if (msg != NULL)
    {
        sendBuf = (char *)calloc((strlen(msg) + 1), sizeof(char));
        memcpy(sendBuf, msg, strlen(msg) + 1);

        ASW_LOGW("  get wlan set %s", msg);
        free(msg);
        return sendBuf;
    }
    else
    {
        return NULL;
    }
}

/*******************************************
 * @brief protocol_nearby_setting_handler
 *
 * @param deviceType
 * @param action
 * @param value
 * @return char*
 ********************************************/
int8_t protocol_nearby_setting_handler(uint16_t deviceType, char *action, cJSON *value, cJSON *cJson)
{
    ASW_LOGW("---setting_handler  value:%d, %s", deviceType, action);
    int8_t op_cmd_code = 0;

    switch (deviceType)
    {
    case 1:
        op_cmd_code = setting_handler_device_1_fun(action, value);
        break;

    case 2:
        op_cmd_code = setting_handler_device_2_fun(action, value);
        break;

    case 3:
        op_cmd_code = setting_handler_device_3_fun(action, value);
        break;

    case 4:
        op_cmd_code = setting_handler_device_4_fun(action, value);
        break;

        // kaco-lanstick 20220808 +
    case 90:
        op_cmd_code = setting_handler_device_90_fun(action, cJson);
        break;

    default:
        op_cmd_code = -1;
        break;
    }

    return op_cmd_code;
}

/********************************************
 * @brief protocol_nearby_wlanset_handler
 * TODO  后期与app对接 关于启停AP、WIFI-Eth的设置
 * @param action
 * @param value
 * @return int8_t
 ********************************************/
int8_t protocol_nearby_wlanset_handler(char *action, cJSON *value)
{
    int8_t m_res_cmd = -1;
    if (strcmp(action, "ap") == 0)
    {
        m_res_cmd = wlanset_handler_mode_ap_fun(value);
    }
    else if (strcmp(action, "station") == 0)
    {
        /* kaco-lanstick 202208080 +- */
        m_res_cmd = 0;
        printf(" Kaco Lanstick no Support WiFi-Sta mode \n");
        // m_res_cmd = wlanset_handler_mode_state_fun(value);
    }
    else if (strcmp(action, "easylink") == 0)
    {
        m_res_cmd = wlanset_handler_mode_easylink_fun(value);
    }
    else if (strcmp(action, "prov") == 0)
    {
        m_res_cmd = wlanset_handler_mode_prov_fun(value);
    }
    else
    {
        m_res_cmd = -1;
    }

    return m_res_cmd;
}

/***************************************
 * @brief protocol_nearby_fdbg_handler
 *
 * @param json
 * @return char*
 ****************************************/
char *protocol_nearby_fdbg_handler(cJSON *json)
{
    ASW_LOGW("******* fdbg_handler  handle..");
    char *ascii_data = NULL; /** 动态内存*/
    int ascii_len;

    cJSON *res_json; /** 动态内存*/
    int BUF_LEN = 600;
    fdbg_msg_t fdbg_msg = {0};
    char *msg = NULL; /** 动态内存*/
    char *sendBuf = NULL;

    if (to_inv_fdbg_mq == NULL)
    {
        ASW_LOGE("to_inv_fdbg_mq is NULL\n");
        if (ascii_data != NULL)
            free(ascii_data);
        return NULL;
    }

    ascii_data = calloc(BUF_LEN, sizeof(char));
    getJsonStr(ascii_data, "data", BUF_LEN, json);

    //[  mark] 被注释掉的
    // fdbg_msg.data = calloc(strlen(ascii_data) / 2, sizeof(char));
    // fdbg_msg.len = StrToHex((unsigned char *)fdbg_msg.data, ascii_data, strlen(ascii_data));

    ASW_LOGI("fdbg req: %s\n", ascii_data);

    char ws[64] = {0};
    // char msgs[300] = {0};  [  mark]

    // strncpy(msgs, ascii_data, 300);
    memset(ws, 'A', 32);
    ASW_LOGI("cgi tans ms %s ws %s \n", ascii_data, ws);

    if (send_cgi_msg(20, ascii_data, strlen(ascii_data), ws) == 0)
    {

        xQueueReset(to_cgi_fdbg_mq);
        ASW_LOGI("to_inv_fdbg_mq send ok\n");
        memset(&fdbg_msg, 0, sizeof(fdbg_msg));
        memset(ascii_data, 0, BUF_LEN);

        if (to_cgi_fdbg_mq != NULL && xQueueReceive(to_cgi_fdbg_mq, &fdbg_msg, 10000 / portTICK_RATE_MS) == pdPASS)
        {
            if (fdbg_msg.type == MSG_FDBG)
            {
                ASW_LOGI("to_inv_fdbg_mq recv ok %d \n", fdbg_msg.len);
                ascii_len = HexToStr((unsigned char *)ascii_data, (unsigned char *)fdbg_msg.data, fdbg_msg.len);
                free(fdbg_msg.data); //[  mark]  在 inv_com.c/upinv_transdata_proc()中 xQueueSend前分配的动态内存
                res_json = cJSON_CreateObject();
                cJSON_AddStringToObject(res_json, "dat", "ok");
                cJSON_AddStringToObject(res_json, "data", ascii_data);
                free(ascii_data);

                msg = cJSON_PrintUnformatted(res_json);
                if (res_json != NULL)
                    cJSON_Delete(res_json);
                if (msg != NULL)
                {
                    sendBuf = (char *)calloc((strlen(msg) + 1), sizeof(char));
                    memcpy(sendBuf, msg, strlen(msg) + 1);

                    ASW_LOGW("  get wlan set %s", msg);
                    free(msg);
                    return sendBuf;
                }

                return NULL;
            }
            else
            {
                ASW_LOGE("fdbg recv type is err\n");
                if (fdbg_msg.data != NULL)
                    free(fdbg_msg.data); //[  mark]
            }
        }
        else
        {
            ASW_LOGE("to_cgi_fdbg_mq is NULL\n");
            if (ascii_data != NULL)
                free(ascii_data);

            if (fdbg_msg.data != NULL)
                free(fdbg_msg.data);
            return NULL;
        }
    }
    else
    {
        ASW_LOGE("to_inv_fdbg_mq send timeout 10 ms\n");
    }

    if (fdbg_msg.data != NULL)
        free(fdbg_msg.data);

    if (ascii_data != NULL)
        free(ascii_data);

    return NULL;
}