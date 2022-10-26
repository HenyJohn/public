/* Simple HTTP Server Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include "nvs_flash.h"
#include "tcpip_adapter.h"
#include "esp_eth.h"
#include "protocol_examples_common.h"

#include <esp_http_server.h>
#include "data_process.h"
#include "asw_store.h"

#include <time.h>
#include "cJSON.h"

#include "mqueue.h"
#include "common.h"
#include "device_data.h"
#include "meter.h"
// #include "cJSON_Utils.h"

// #define ENCODE

// #include "http_parser.h"

/* A simple example that demonstrates how to create GET and POST
 * handlers for the web server.
 */
static char my_sta_ip[16] = {0};
static char my_sta_gw[16] = {0};
static char my_sta_mk[16] = {0};

static const char *TAG = "wifi_sta_server";
const char *POST_RES_OK = "{\"dat\":\"ok\"}";
const char *POST_RES_ERR = "{\"dat\":\"err\"}";

const char *update_page = "<html>"
                          "<head>"
                          "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\">"
                          "<title>固件升级</title>"
                          "</head>"
                          "<body>"
                          "<h1>CAT1监控器升级(主芯片ESP32,从芯片QUECTEL)</h1><br><br><br><br>"
                          "<form action=\"/update.cgi\" enctype=\"multipart/form-data\" method=\"POST\" id=\"from1\"><input type=\"file\" "
                          "name=\"esp32app\" id=\"Img1\"><br><br><input id=\"q\" type=\"submit\"></form>"
                          "</body>"
                          "</html>";

int scan_stop = 0;

extern Inv_arr_t cgi_inv_arr;

extern int inv_real_num;
extern Inverter inv_meter;
extern meter_setdata mtconfig;
extern int ap_enable;
extern bool scan_done;
extern char g_ccid[22];

void int_2_str(int x, char *s, int s_len)
{
    memset(s, 0, s_len);
    sprintf(s, "%d", x);
}

void add_int_to_json(cJSON *res, char *tag_name, int num)
{
    char buf[300] = {0};
    int_2_str(num, buf, sizeof(buf));
    cJSON_AddStringToObject(res, tag_name, buf);
}
void add_str_to_json(cJSON *res, char *tag_name, char *val)
{
    cJSON_AddStringToObject(res, tag_name, val);
}

int get_rssi(void)
{
    wifi_ap_record_t ap_info = {0};
    esp_wifi_sta_get_ap_info(&ap_info);
    return ap_info.rssi;
}

char *get_ssid(void)
{
    char *ssid = calloc(100, 1);
    wifi_ap_record_t ap_info = {0};
    esp_wifi_sta_get_ap_info(&ap_info);
    memcpy(ssid, ap_info.ssid, strlen(ap_info.ssid));
    return ssid;
}

//从json中获取某字符串字段的值
void getJsonStr(char *dest, char *name, int size, cJSON *json)
{
    const cJSON *item = NULL;
    item = cJSON_GetObjectItemCaseSensitive(json, name);
    if (cJSON_IsString(item) && (item->valuestring != NULL))
    {
        strncpy(dest, item->valuestring, size);
    }
}
//从json中获取某数字字段的值
void getJsonNum(int *dest, char *name, cJSON *json)
{
    const cJSON *item = NULL;
    item = cJSON_GetObjectItemCaseSensitive(json, name);
    if (cJSON_IsNumber(item))
    {
        *dest = item->valueint;
    }
}

void web_page_handler(httpd_req_t *req, http_req_para_t *para)
{
    httpd_resp_set_type(req, HTTPD_TYPE_TEXT);
    httpd_resp_send(req, update_page, strlen(update_page));
}

extern int g_4g_mode;
extern char g_oper[30];
extern char g_ip[17];
void wlanget_handler(httpd_req_t *req, http_req_para_t *para)
{
    if (strcmp(para->info, "2") == 0)
    {
        wifi_ap_para_t ap_para = {0};
        general_query(NVS_AP_PARA, &ap_para);

        cJSON *res = cJSON_CreateObject();
        char *msg = NULL;
        if (g_4g_mode == 4)
        {
            cJSON_AddStringToObject(res, "mode", "4G");
        }
        else if (g_4g_mode == 2)
        {
            cJSON_AddStringToObject(res, "mode", "2G");
        }
        else
        {
            cJSON_AddStringToObject(res, "mode", "NULL");
        }

        cJSON_AddStringToObject(res, "sid", g_oper);
        cJSON_AddStringToObject(res, "psw", ap_para.password);
        cJSON_AddStringToObject(res, "ip", g_ip);
        int csq = cat1_get_csq();
        char buf[20] = {0};
        sprintf(buf, "%d", csq);
        cJSON_AddStringToObject(res, "srh", buf);
        // cJSON_AddNumberToObject(res, "srh", csq);

        msg = cJSON_PrintUnformatted(res);
        httpd_resp_send(req, msg, strlen(msg));

        free(msg);
        cJSON_Delete(res);
    }

    else if (strcmp(para->info, "5") == 0)
    {
        apn_para_t apn_para = {0};
        general_query(NVS_APN_PARA, &apn_para);

        cJSON *res = cJSON_CreateObject();
        char *msg = NULL;

        cJSON_AddStringToObject(res, "apn", apn_para.apn);
        cJSON_AddStringToObject(res, "username", apn_para.username);
        cJSON_AddStringToObject(res, "password", apn_para.password);

        msg = cJSON_PrintUnformatted(res);
        httpd_resp_send(req, msg, strlen(msg));

        free(msg);
        cJSON_Delete(res);
    }
    else
    {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "request not found, 404");
    }
}
int fileter_time(char *times, char *buf)
{
    int i = 0, j = 0;
    char new_time[64] = {0};
    for (; i < strlen(times); i++)
        if (times[i] != '-' && times[i] != ' ' && times[i] != ':')
            new_time[j++] = times[i];

    memcpy(buf, new_time, strlen(new_time));
    return 0;
}

void getdevdata_handler(httpd_req_t *req, http_req_para_t *para)
{
    if (strcmp(para->device, "2") == 0)
    {

        cJSON *res = cJSON_CreateObject();
        char *msg = NULL;
        char i = 0;

        for (; i < inv_real_num; i++)
            if (strcmp(cgi_inv_arr[i].regInfo.sn, para->sn) == 0)
                break;
        if (i >= inv_real_num)
        {
            httpd_resp_send(req, "device not found", strlen("device not found"));
            return -1;
        }

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
        int ac_num = cgi_inv_arr[i].regInfo.type - 0X30; //inv_ptr->type
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
        httpd_resp_send(req, msg, strlen(msg));
        printf("\nhttp res: %s\n", msg);

        free(msg);
        cJSON_Delete(res);
    }
    else if (strcmp(para->device, "3") == 0)
    {
        cJSON *res = cJSON_CreateObject(); //Inverter inv_meter = {0};
        char *msg = NULL;

        cJSON_AddNumberToObject(res, "flg", inv_meter.invdata.status);
        cJSON_AddStringToObject(res, "tim", inv_meter.invdata.time);
        //printf("inv_meter.invdata.pac %d \n", inv_meter.invdata.pac);
        cJSON_AddNumberToObject(res, "pac", (int)inv_meter.invdata.pac);
        cJSON_AddNumberToObject(res, "itd", inv_meter.invdata.con_stu);
        cJSON_AddNumberToObject(res, "otd", inv_meter.invdata.e_today);
        cJSON_AddNumberToObject(res, "iet", inv_meter.invdata.e_total);
        cJSON_AddNumberToObject(res, "oet", inv_meter.invdata.h_total);
        cJSON_AddNumberToObject(res, "mod", mtconfig.mod);
        cJSON_AddNumberToObject(res, "enb", mtconfig.enb);
        // // meter_setdata mtconfigy={0};
        // // general_query(NVS_METER_SET, (void *)&mtconfigy);
        // cJSON_AddNumberToObject(res, "oet", mtconfigy.reg);
        printf("read ---->enb:%d, mod:%d, regulate:%d, target:%d, date:%d \n", mtconfig.enb, mtconfig.mod, mtconfig.reg, mtconfig.target, mtconfig.date);

        msg = cJSON_PrintUnformatted(res);
        httpd_resp_send(req, msg, strlen(msg));

        free(msg);
        cJSON_Delete(res);
    }
    else if (strcmp(para->device, "4") == 0)
    {
        cJSON *res = cJSON_CreateObject();
        char *msg = NULL;

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
        httpd_resp_send(req, msg, strlen(msg));

        free(msg);
        cJSON_Delete(res);
    }
    else
    {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "request not found, 404");
    }
}

void getdev_handler(httpd_req_t *req, http_req_para_t *para)
{
    if (strcmp(para->device, "1") == 0 || strlen(para->device) == 0)
    {
        if (strcmp(para->secret, "1") == 0 || strlen(para->secret) == 0)
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

            // cJSON_AddStringToObject(res, "apn", setting.apn);
            cJSON_AddStringToObject(res, "ali_ip", setting.host);
            cJSON_AddNumberToObject(res, "ali_port", setting.port);
            cJSON_AddNumberToObject(res, "meter_en", setting.meter_en);
            cJSON_AddNumberToObject(res, "meter_mod", setting.meter_mod);
            cJSON_AddNumberToObject(res, "meter_add", setting.meter_add);

            cJSON_AddNumberToObject(res, "status", cat1_get_mqtt_status(0) == 1 ? 0 : -1);
            cJSON_AddNumberToObject(res, "csq", cat1_get_csq());
            cJSON_AddStringToObject(res, "iccid", g_ccid);

            msg = cJSON_PrintUnformatted(res);
            httpd_resp_send(req, msg, strlen(msg));

            free(msg);
            cJSON_Delete(res);
        }
    }
    else if (strcmp(para->device, "2") == 0)
    {
        cJSON *res = cJSON_CreateObject();
        char *msg = NULL;
        cJSON *myArr = NULL;
        int i = 0;
        int cnt = 0;

        myArr = cJSON_AddArrayToObject(res, "inv");

        for (i = 0; i < INV_NUM; i++)
        {
            if (strlen(cgi_inv_arr[i].regInfo.sn) <= 0)
                continue;
            cJSON *item = cJSON_CreateObject();
            //memcpy(&(cgi_inv_arr[*inv_index - 1].regInfo), p_device, sizeof(InvRegister));
            //memcpy(&cgi_inv_arr[k].invdata, &inv_data, sizeof(Inv_data));
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
            cJSON_AddNumberToObject(item, "mty", cgi_inv_arr[i].regInfo.mach_type);

            cJSON_AddItemToArray(myArr, item);
            cnt++;
        }
        cJSON_AddNumberToObject(res, "num", cnt);
        msg = cJSON_PrintUnformatted(res);
        // uptime_print("before inv info list\n");
        printf("\nhttp res: %s\n", msg);
        httpd_resp_send(req, msg, strlen(msg));

        free(msg);
        cJSON_Delete(res);
    }
    else if (strcmp(para->device, "3") == 0)
    {
        cJSON *res = cJSON_CreateObject();
        char *msg = NULL;
        char i = 0;
        int all_pac = 0;
        int all_prc = 0;
        int reg_tmp = 5;
        int abs = 0;
        int offset = 0;
        meter_setdata meter = {0};
        //general_query(NVS_METER, &meter);
        general_query(NVS_METER_SET, (void *)&meter);

        cJSON_AddNumberToObject(res, "mod", meter.mod);
        cJSON_AddNumberToObject(res, "enb", meter.enb);

        cJSON_AddNumberToObject(res, "exp_m", meter.target);
        if (meter.reg > 0x99)
            reg_tmp = 10;
        cJSON_AddNumberToObject(res, "regulate", reg_tmp);
        cJSON_AddNumberToObject(res, "enb_PF", meter.enb_pf);
        cJSON_AddNumberToObject(res, "target_PF", meter.target_pf);
        //add abs, offset
        if (meter.reg > 0x100)
        {
            abs = 1;
            offset = meter.reg - 0x100;
        }
        cJSON_AddNumberToObject(res, "abs", abs);
        cJSON_AddNumberToObject(res, "abs_offset", offset);

        for (i = 0; i < inv_real_num; i++)
        {
            if (strlen(cgi_inv_arr[i].regInfo.sn) <= 0)
                continue;
            all_pac += cgi_inv_arr[i].invdata.pac;
            all_prc += cgi_inv_arr[i].regInfo.rated_pwr;
        }
        cJSON_AddNumberToObject(res, "total_pac", all_pac);
        cJSON_AddNumberToObject(res, "total_fac", all_prc);
        cJSON_AddNumberToObject(res, "meter_pac", (int)inv_meter.invdata.pac);

        msg = cJSON_PrintUnformatted(res);
        httpd_resp_send(req, msg, strlen(msg));

        free(msg);
        cJSON_Delete(res);
    }
    else if (strcmp(para->device, "4") == 0)
    {
        cJSON *res = cJSON_CreateObject();
        char *msg = NULL;

        cJSON_AddStringToObject(res, "isn", "xxx");
        cJSON_AddNumberToObject(res, "stu_r", 0);
        cJSON_AddNumberToObject(res, "mod_r", 0);
        cJSON_AddNumberToObject(res, "stu_c", 0);
        cJSON_AddNumberToObject(res, "cha", 0);
        cJSON_AddNumberToObject(res, "muf", 0);
        cJSON_AddNumberToObject(res, "mod", 0);
        cJSON_AddNumberToObject(res, "num", 0);
        cJSON_AddNumberToObject(res, "fir_r", 0);

        msg = cJSON_PrintUnformatted(res);
        httpd_resp_send(req, msg, strlen(msg));

        free(msg);
        cJSON_Delete(res);
    }
}

void getfile_handler(httpd_req_t *req, http_req_para_t *para)
{
    char msg[500] = {0};

    get_file_list(msg);
    httpd_resp_send(req, msg, strlen(msg));
}

int g_mqtt_conn_disable = 0;
void setting_handler(httpd_req_t *req, http_req_para_t *para)
{
    int device = 0;
    char action[30] = {0};
    cJSON *json;
    const char *msg = "error : setting_handler";

    // printf("%.*s\n", para->body_len, para->body);
    json = cJSON_Parse(para->body);
    if (json == NULL)
        goto ERR;

    getJsonNum(&device, "device", json);
    getJsonStr(action, "action", sizeof(action), json);
    cJSON *value = cJSON_GetObjectItemCaseSensitive(json, "value");

    if (device == 1)
    {
        if (strcmp(action, "settime") == 0)
        {
            char buf[20] = {0};
            getJsonStr(buf, "time", sizeof(buf), value);
            printf("recv time: %s\n", buf); //recv time: 20201022140813
            if (strlen(buf) > 10)
            {
                set_time_cgi(buf);
            }
        }
        else if (strcmp(action, "set_3rd_mqtt") == 0)
        {
            _3rd_mqtt_para_t setting = {0};
            getJsonStr(setting.host, "host", sizeof(setting.host), value);
            getJsonNum(&setting.port, "port", value);
            getJsonStr(setting.client_id, "client_id", sizeof(setting.client_id), value);
            getJsonStr(setting.username, "username", sizeof(setting.username), value);
            getJsonStr(setting.password, "password", sizeof(setting.password), value);
            getJsonNum(&setting.ssl_enable, "ssl_enable", value);
            // getJsonStr(setting.ca_cert, "ca_cert", sizeof(setting.ca_cert), value);
            getJsonStr(setting.pub_topic, "pub_topic", sizeof(setting.pub_topic), value);
            getJsonStr(setting.sub_topic, "sub_topic", sizeof(setting.sub_topic), value);
            general_add(NVS_3RD_MQTT_PARA, &setting);

            ssl_file_t cert = {0};
            getJsonStr(cert, "ca_cert", sizeof(cert), value);
            if (strlen(cert) > 0)
                general_add(NVS_3RD_CA_CERT, cert);
        }
        else if (strcmp(action, "set_mqtt_disc") == 0) //fortest
        {
            // cat1_set_mqtt_error(0);
            // cat1_set_mqtt_error(1);
            // g_mqtt_conn_disable = 1;
            ;
        }
        else if (strcmp(action, "setdev") == 0)
        {
            Setting_Para setting = {0};
            general_query(NVS_ATE, &setting);
            getJsonStr(setting.apn, "apn", sizeof(setting.apn), value);
            getJsonStr(setting.psn, "psn", sizeof(setting.psn), value);
            getJsonStr(setting.reg_key, "key", sizeof(setting.reg_key), value);

            wifi_ap_para_t ap_para = {0};
            sprintf(ap_para.ssid, "%s%s", "AISWEI-", setting.psn + strlen(setting.psn) - 4);
            sprintf(ap_para.password, "%s", setting.reg_key);
            general_add(NVS_AP_PARA, &ap_para);

            getJsonNum(&setting.typ, "typ", value);
            getJsonStr(setting.nam, "nam", sizeof(setting.nam), value);
            getJsonStr(setting.mod, "mod", sizeof(setting.mod), value);
            getJsonStr(setting.mfr, "muf", sizeof(setting.mfr), value);
            getJsonStr(setting.brw, "brd", sizeof(setting.brw), value);
            getJsonStr(setting.hw, "hw", sizeof(setting.hw), value);
            //getJsonStr(setting.sw, "sw", sizeof(setting.sw), value);
            getJsonStr(setting.wsw, "wsw", sizeof(setting.wsw), value);
            getJsonStr(setting.product_key, "pdk", sizeof(setting.product_key), value);
            getJsonStr(setting.device_secret, "ser", sizeof(setting.device_secret), value);

            getJsonStr(setting.host, "ali_ip", sizeof(setting.host), value);
            getJsonNum(&setting.port, "ali_port", value);
            getJsonNum(&setting.meter_en, "meter_en", value);
            getJsonNum(&setting.meter_add, "meter_add", value);
            getJsonNum(&setting.meter_mod, "meter_mod", value);

            general_add(NVS_ATE, &setting);

            esp32rf_nvs_clear();
        }
        else if (strcmp(action, "test") == 0)
        {
            int cmd = -1;

            getJsonNum(&cmd, "mqtt_disc", value);
            if (cmd == 0)
                cat1_set_mqtt_error(0);
        }
        else if (strcmp(action, "operation") == 0)
        {
            char buf[100] = {0};
            int num = 0;

            getJsonNum(&num, "reboot", value);
            printf("reboot: %d\n", num);
            httpd_resp_send(req, POST_RES_OK, strlen(POST_RES_OK));
            sleep(1);
            if (num == 1)
            {
                esp_restart();
            }

            num = 0;
            getJsonNum(&num, "factory_reset", value);
            printf("factory_reset: %d\n", num);
            if (num == 1)
            {
                clear_file_system();
                factory_reset_nvs();

                wifi_ap_para_t ap_para = {0};
                Setting_Para setting = {0};
                general_query(NVS_ATE, &setting);
                sprintf(ap_para.ssid, "%s%s", "AISWEI-", setting.psn + strlen(setting.psn) - 4);
                sprintf(ap_para.password, "%s", setting.reg_key);
                general_add(NVS_AP_PARA, &ap_para);

                esp_restart();
            }

            getJsonNum(&num, "drm", value);
            printf("drm: %d\n", num);

            getJsonNum(&num, "ufs_format", value);
            printf("ufs_format: %d\n", num);
            if (num == 1)
            {
                clear_file_system();
            }
            getJsonStr(buf, "ufs_rm", sizeof(buf), value);
            printf("ufs_rm: %s\n", buf);

            num = 0;
            getJsonNum(&num, "setscan", value);
            printf("setscan: %d\n", num);
            if (num == 1)
            {
                /** 扫描逆变器*/
                // fdbg_msg_t fdbg_msg = {0};
                // fdbg_msg.type = MSG_SCAN_INVERTER;
                printf("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ %d\n", num);
                if (1)
                {
                    unsigned char u8msg[8] = {0};
                    unsigned char cnt = 0;

                    cnt = 15;
                    int byteLen = 1;
                    u8msg[0] = cnt;
                    printf("cgi cnt int %d  %d\n", cnt, u8msg[0]);
                    send_cgi_msg(50, u8msg, byteLen, NULL);
                }
                scan_stop = 0;
                // if (xQueueSend(to_inv_fdbg_mq, (void *)&fdbg_msg, 10 / portTICK_RATE_MS) == pdPASS)
                // {
                //     if (to_cgi_fdbg_mq != NULL)
                //     {
                //         if (xQueueReceive(to_cgi_fdbg_mq,
                //                           &fdbg_msg,
                //                           5000 / portTICK_RATE_MS) == pdPASS)
                //         {
                //             ;
                //         }
                //     }
                // }
            }
            if (num == 0 && scan_stop == 0)
            {
                // uptime_print("KKK:recv scan stop cmd, cgi server------------------\n");
                scan_stop = 1;
            }
        }
        else
        {
            httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "request not found, 404");
        }
    }
    else if (device == 2)
    {
        if (strcmp(action, "setpower") == 0)
        {
            char buf[100] = {0};
            int num = 0;

            getJsonNum(&num, "first_start", value);
            printf("first_start: %d\n", num);

            getJsonNum(&num, "power", value);
            printf("power: %d\n", num);

            getJsonNum(&num, "on_off", value);
            printf("on_off: %d\n", num);
        }
        else if (strcmp(action, "apnsetting") == 0)
        {
            apn_para_t apn_para = {0};
            getJsonStr(apn_para.apn, "apn", sizeof(apn_para.apn), value);
            getJsonStr(apn_para.username, "username", sizeof(apn_para.username), value);
            getJsonStr(apn_para.password, "password", sizeof(apn_para.password), value);
            general_add(NVS_APN_PARA, &apn_para);
            httpd_resp_send(req, POST_RES_OK, strlen(POST_RES_OK));
            sleep(4);
            esp_restart();
        }
        else
        {
            httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "request not found, 404");
        }
    }
    else if (device == 3)
    {
        if (strcmp(action, "setmeter") == 0)
        {
            //char buf[100] = {0};
            //int num = 0;
            //meter_regulate_t meter = {0};
            //general_query(NVS_METER, &meter);
            meter_setdata meter = {0};
            general_query(NVS_METER_SET, (void *)&meter);

            getJsonNum(&meter.enb, "enb", value);
            printf("enb: %d\n", meter.enb);

            getJsonNum(&meter.mod, "mod", value);
            printf("mod: %d\n", meter.mod);

            getJsonNum(&meter.target, "target", value);
            printf("target: %d\n", meter.target);

            getJsonNum(&meter.reg, "regulate", value);
            printf("regulate: %d\n", meter.reg);

            getJsonNum(&meter.enb_pf, "enb_PF", value);
            printf("enb_PF: %d\n", meter.enb_pf);

            getJsonNum(&meter.target_pf, "target_PF", value);
            printf("target_PF: %d\n", meter.target_pf);

            int abs0 = 0;
            int offs = 0;
            getJsonNum(&abs0, "abs", value);
            printf("abs0: %d\n", abs0);
            getJsonNum(&offs, "offset", value);
            printf("offs: %d\n", offs);

            if (meter.reg == 10)
            {
                meter.reg = 0X100;
                if (abs0 == 1 && meter.target == 0)
                {
                    if (offs <= 0 || offs > 100)
                        offs = 5;

                    meter.reg |= offs;
                }
            }
            meter.date = get_current_days();

            printf("cgi ------enb:%d, mod:%d, regulate:%d, target:%d, date:%d enabpf:%d tarpf:%d \n", meter.enb, meter.mod, meter.reg, meter.target, meter.date, meter.enb_pf, meter.target_pf);

            general_add(NVS_METER_SET, (void *)&meter);
            //httpd_resp_send(req, POST_RES_OK, strlen(POST_RES_OK));
            //sleep(2);
            //esp_restart();
            send_cgi_msg(60, "clearmeter", 9, NULL);
        }
        else
        {
            httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "request not found, 404");
        }
    }
    else if (device == 4)
    {
        if (strcmp(action, "setbattery") == 0)
        {
            char buf[100] = {0};
            int num = 0;

            getJsonNum(&num, "mod_r", value);
            printf("mod_r: %d\n", num);

            getJsonNum(&num, "muf", value);
            printf("muf: %d\n", num);

            getJsonNum(&num, "mod", value);
            printf("mod: %d\n", num);

            getJsonNum(&num, "num", value);
            printf("num: %d\n", num);
        }
        else
        {
            httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "request not found, 404");
        }
    }
    else
    {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "request not found, 404");
    }

    httpd_resp_send(req, POST_RES_OK, strlen(POST_RES_OK));
    if (json != NULL)
        cJSON_Delete(json);
    return;

ERR:
    httpd_resp_send(req, msg, strlen(msg));
    if (json != NULL)
        cJSON_Delete(json);
    return;
}

void wlanset_handler(httpd_req_t *req, http_req_para_t *para)
{
    char action[30] = {0};
    cJSON *json;
    const char *msg = "error : wlanset_handler";

    // printf("%.*s\n", para->body_len, para->body);
    json = cJSON_Parse(para->body);
    if (json == NULL)
        goto ERR;
    getJsonStr(action, "action", sizeof(action), json);
    cJSON *value = cJSON_GetObjectItemCaseSensitive(json, "value");

    if (strcmp(action, "ap") == 0)
    {
        wifi_ap_para_t _ap_para = {0};
        getJsonStr(_ap_para.ssid, "ssid", sizeof(_ap_para.ssid), value);
        printf("ap ssid: %s\n", _ap_para.ssid);

        //getJsonStr(_ap_para.password, "psw", sizeof(_ap_para.password), value);
        //printf("ap psw: %s\n", _ap_para.password);
        wifi_ap_para_t ap_para_tmp = {0};
        general_query(NVS_AP_PARA, &ap_para_tmp);
        memcpy(_ap_para.password, ap_para_tmp.password, strlen(ap_para_tmp.password));
        if (strlen(_ap_para.ssid) > 0)
            general_add(NVS_AP_PARA, &_ap_para);
        httpd_resp_send(req, POST_RES_OK, strlen(POST_RES_OK));
        sleep(2);
        esp_restart();
    }
    // else if (strcmp(action, "station") == 0)
    // {
    //     wifi_sta_para_t _sta_para = {0};
    //     getJsonStr(_sta_para.ssid, "ssid", sizeof(_sta_para.ssid), value);
    //     printf("ssid-------> %s\n", _sta_para.ssid);

    //     getJsonStr(_sta_para.password, "psw", sizeof(_sta_para.password), value);
    //     printf("psw--------> %s\n", _sta_para.password);

    //     general_add(NVS_STA_PARA, &_sta_para);
    //     int work_mode = NORMAL;
    //     general_add(NVS_WORK_MODE, &work_mode);
    //     httpd_resp_send(req, POST_RES_OK, strlen(POST_RES_OK));
    //     sleep(5);
    //     // char ssix[64]={0};
    //     // char pssx[64]={0};
    //     // memcpy(ssix, _sta_para.ssid, strlen(_sta_para.ssid));
    //     // memcpy(pssx, _sta_para.password, strlen(_sta_para.password));
    //     // reconfig_sta();
    //     // esp_wifi_connect();
    //     printf("set replay ok---------------\n");
    //     esp_restart();
    // }
    // else if (strcmp(action, "easylink") == 0)
    // {
    //     //char buf[16] = {0};
    //     //getJsonStr(buf, "enable", sizeof(buf), value);
    //     int modex = 0;
    //     getJsonNum(&modex, "enable", value);
    //     printf("enable: %d\n", modex);
    //     if (modex == 1)
    //     {
    //         int work_mode = SMART_PROV;
    //         general_add(NVS_WORK_MODE, &work_mode);
    //         httpd_resp_send(req, POST_RES_OK, strlen(POST_RES_OK));
    //         sleep(2);
    //         esp_restart();
    //     }
    //     if (modex == 2)
    //     {
    //         int work_mode = AP_PROV;
    //         general_add(NVS_WORK_MODE, &work_mode);
    //         httpd_resp_send(req, POST_RES_OK, strlen(POST_RES_OK));
    //         sleep(2);
    //         esp_restart();
    //     }
    // }
    // else if (strcmp(action, "prov") == 0)
    // {
    //     int modex = 0;
    //     getJsonNum(&modex, "enable", value);
    //     printf("prov %d\n", modex);
    //     if (modex == 1 || modex == 3)
    //     {
    //         general_add(NVS_WORK_MODE, &modex);
    //         httpd_resp_send(req, POST_RES_OK, strlen(POST_RES_OK));
    //         sleep(2);
    //         esp_restart();
    //     }
    // }
    else
    {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "request not found, 404");
    }

    httpd_resp_send(req, POST_RES_OK, strlen(POST_RES_OK));
    if (json != NULL)
        cJSON_Delete(json);
    return;

ERR:
    httpd_resp_send(req, msg, strlen(msg));
    if (json != NULL)
        cJSON_Delete(json);
    return;
}

void innerfile_handler(httpd_req_t *req, http_req_para_t *para)
{
    char msg[500] = {0};
    char *netlog = "/home/netlog.txt";

    printf("LOG NET --------------------------------\r\n");
    if (strlen(para->filename) == 0)
    {
        get_file_list(msg);
        httpd_resp_send(req, msg, strlen(msg));
        return;
    }
    else
    {
        char path[100] = {0};
        sprintf(path, "/home/%s", para->filename);

        if (strcmp(path, netlog) == 0)
        {
            FILE *fp = NULL;
            fp = fopen(netlog, "r");
            if (fp != NULL)
            {
                char buf[100] = {0};
                while (!feof(fp)) // to read file
                {
                    memset(buf, 0, sizeof(buf));
                    int nread = fread(buf, 1, 100, fp);
                    httpd_resp_send_chunk(req, buf, nread);
                }
                httpd_resp_send_chunk(req, NULL, 0);

                fclose(fp);
                return;
            }
        }
    }
    httpd_resp_send(req, POST_RES_ERR, strlen(POST_RES_ERR));
}

extern char g_ip[17];
int get_all_log(char *buf)
{
    char bufx[1024] = {0};
    char tmp[350] = {0};

    get_sys_log(tmp);
    //printf("log---------->%d--%s \n", strlen(tmp), tmp);
    strcat(bufx, tmp);
    //printf("log---------->%d--%s \n", strlen(bufx), bufx);
    memset(tmp, 0, 350);
    get_inv_log(tmp, 1);
    strcat(bufx, tmp);
    //printf("log================>%d--%s \n", strlen(bufx), bufx);
    memset(tmp, 0, 350);
    get_inv_log(tmp, 2);

    if ((strlen(bufx) + strlen(tmp)) < 1024)
        strcat(bufx, tmp);
    //printf("log##################%d--%s \n", strlen(bufx), bufx);
    memset(tmp, 0, 350);
    get_inv_log(tmp, 3);

    if ((strlen(bufx) + strlen(tmp)) < 1024)
        strcat(bufx, tmp);
    //printf("log++++++++++++++++++++++%d--%s \n", strlen(bufx), bufx);
    char now_timex[30] = {0};
    char ipx[30] = {0};
    char buft[100] = {0};

    get_time(now_timex, sizeof(now_timex));

    sprintf(buft, "\n%s ip:%s ", now_timex, g_ip);

    if ((strlen(bufx) + strlen(buft)) < 1024)
        strcat(bufx, buft);

    memcpy(buf, bufx, strlen(bufx));
    return 0;
}

void get_log_handler(httpd_req_t *req, http_req_para_t *para)
{
    char buff[1024] = {0};
    get_all_log(buff);
    printf("cgisyslog %d--%s \n", strlen(buff), buff);

    if (strlen(buff))
        httpd_resp_send_chunk(req, (uint8_t *)buff, strlen(buff));
    else
        httpd_resp_send_chunk(req, "no log", strlen("no log"));
}

void print_bytes_as_hex(char *_tag, char *buf, int len)
{
    int i;
    printf("%s: ", _tag);
    for (i = 0; i < len; i++)
    {
        printf("%02X ", buf[i]);
    }
    printf("\n");
}

void fdbg_handler(httpd_req_t *req, http_req_para_t *para)
{
    char *ascii_data = NULL; /** 动态内存*/
    int ascii_len;
    cJSON *json;     /** 动态内存*/
    cJSON *res_json; /** 动态内存*/
    int BUF_LEN = 600;
    fdbg_msg_t fdbg_msg = {0};
    char *msg = NULL; /** 动态内存*/

    // printf("%.*s\n", para->body_len, para->body);
    json = cJSON_Parse(para->body);
    if (json != NULL)
    {
        ascii_data = calloc(BUF_LEN, sizeof(char));
        getJsonStr(ascii_data, "data", BUF_LEN, json);
        if (json != NULL)
            cJSON_Delete(json);

        // uptime_print("KKK:fdbg req------------------\n");
        printf("fdbg req: %s\n", ascii_data);
        //fdbg_msg.data = calloc(strlen(ascii_data) / 2, sizeof(char));
        //fdbg_msg.len = StrToHex(fdbg_msg.data, ascii_data, strlen(ascii_data));
        print_bytes_as_hex("parse bytes", fdbg_msg.data, fdbg_msg.len);

        if (to_inv_fdbg_mq != NULL)
        {
            //xQueueReset(to_cgi_fdbg_mq);

            char ws[64] = {0};
            char msgs[300] = {0};
            //cJSON *item = cJSON_GetObjectItem(json, "ws");
            //strncpy(ws, item->valuestring, 64);
            //item = cJSON_GetObjectItem(json, "message");
            strncpy(msgs, ascii_data, 300);
            memset(ws, 'A', 32);
            printf("cgi tans ms %s ws %s \n", msgs, ws);

            // uptime_print("KKK:snd req to mq------------------\n");
            if (send_cgi_msg(20, msgs, strlen(msgs), ws) == 0)
            {
                /*if (xQueueReceive(to_cgi_fdbg_mq,
                                  &fdbg_msg,
                                  1800 / portTICK_RATE_MS) == pdPASS);
                */
                xQueueReset(to_cgi_fdbg_mq);
                printf("to_inv_fdbg_mq send ok\n");
                memset(&fdbg_msg, 0, sizeof(fdbg_msg));
                memset(ascii_data, 0, BUF_LEN);
                // uptime_print("KKK:fdbg wait com res------------------\n");
                if (to_cgi_fdbg_mq != NULL)
                {
                    if (xQueueReceive(to_cgi_fdbg_mq,
                                      &fdbg_msg,
                                      10000 / portTICK_RATE_MS) == pdPASS)
                    {
                        if (fdbg_msg.type == MSG_FDBG)
                        {
                            // uptime_print("KKK:fdbg com res recved------------------\n");
                            printf("to_inv_fdbg_mq recv ok %d \n", fdbg_msg.len);
                            ascii_len = HexToStr(ascii_data, fdbg_msg.data, fdbg_msg.len);
                            free(fdbg_msg.data);
                            res_json = cJSON_CreateObject();
                            cJSON_AddStringToObject(res_json, "dat", "ok");
                            cJSON_AddStringToObject(res_json, "data", ascii_data);
                            free(ascii_data);

                            msg = cJSON_PrintUnformatted(res_json);
                            if (res_json != NULL)
                                cJSON_Delete(res_json);
                            // uptime_print("KKK:fdbg finish------------------\n");
                            httpd_resp_send(req, msg, strlen(msg));
                            free(msg);
                            return;
                        }
                        else
                        {
                            printf("fdbg recv type is err\n");
                            free(fdbg_msg.data);
                        }
                    }
                    else
                    {
                        printf("fdbg recv inv timeout after 5 sec\n");
                    }
                }
                else
                {
                    printf("err: to_cgi_fdbg_mq is null\n");
                }
            }
            else
            {
                printf("to_inv_fdbg_mq send timeout 10 ms\n");
            }
        }
        else
        {
            printf("to_inv_fdbg_mq is NULL\n");
        }
        free(ascii_data);
    }

    httpd_resp_send(req, POST_RES_ERR, strlen(POST_RES_ERR));
    return;
}

void update_handler(httpd_req_t *req, http_req_para_t *para)
{
    char *msg = "update ok";
    httpd_resp_send(req, msg, strlen(msg));
    return;
}

typedef struct
{
    const char *cgi;
    void (*hook)(httpd_req_t *req, http_req_para_t *para);
} http_req_tab_t;

static const http_req_tab_t http_req_tab[] = {
    {"/getdev.cgi", getdev_handler},
    {"/getdevdata.cgi", getdevdata_handler},
    {"/wlanget.cgi", wlanget_handler},
    {"/wlanset.cgi", wlanset_handler},
    {"/webpage.cgi", web_page_handler},
    {"/update.cgi", update_handler},
    {"/fdbg.cgi", fdbg_handler},
    {"/setting.cgi", setting_handler},
    {"/innerfile.cgi", innerfile_handler},
    {"/get_sys_log.cgi", get_log_handler}};

/* An HTTP GET handler */
static esp_err_t asw_get_handler(httpd_req_t *req)
{
    char *buf;
    size_t buf_len;
    http_req_para_t req_para = {0};

    /** 无效请求响应  */
    // httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "request not found, 404");
    // return ESP_OK;

    ESP_LOGE("http get req url", "%s", req->uri);
    ESP_LOGE("http get req len", "%d", req->content_len);

    /** 解析head中的键值对 */
    // buf_len = httpd_req_get_hdr_value_len(req, "Host") + 1;
    // if (buf_len > 1)
    // {
    //     buf = malloc(buf_len);
    //     /* Copy null terminated value string into buffer */
    //     if (httpd_req_get_hdr_value_str(req, "Host", buf, buf_len) == ESP_OK)
    //     {
    //         ESP_LOGI(TAG, "Found header => Host: %s", buf);
    //     }
    //     free(buf);
    // }

    /** 解析url中的参数 */
    buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1)
    {
        buf = malloc(buf_len);
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK)
        {
            ESP_LOGI(TAG, "Found URL query => %s", buf);
            /* Get value of expected key from query string */
            if (httpd_query_key_value(buf, "sn", req_para.sn, sizeof(req_para.sn)) == ESP_OK)
            {
                ESP_LOGI(TAG, "Found => sn=%s", req_para.sn);
            }
            if (httpd_query_key_value(buf, "device", req_para.device, sizeof(req_para.device)) == ESP_OK)
            {
                ESP_LOGI(TAG, "Found => device=%s", req_para.device);
            }
            if (httpd_query_key_value(buf, "secret", req_para.secret, sizeof(req_para.secret)) == ESP_OK)
            {
                ESP_LOGI(TAG, "Found => secret=%s", req_para.secret);
            }
            if (httpd_query_key_value(buf, "info", req_para.info, sizeof(req_para.info)) == ESP_OK)
            {
                ESP_LOGI(TAG, "Found => info=%s", req_para.info);
            }
            if (httpd_query_key_value(buf, "filename", req_para.filename, sizeof(req_para.filename)) == ESP_OK)
            {
                ESP_LOGI(TAG, "Found => filename=%s", req_para.filename);
            }
        }
        free(buf);
    }

    int i = 0;
    int req_num = sizeof(http_req_tab) / sizeof(http_req_tab_t);

    for (i = 0; i < req_num; i++)
    {
        if (strstr(req->uri, http_req_tab[i].cgi) != NULL)
            break;
        if (i == req_num - 1)
        {
            printf("[ err ] No this http request: %s\n", req->uri);
            httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "request not found, 404");
            return ESP_OK;
        }
    }
    /** 响应头中加入自定义键值对 */
    httpd_resp_set_hdr(req, "Connection", "close");
    httpd_resp_set_hdr(req, "Cache-Control", "no-cache");
    httpd_resp_set_type(req, HTTPD_TYPE_JSON);
    (*http_req_tab[i].hook)(req, &req_para);

    /* After sending the HTTP response the old HTTP request
     * headers are lost. Check if HTTP request headers can be read now. */
    // if (httpd_req_get_hdr_value_len(req, "Host") == 0)
    // {
    //     ESP_LOGI(TAG, "Request headers lost");
    // }
    return ESP_OK;
}

//char my_rssi[100] = {0};

const httpd_uri_t asw_get_callback = {
    .uri = "/*",
    .method = HTTP_GET,
    .handler = asw_get_handler,
    .user_ctx = NULL};

/* An HTTP POST handler */
#define FORM_MATCHER "multipart/"
#define BOUNDARY_MATCHER "boundary="
#define NEWLINE_MATCHER "\r\n"
#define DOUBLELINE_MATCHER "\r\n\r\n"
#define FILENAME_MATCHER "filename=\""
#define FILENAME_END_MATCHER "\"\r\n"
static const int form_matcher_len = strlen(FORM_MATCHER);
static const int boundary_matcher_len = strlen(BOUNDARY_MATCHER);
static const int newline_matcher_len = strlen(NEWLINE_MATCHER);
static const int doubleline_matcher_len = strlen(DOUBLELINE_MATCHER);
static const int filename_matcher_len = strlen(FILENAME_MATCHER);

enum
{
    MULTIPART_WAIT_START,
    MULTIPART_WAIT_FINISH
};

int send_cgi_msg(int type, unsigned char *buff, int lenthg, unsigned char *ws)
{
    cloud_inv_msg buf = {0};
    int ret;
    printf("send cgi msg--------------------%d %d %d \n", type, buff[0], lenthg);

    if (lenthg)
        memcpy(buf.data, buff, lenthg);
    if (ws)
    {
        memcpy(&buf.data[lenthg], ws, strlen(ws));
        printf("send cgi msg %d %d \n", buf.data[0], lenthg);
    }
    buf.type = type;
    buf.len = lenthg;

    printf("send cgi msg %d %d \n", buf.data[0], lenthg);

    if (to_inv_fdbg_mq != NULL)
    {
        xQueueSend(to_inv_fdbg_mq, (void *)&buf, (TickType_t)10); //10 / portTICK_RATE_MS
        //printf("send cgi msg ok %d \n", ret);
        return 0;
    }
    return -1;
}

Update_p update_info = NULL;
void update_info_gen(void)
{
    char *head_ptr = NULL;
    if (update_info == NULL)
    {
        printf("err: update_info is null\n");
        return;
    }
    if (strstr(update_info->file_name, "master") != NULL)
    {
        head_ptr = update_info->file_name + strlen("master");
        memset(update_info->part_num, 0, sizeof(update_info->part_num));
        strncpy(update_info->part_num, head_ptr, 13);

        memset(update_info->file_path, 0, sizeof(update_info->file_path));
        sprintf(update_info->file_path, "/home/%s", update_info->file_name);

        update_info->file_type = 1;
    }
    else if (strstr(update_info->file_name, "safety") != NULL)
    {
        head_ptr = update_info->file_name + strlen("safety");
        memset(update_info->part_num, 0, sizeof(update_info->part_num));
        strncpy(update_info->part_num, head_ptr, 13);

        memset(update_info->file_path, 0, sizeof(update_info->file_path));
        sprintf(update_info->file_path, "/home/%s", update_info->file_name);

        update_info->file_type = 4;
    }
    else if (strstr(update_info->file_name, "slave") != NULL)
    {
        head_ptr = update_info->file_name + strlen("slave");
        memset(update_info->part_num, 0, sizeof(update_info->part_num));
        strncpy(update_info->part_num, head_ptr, 13);

        update_info->file_type = 4;
    }
    else if (strstr(update_info->file_name, "update") != NULL)
    {
        head_ptr = update_info->file_name + strlen("update");
        memset(update_info->part_num, 0, sizeof(update_info->part_num));
        strncpy(update_info->part_num, head_ptr, 13);

        memset(update_info->file_path, 0, sizeof(update_info->file_path));
        sprintf(update_info->file_path, "%s", "/home/update.bin");

        update_info->file_type = 0;
    }
    else
    {
        update_info->file_type = -1;
    }
    printf("update_info->file_type: %d\n", update_info->file_type);
}

static esp_err_t asw_post_handler(httpd_req_t *req)
{
    char *data = NULL;
    char *decode_data = NULL;
    int data_len = 0;
    int decode_len = 0;
    int req_num;
    int is_cgi = 1;

    /** 接收文件所需*/
    char *content_type = NULL;
    char *boundary = NULL;
    char *finish_boundary = NULL;
    char *frame = NULL;
    char *last_frame_tail = NULL;
    char *search_box = NULL;
    int frame_len = 0;
    int last_frame_tail_len;
    int search_box_len;
    int content_type_len = 0;
    int boundary_len;
    int start_boundary_len = 0;
    int finish_boundary_len = 0;
    char *p1;
    char *p2;
    char *p3;
    char *p4;
    char *p5;
    int len1;
    int len2;
    int file_len = 0;
    int time_cnt_sec = 0;
    int nwrite;
    int multipart_state = MULTIPART_WAIT_START;
    FILE *update_file = NULL;
    int check;
    int declare_file_len = 0;
    http_req_para_t req_para = {0};
    int i = 0;

    char buf[1024] = {0};
    int ret, remaining = req->content_len;

    ESP_LOGE("http post req url", "%s", req->uri);
    ESP_LOGE("http post req len", "%d", req->content_len);

    /** 检查点对点标识 */
    char *p2p_str = NULL;
    int p2p_len = httpd_req_get_hdr_value_len(req, "p2p");
    if (p2p_len > 0)
    {
        p2p_str = calloc(p2p_len + 1, 1);
        if (httpd_req_get_hdr_value_str(req, "p2p", p2p_str, p2p_len) == ESP_OK)
        {
            if (strcmp(p2p_str, "1") == 0)
            {
                g_p2p_mode = 1; //38400
            }
            else if (strcmp(p2p_str, "2") == 0)
            {
                g_p2p_mode = 2; //9600
            }
        }
        free(p2p_str);
    }

    /** 检查接收multipart文件 */
    content_type_len = httpd_req_get_hdr_value_len(req, "Content-Type") + 1;
    if (content_type_len > 1)
    {
        content_type = malloc(content_type_len);
        memset(content_type, 0, content_type_len);
        /* Copy null terminated value string into buffer */
        if (httpd_req_get_hdr_value_str(req, "Content-Type", content_type, content_type_len) == ESP_OK)
        {
            ESP_LOGE(TAG, "Found header => Content-Type: %s", content_type);
            /** 匹配multipart，提取分割线*/
            p1 = strstr(content_type, FORM_MATCHER);
            if (p1 != NULL)
            {
                is_cgi = 0;
                update_info = (Update_p)malloc(sizeof(Update_t));
                memset(update_info, 0, sizeof(Update_t));
                req_para.update_info = update_info;
                p1 = strstr(content_type, BOUNDARY_MATCHER);
                if (p1 != NULL)
                {
                    p2 = p1 + boundary_matcher_len;
                    boundary_len = strlen(p2);
                    boundary = malloc(boundary_len);
                    if (boundary != NULL)
                    {
                        /** 获取到boundary*/
                        memcpy(boundary, p2, boundary_len);
                        printf("my boundary: %.*s\n", boundary_len, boundary);
                        /** 拼接结束分割线*/
                        finish_boundary = malloc(boundary_len + 6);
                        memcpy(finish_boundary, "\r\n--", strlen("\r\n--"));
                        memcpy(finish_boundary + 4, boundary, boundary_len);
                        memcpy(finish_boundary + 4 + boundary_len, "--", strlen("--"));
                        finish_boundary_len = boundary_len + 6;
                        last_frame_tail_len = finish_boundary_len - 1;

                        /** 计时器，控制超时*/
                        time_cnt_sec = get_second_sys_time();
                        while (1)
                        {
                            //printf("time now1 %lld--------------------------------------------############ \n", esp_timer_get_time());
                            ret = httpd_req_recv(req, buf, sizeof(buf));
                            //printf("ffbuf %.*s \n", ret, buf);
                            //printf("recv ret %d--------------------------------------------############ \n", ret);
                            // if (ret == HTTPD_SOCK_ERR_TIMEOUT || get_second_sys_time() - time_cnt_sec > 300)
                            if (get_second_sys_time() - time_cnt_sec > 300) //1800 //300
                            {
                                httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "recv post file timeout, 404");
                                goto OK;
                            }
                            if (ret <= 0)
                                continue;

                            frame = realloc(frame, frame_len + ret);
                            memcpy(frame + frame_len, buf, ret);
                            frame_len = frame_len + ret;
                            //printf("time now22 %lld--------------------------------------------############ \n", esp_timer_get_time());
                            switch (multipart_state)
                            {
                            case MULTIPART_WAIT_START:
                                /** 文件至少1024字节*/
                                if (frame_len > 1024)
                                {
                                    /** compute real file length******************************start*/
                                    char *cp1 = NULL;
                                    char *cp2 = NULL;
                                    cp1 = strstr(frame, "\r\n\r\n");
                                    if (cp1 != NULL)
                                    {
                                        declare_file_len = (int)req->content_len - (int)(cp1 - frame) - (boundary_len + 12);
                                        printf("content len: %d ****************\n", (int)req->content_len);
                                        printf("real file len: %d ****************\n", declare_file_len);
                                    }
                                    /** compute real file length*****************************finish*/

                                    p1 = memmem(frame, frame_len, FILENAME_MATCHER, filename_matcher_len);
                                    if (p1 != NULL)
                                    {
                                        p2 = memmem(p1, frame + frame_len - p1, FILENAME_END_MATCHER, strlen(FILENAME_END_MATCHER));
                                        if (p2 != NULL)
                                        {
                                            p1 = p1 + filename_matcher_len;

                                            printf("filename s%s e%s \n", p2, p1);
                                            memcpy(req_para.update_info->file_name, p1, p2 - p1);
                                            printf("ffname %s \n", req_para.update_info->file_name);
                                            update_info_gen();
                                            if (update_info->file_type == -1)
                                            {
                                            }

                                            /** 已获取到文件名*/
                                            printf("my filename: %s\n", req_para.update_info->file_name);

                                            p1 = memmem(p2, frame + frame_len - p2, DOUBLELINE_MATCHER, doubleline_matcher_len);
                                            if (p1 != NULL)
                                            {
                                                p2 = p1 + doubleline_matcher_len;
                                                len1 = frame + frame_len - p2;
                                                // p1 = memmem(p2, frame + frame_len - p2, finish_boundary, finish_boundary_len);
                                                //ESP_LOGE("RECV FILE START", "%.*s\n", len1, p2);
                                                printf("RECV FILE START %d \n", len1);
                                                /** 存到文件：pmu除外*/
                                                if (update_info->file_type >= 0)
                                                {
                                                    printf("write fopen ...%d %s\n", update_info->file_type, update_info->file_path);
                                                    if (update_info->file_type > 0)
                                                        update_file = fopen("/home/inv.bin", "wb"); //fopen(update_info->file_path, "wb");
                                                    else
                                                        update_file = fopen(update_info->file_path, "wb");

                                                    if (update_file == NULL)
                                                    {
                                                        printf("updatefile fopen fail \n");
                                                        force_flash_reformart("storage");
                                                    }
                                                    nwrite = fwrite(p2, 1, len1 - last_frame_tail_len, update_file);
                                                    if (nwrite != len1 - last_frame_tail_len)
                                                    {
                                                        printf("update write file error %d/%d\n", nwrite, len1);
                                                        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "write file header error, 505");
                                                        force_flash_reformart("storage");
                                                        sleep(2);
                                                        sent_newmsg();
                                                    }
                                                }
                                                else if (update_info->file_type == -1)
                                                {
                                                    /** 无效文件名*/
                                                    goto RESPONSE_PROCESSING;
                                                }
                                                file_len += len1 - last_frame_tail_len;
                                                printf("file len start: %d\n", file_len);
                                                /** 重新开始接收下一帧*/
                                                last_frame_tail = malloc(last_frame_tail_len);
                                                memcpy(last_frame_tail, p2 + len1 - last_frame_tail_len, last_frame_tail_len);

                                                frame_len = 0;
                                                multipart_state = MULTIPART_WAIT_FINISH;
                                            }
                                        }
                                    }
                                }
                                break;
                            case MULTIPART_WAIT_FINISH:
                                //printf("time now3 %lld--------------------------------------------############ \n", esp_timer_get_time());
                                search_box_len = last_frame_tail_len + frame_len;
                                search_box = realloc(search_box, search_box_len);
                                memcpy(search_box, last_frame_tail, last_frame_tail_len);
                                memcpy(search_box + last_frame_tail_len, frame, frame_len);
                                /** 只需对新增的ret个进行匹配*/
                                p3 = search_box + search_box_len - finish_boundary_len - ret;
                                len1 = finish_boundary_len + ret;
                                //printf("------------------------len1 %d/%d\n", finish_boundary_len, ret);
                                p1 = memmem(p3, len1, finish_boundary, finish_boundary_len);
                                /** 发现结束分割线*/
                                if (p1 != NULL)
                                {
                                    p2 = search_box + last_frame_tail_len;
                                    len1 = p1 - search_box;
                                    // printf("RECV FILE FINISH: %.*s\n", len1, p2);
                                    printf("RECV FILE FINISH:  %d \n", len1);
                                    /** 存到文件*/
                                    if (update_file != NULL)
                                    {
                                        nwrite = fwrite(search_box, 1, len1, update_file);
                                        if (nwrite != len1)
                                        {
                                            printf("[ err ] write file bytes: %d/%d\n", nwrite, len1);
                                            httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "write file end error, 505");
                                        }
                                        nwrite = fclose(update_file);
                                        if (nwrite != 0)
                                        {
                                            printf("[ err ] file close: %d\n", nwrite);
                                        }
                                    }

                                    file_len += len1;
                                    printf("file len finish: %d\n", file_len);
                                    if (declare_file_len > 0)
                                    {
                                        printf("%d%%\n", (int)(100 * file_len / declare_file_len));
                                    }

                                    /** 写入OTA分区，升级PMU*************************************************/
                                    if (update_info->file_type == 0)
                                    {
                                        printf("file path: %s\n", update_info->file_path);
                                        printf("file type: %d\n", update_info->file_type);
                                        check = sha256_file_check(update_info->file_path);
                                        if (check == 0)
                                        {
                                            char *ss = "update monitor begin";
                                            httpd_resp_send(req, ss, strlen(ss));
                                            printf("native upgrade wifi stick start: ...\n");
                                            update_pmu();
                                            esp_restart();
                                        }
                                    }
                                    /** master update*/
                                    else if (update_info->file_type == 1)
                                    {
                                        fdbg_msg_t fdbg_msg = {0};
                                        fdbg_msg.type = MSG_UPDATE_INV_MASTER;
                                        httpd_resp_send(req, "update inverter begin", strlen("update inverter begin"));
                                        if (send_cgi_msg(30, update_info->file_path, strlen(update_info->file_path), NULL) == 0)
                                        {
                                            printf("[ ok ] MSG_UPDATE_INV_MASTER send to inv\n");
                                        }
                                        else
                                        {
                                            printf("[ err ] MSG_UPDATE_INV_MASTER send to inv\n");
                                        }
                                    }
                                    /** safety update*/
                                    else if (update_info->file_type == 4)
                                    {
                                        fdbg_msg_t fdbg_msg = {0};
                                        fdbg_msg.type = MSG_UPDATE_INV_SAFETY;
                                        httpd_resp_send(req, "update inverter begin", strlen("update inverter begin"));
                                        if (send_cgi_msg(30, update_info->file_path, strlen(update_info->file_path), NULL) == 0)
                                        {
                                            printf("[ ok ] MSG_UPDATE_INV_SAFETY send to inv\n");
                                        }
                                        else
                                        {
                                            printf("[ err ] MSG_UPDATE_INV_SAFETY send to inv\n");
                                        }
                                    }

                                    multipart_state = MULTIPART_WAIT_START;
                                    goto RESPONSE_PROCESSING;
                                    break;
                                }
                                /** 纯内容，不含结束分割线*/
                                if (frame_len >= 1000)
                                {
                                    //printf("time now4 %lld--------------------------------------------############ \n", esp_timer_get_time());
                                    /** 非结束内容，保存到文件*/
                                    //ESP_LOGI("RECV FILE MIDDLE", "%.*s\n", frame_len, frame);
                                    printf("RECV FILE MIDDLE %d \n", frame_len);
                                    file_len += frame_len;
                                    printf("file len:%d %d\n", frame_len, file_len);
                                    if (declare_file_len > 0)
                                    {
                                        printf("%d%%\n", (int)(100 * file_len / declare_file_len));
                                    }
                                    if (update_file != NULL)
                                    {
                                        //printf("time now8 %lld--------------------------------------------############ \n", esp_timer_get_time());
                                        nwrite = fwrite(search_box, 1, frame_len, update_file);
                                        //printf("time now9 %lld--------------------------------------------############ \n", esp_timer_get_time());
                                        if (nwrite != frame_len)
                                        {
                                            perror("write file recfile");
                                            printf("[ err ] write file bytes: %d/%d\n", nwrite, frame_len);
                                            httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "write file content error, 505");
                                        }
                                    }
                                    /** 重新开始接收下一帧:
                                     * @记住上一帧的尾部， 下次匹配要用
                                    */
                                    p1 = frame + frame_len - last_frame_tail_len;
                                    memcpy(last_frame_tail, p1, last_frame_tail_len);
                                    //printf("time now5 %lld--------------------------------------------############ \n", esp_timer_get_time());
                                    frame_len = 0;
                                    continue;
                                }
                                break;
                            default:
                                break;
                            }
                        }
                    }
                }
                goto RESPONSE_PROCESSING;
            }
        }
    }

    /** 非文件CGI*/
    data = malloc(req->content_len);
    decode_data = malloc(req->content_len);
    memset(decode_data, 0, req->content_len);
    while (remaining > 0)
    {
        if ((ret = httpd_req_recv(req, buf,
                                  MIN(remaining, sizeof(buf)))) <= 0)
        {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT)
            {
                httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "request cgi timeout, 404");
                goto OK;
            }
            // usleep(10000);
            continue;
        }

        memcpy(data + data_len, buf, ret);
        data_len += ret;
        remaining -= ret;
    }
    ESP_LOGE(TAG, "===== RECEIVED POST BEGIN =====");
    printf("%.*s\n", data_len, data);
    ESP_LOGE(TAG, "===== RECEIVED POST END =======");

#ifdef ENCODE
    /** body解密*/
    Setting_Para setting = {0};
    general_query(NVS_ATE, &setting);

    char key[50] = {0};
    strncpy(key, setting.psn, sizeof(key));

    time_t t = time(NULL);
    struct tm currtime;
    localtime_r(&t, &currtime);
    currtime.tm_year += 1900;
    currtime.tm_mon += 1;
    snprintf(key, sizeof(key), "%s%02d%02d", setting.psn, currtime.tm_mon, currtime.tm_mday);

    printf("Key=%s\n", key);
    printf("Raw Len=%d\n", data_len);

    Custom_AES128_ECB_decrypt(data, data_len, decode_data, key);
    for (i = 0; i < data_len; i++)
    {
        printf("Raw body byte[%d]: %02X\n", i, *(uint8_t *)(&data[i]));
    }
    printf("Raw body: %.*s\n", (int)data_len, data);
    printf("Decode body: %.*s\n", (int)decode_len, decode_data);

    /** 传参，以待下一步处理*/
    req_para.body = decode_data;
    req_para.body_len = decode_len;
#else
    /** 传参，以待下一步处理*/
    req_para.body = data;
    req_para.body_len = data_len;
#endif

    /** 自定义响应，根据body内容 */
RESPONSE_PROCESSING:
    req_num = sizeof(http_req_tab) / sizeof(http_req_tab_t);

    for (i = 0; i < req_num; i++)
    {
        if (strstr(req->uri, http_req_tab[i].cgi) != NULL)
            break;
        if (i == req_num - 1)
        {
            printf("[ err ] No this http request: %s\n", req->uri);
            httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "request not found, 404");
            goto OK;
        }
    }
    /** 响应头中加入自定义键值对 */
    httpd_resp_set_hdr(req, "Connection", "close");
    httpd_resp_set_hdr(req, "Cache-Control", "no-cache");
    (*http_req_tab[i].hook)(req, &req_para);

OK:
    if (update_file != NULL)
        fclose(update_file);
    free(data);
    free(decode_data);
    free(content_type);
    free(boundary);
    free(finish_boundary);
    free(frame);
    free(last_frame_tail);
    free(search_box);
    free(update_info);
    /** 全局指针，释放后必须赋手动为NULL*/
    update_info = NULL;

    printf("mem left: %d\n", heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
    printf("post handle finished\n");
    return ESP_OK;
}

const httpd_uri_t asw_post_callback = {
    .uri = "/*",
    .method = HTTP_POST,
    .handler = asw_post_handler,
    .user_ctx = NULL};

/** 开启http服务器 ******************************************/
// static httpd_handle_t start_webserver(void)
// {
//     httpd_handle_t server = NULL;
//     httpd_config_t config = HTTPD_DEFAULT_CONFIG();

//     // Start the httpd server
//     ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
//     if (httpd_start(&server, &config) == ESP_OK)
//     {
//         // Set URI handlers
//         ESP_LOGI(TAG, "Registering URI handlers");
//         httpd_register_uri_handler(server, &asw_get_callback);
//         httpd_register_uri_handler(server, &asw_post_callback);
//         return server;
//     }

//     ESP_LOGI(TAG, "Error starting server!");
//     return NULL;
// }

static void stop_webserver(httpd_handle_t server)
{
    httpd_stop(server);
}

/** 断网时，释放服务器 */
// static void disconnect_handler(void *arg, esp_event_base_t event_base,
//                                int32_t event_id, void *event_data)
// {
//     httpd_handle_t *server = (httpd_handle_t *)arg;
//     if (*server)
//     {
//         ESP_LOGI(TAG, "Stopping webserver");
//         stop_webserver(*server);
//         *server = NULL;
//     }
// }

/** 来网时，启动服务器 */
// static void connect_handler(void *arg, esp_event_base_t event_base,
//                             int32_t event_id, void *event_data)
// {
//     httpd_handle_t *server = (httpd_handle_t *)arg;
//     if (*server == NULL)
//     {
//         ESP_LOGI(TAG, "Starting webserver");
//         *server = start_webserver();
//     }
// }

/** ********************************************************************************************/
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"

/* The examples use WiFi configuration that you can set via project configuration menu

   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/

#define EXAMPLE_ESP_WIFI_SSID "zzzZ"
#define EXAMPLE_ESP_WIFI_PASS "aass123456789"
#define EXAMPLE_ESP_MAXIMUM_RETRY 5

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0

// static const char *TAG = "wifi station";

static int s_retry_num = 0;
//static char my_sta_ip[16] = {0};
// static void event_handler(void *arg, esp_event_base_t event_base,
//                           int32_t event_id, void *event_data)
// {
//     if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
//     {
//         printf("event_handler(void *arg, esp_event_base_t event_base \n");
//         esp_wifi_connect();
//     }
//     else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
//     {
//         xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
//         ESP_LOGI(TAG, "connect to the AP fail");
//     }
//     else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
//     {
//         ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
//         ESP_LOGI(TAG, "got ip:%s",
//                  ip4addr_ntoa(&event->ip_info.ip));

//         printf("   our_ipaddr  = %s\n", ip4addr_ntoa(&event->ip_info.ip));
//         printf("   his_ipaddr  = %s\n", ip4addr_ntoa(&event->ip_info.gw));
//         printf("   netmask     = %s\n", ip4addr_ntoa(&event->ip_info.netmask));

//         memset(my_sta_ip, 0, sizeof(my_sta_ip));
//         strcpy(my_sta_ip, ip4addr_ntoa(&event->ip_info.ip));

//         memset(my_sta_gw, 0, sizeof(my_sta_gw));
//         strcpy(my_sta_gw, ip4addr_ntoa(&event->ip_info.gw));

//         memset(my_sta_mk, 0, sizeof(my_sta_mk));
//         strcpy(my_sta_mk, ip4addr_ntoa(&event->ip_info.netmask));

//         s_retry_num = 0;
//         xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
//     }
//     else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_SCAN_DONE)
//     {
//         scan_done = true;
//     }
// }

// void wifi_init_sta()
// {
//     s_wifi_event_group = xEventGroupCreate();

//     tcpip_adapter_init();
//     //ESP_ERROR_CHECK(esp_event_loop_create_default());
//     ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));

//     wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
//     ESP_ERROR_CHECK(esp_wifi_init(&cfg));

//     ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
//     ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

//     wifi_sta_para_t sta_para = {0};
//     general_query(NVS_STA_PARA, &sta_para);

//     wifi_config_t wifi_config = {0};
//     memcpy(wifi_config.sta.ssid, sta_para.ssid, sizeof(wifi_config.sta.ssid));
//     memcpy(wifi_config.sta.password, sta_para.password, sizeof(wifi_config.sta.password));
//     memcpy(sta_save_ssid, sta_para.ssid, sizeof(wifi_config.sta.ssid));
//     // memcpy(wifi_config.sta.ssid, "realmex", sizeof("realmex"));
//     // memcpy(wifi_config.sta.password, "12345678", sizeof("12345678"));

//     // memcpy(wifi_config.sta.ssid, "HUAWEI-B310-9290", sizeof("HUAWEI-B310-9290"));
//     // memcpy(wifi_config.sta.password, "QAYH321MF98", sizeof("QAYH321MF98"));

//     // memcpy(wifi_config.sta.ssid, "WiFi-stick华为荣耀", sizeof("WiFi-stick华为荣耀"));
//     // memcpy(wifi_config.sta.password, "69377272", sizeof("69377272"));

//     // memcpy(wifi_config.sta.ssid, "zzzZ", sizeof("zzzZ"));
//     // memcpy(wifi_config.sta.password, "aass123456789", sizeof("aass123456789"));

//     ESP_LOGE("my ssid", "%s", wifi_config.sta.ssid);
//     ESP_LOGE("my password", "%s", wifi_config.sta.password);

//     ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
//     ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
//     ESP_ERROR_CHECK(esp_wifi_start());
//     //esp_wifi_connect();
//     ESP_LOGI(TAG, "wifi_init_sta finished.");

//     /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
//      * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
//     // EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
//     //                                        WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
//     //                                        pdFALSE,
//     //                                        pdFALSE,
//     //                                        10000);

//     /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
//      * happened. */
//     // if (bits & WIFI_CONNECTED_BIT)
//     // {
//     //     ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
//     //              wifi_config.sta.ssid, wifi_config.sta.password);
//     // }
//     // else if (bits & WIFI_FAIL_BIT)
//     // {
//     //     ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
//     //              wifi_config.sta.ssid, wifi_config.sta.password);
//     // }
//     // else
//     // {
//     //     ESP_LOGE(TAG, "UNEXPECTED EVENT");
//     // }

//     // ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler));
//     // ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler));
//     // vEventGroupDelete(s_wifi_event_group);
// }

/////////////////////////////////////////////////////////////////////////////////////
/** sta_http_server_run()内含有以下初始化：
     * 1.tcp ini
     * 2.event loop start
     * 3.sta wifi connect
     * 4.sta websever start
     */
// void sta_http_server_run()
// {
//     esp_err_t ret = nvs_flash_init();
//     if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
//     {
//         ESP_ERROR_CHECK(nvs_flash_erase());
//         ret = nvs_flash_init();
//     }
//     ESP_ERROR_CHECK(ret);

//     ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
//     wifi_init_sta();
//     ////////////////////////////////////////////////////////////////////////////////
//     static httpd_handle_t server = NULL;

//     ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &connect_handler, &server));
//     ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &disconnect_handler, &server));

//     /* Start the server for the first time */
//     server = start_webserver();
//     // while (1)
//     // {
//     //     wifi_ap_record_t my_ap_info = {0};
//     //     esp_wifi_sta_get_ap_info(&my_ap_info);
//     //     memset(my_rssi, 0, sizeof(my_rssi));
//     //     sprintf(my_rssi, "rssi: %d", my_ap_info.rssi);
//     //     sleep(1);
//     // }
//     //sprintf(my_rssi, "%d", -22);
// }

// int get_wifi_connect_status(void)
// {
//     EventBits_t var;
//     var = xEventGroupGetBits(s_wifi_event_group);
//     if (var & WIFI_CONNECTED_BIT)
//     {
//         return 0;
//     }
//     else
//     {
//         return -1;
//     }
// }

// int get_sta_ip(char *ip)
// {
//     if (get_wifi_connect_status() == 0)
//     {
//         memcpy(ip, my_sta_ip, strlen(my_sta_ip));
//         return 0;
//     }
//     else
//     {
//         return -1;
//     }
// }

// int get_wifi_connect_status_2(void)
// {
//     wifi_ap_record_t ap_info = {0};
//     int res = esp_wifi_sta_get_ap_info(&ap_info);
//     if (ESP_OK == res)
//         return 0;
//     else
//     {
//         sleep(3);
//         return -1;
//     }
// }

// int check_wifi_reconnect(void)
// {
//     if (get_wifi_connect_status() < 0)
//     {
//         //printf("wifi missing, reconnect ...\n");
//         esp_wifi_connect();
//     }
// }

// void wifi_sta_cfg_print(void)
// {
//     // wifi_config_t read_wifi_config = {0};
//     // //esp_wifi_get_config(ESP_IF_WIFI_STA, &read_wifi_config);
//     // printf("read [ssid]: %s\n", read_wifi_config.sta.ssid);
//     // printf("read [password]: %s\n", read_wifi_config.sta.password);
// }

// void wifi_sta_cfg_set_invalid(void)
// {
//     wifi_config_t cfg = {
//         .sta = {
//             .ssid = "000",
//             .password = "kkk"},
//     };
//     ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &cfg));
// }

void asw_nvs_init(void)
{
    esp_err_t ret = nvs_flash_init_partition("my_nvs");
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase_partition("my_nvs"));
        ret = nvs_flash_init_partition("my_nvs");
    }
    ESP_ERROR_CHECK(ret);
}

void asw_nvs_clear(void)
{
    nvs_flash_deinit_partition("my_nvs");
    ESP_ERROR_CHECK(nvs_flash_erase_partition("my_nvs"));
    asw_nvs_init();
}

void factory_initial(void)
{
    clear_file_system();
    factory_reset_nvs();
    esp_restart();
}