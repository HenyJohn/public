/* MQTT over SSL Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include "asw_mqtt.h"
#include "utility.h"

//------------------------------------------//
int netework_state = 1;  //0-网络连接正常,1-网络链接异常
static int8_t asw_mqtt_app_state_code = -1; //-1-无状态，1-初始完毕，2-connect,3-disconnet,4-stop,5-free

static const char *TAG = "asw_mqtt.c";
char pub_topic[150] = "/a1tqX123itk/B80052030075/user/Action";
char sub_topic_get[150] = "/a1tqX123itk/B80052030075/user/get";
char sub_topic[150] = "/sys/a1tqX123itk/B80052030075/rrpc/request/+";

char client_id[150] = "G123456789EF&a1xAam8NEec|timestamp=123456,securemode=2,signmethod=hmacsha1,lan=C|";
char username[64] = "G123456789EF&a1xAam8NEec";
char password[65] = "6D09E41CCE71B739557F11BA6C2F26987FCF043BCED9A41CBDCC4107390360C8";
char my_uri[100] = "mqtts://a1xAam8NEec.iot-as-mqtt.cn-shanghai.aliyuncs.com:1883";

char cert[] = "-----BEGIN CERTIFICATE-----\nMIIDdTCCAl2gAwIBAgILBAAAAAABFUtaw5QwDQYJKoZIhvcNAQEFBQAwVzELMAkG\nA1UEBhMCQkUxGTAXBgNVBAoTEEdsb2JhbFNpZ24gbnYtc2ExEDAOBgNVBAsTB1Jv\nb3QgQ0ExGzAZBgNVBAMTEkdsb2JhbFNpZ24gUm9vdCBDQTAeFw05ODA5MDExMjAw\nMDBaFw0yODAxMjgxMjAwMDBaMFcxCzAJBgNVBAYTAkJFMRkwFwYDVQQKExBHbG9i\nYWxTaWduIG52LXNhMRAwDgYDVQQLEwdSb290IENBMRswGQYDVQQDExJHbG9iYWxT\naWduIFJvb3QgQ0EwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQDaDuaZ\njc6j40+Kfvvxi4Mla+pIH/EqsLmVEQS98GPR4mdmzxzdzxtIK+6NiY6arymAZavp\nxy0Sy6scTHAHoT0KMM0VjU/43dSMUBUc71DuxC73/OlS8pF94G3VNTCOXkNz8kHp\n1Wrjsok6Vjk4bwY8iGlbKk3Fp1S4bInMm/k8yuX9ifUSPJJ4ltbcdG6TRGHRjcdG\nsnUOhugZitVtbNV4FpWi6cgKOOvyJBNPc1STE4U6G7weNLWLBYy5d4ux2x8gkasJ\nU26Qzns3dLlwR5EiUWMWea6xrkEmCMgZK9FGqkjWZCrXgzT/LCrBbBlDSgeF59N8\n9iFo7+ryUp9/k5DPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNVHRMBAf8E\nBTADAQH/MB0GA1UdDgQWBBRge2YaRQ2XyolQL30EzTSo//z9SzANBgkqhkiG9w0B\nAQUFAAOCAQEA1nPnfE920I2/7LqivjTFKDK1fPxsnCwrvQmeU79rXqoRSLblCKOz\nyj1hTdNGCbM+w6DjY1Ub8rrvrTnhQ7k4o+YviiY776BQVvnGCv04zcQLcFGUl5gE\n38NflNUVyRRBnMRddWQVDf9VMOyGj/8N7yy5Y0b2qvzfvGn9LhJIZJrglfCm7ymP\nAbEVtQwdpf5pLGkkeB6zpxxxYu7KyJesF12KwvhHhm4qxFYxldBniYUr+WymXUad\nDKqC5JlR3XC321Y9YeRq4VzW9v493kHMB65jUr9TU/Qr6cf9tveCX4XSQRjbgbME\nHMUfpIBvFSDJ3gyICh3WZlXi/EjJKSZp4A==\n-----END CERTIFICATE-----";
int g_state_mqtt_connect = -1;
EXT_RAM_ATTR char rrpc_res_topic[256] = {0};
unsigned int mqtt_pub_res = 0;
char DEMO_PRODUCT_KEY[IOTX_PRODUCT_KEY_LEN + 1] = "a1tqX123itk";
char DEMO_DEVICE_NAME[IOTX_DEVICE_NAME_LEN + 1] = "B80052030075";
char DEMO_DEVICE_SECRET[IOTX_DEVICE_SECRET_LEN + 1] = "VrRCgge0PTkZesAVyGguPJrMIA4Tr4ME"; //"JDuE0bUeFjcfQucPtvnJEk5vcVcD5E2L";
                                                                                          //"VrRCgge0PTkZesAVyGguPJrMIA4Tr4ME";
EXT_RAM_ATTR static esp_mqtt_client_handle_t asw_mqtt_client = NULL;
//------------------------------------------//

void set_mqtt_server(char *pdk, char *server, int port)
{
    snprintf(my_uri, sizeof(my_uri), "mqtts://%s.iot-as-mqtt.%s.aliyuncs.com:%d", pdk, server, port);
    ESP_LOGI(TAG, "mqtt server %s \n", my_uri);
}

// void set_mqtt_server(char *server, int port)
// {
//     /* uri 改为全拼 */
//     if (strlen(server) == 0)
//     {
//         ESP_LOGW("mqtt server error", "The server is not setting!!!");
//         return;
//     }

//     snprintf(my_uri, sizeof(my_uri), "mqtts://%s:%d", server, port);
//     // snprintf(my_uri, sizeof(my_uri), "%s", server);
//     ESP_LOGI(TAG, "mqtt server:%s \n", my_uri);
// }
// //------------------------------------------//
void set_product_key(char *product_key)
{
    memset(DEMO_PRODUCT_KEY, 0, sizeof(DEMO_PRODUCT_KEY));
    strncpy(DEMO_PRODUCT_KEY, product_key, sizeof(DEMO_PRODUCT_KEY));
}
//------------------------------------------//
void set_device_name(char *device_name)
{
    memset(DEMO_DEVICE_NAME, 0, sizeof(DEMO_DEVICE_NAME));
    strncpy(DEMO_DEVICE_NAME, device_name, sizeof(DEMO_DEVICE_NAME));
}
//---------------------------------------------//
void set_device_secret(char *device_secret)
{
    memset(DEMO_DEVICE_SECRET, 0, sizeof(DEMO_DEVICE_SECRET));
    strncpy(DEMO_DEVICE_SECRET, device_secret, sizeof(DEMO_DEVICE_SECRET));
}

//-------------------------------------//
void newparaget(void)
{
    cJSON *res = cJSON_CreateObject();
    char *msg = NULL;
    char buf[20] = {0};

    Setting_Para setting = {0};

    general_query(NVS_ATE, &setting);
    ESP_LOGE("ATE", "para get %d %d \n", setting.meter_add, setting.typ);
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
    cJSON_AddNumberToObject(res, "status", 1);

    cJSON_AddStringToObject(res, "ali_ip", setting.host);
    cJSON_AddNumberToObject(res, "ali_port", setting.port);
    cJSON_AddNumberToObject(res, "meter_en", setting.meter_en);
    cJSON_AddNumberToObject(res, "meter_add", setting.meter_add);
    cJSON_AddNumberToObject(res, "meter_mod", setting.meter_mod);

    msg = cJSON_PrintUnformatted(res);
    printf("newsss set: %s\n", msg);

    cJSON_Delete(res);
    free(msg); // [ mark]add
}
//==========================================
void setting_new_server(cJSON *body)
{
    cJSON *res = cJSON_CreateObject();
    char *msg = NULL;
    // char buf[20] = {0};

    Setting_Para setting = {0};

    general_query(NVS_ATE, &setting);
    ESP_LOGI("ATE", "org para %d %d \n", setting.meter_add, setting.typ);
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

    cJSON_AddStringToObject(res, "pdk", setting.product_key);
    cJSON_AddStringToObject(res, "ser", setting.device_secret);
    cJSON_AddNumberToObject(res, "status", 1);

    cJSON_AddStringToObject(res, "ali_ip", setting.host);
    cJSON_AddNumberToObject(res, "ali_port", setting.port);
    cJSON_AddNumberToObject(res, "meter_en", setting.meter_en);
    cJSON_AddNumberToObject(res, "meter_add", setting.meter_add);
    cJSON_AddNumberToObject(res, "meter_mod", setting.meter_mod);

    msg = cJSON_PrintUnformatted(res);
    printf("org set: %s\n", msg);

    cJSON *json;
    printf("new server setting %s\n", (char *)body);
    json = body; // cJSON_Parse(body);
    if (json == NULL)
        return; //// return -1; [ mark]

    memset(&setting.host, 0, sizeof(setting.host));
    memset(&setting.product_key, 0, sizeof(setting.product_key));
    memset(&setting.device_secret, 0, sizeof(setting.device_secret));

    getJsonStr(setting.host, "host", sizeof(setting.host), json);
    getJsonStr(setting.product_key, "pdk", sizeof(setting.product_key), json);
    getJsonStr(setting.device_secret, "sec", sizeof(setting.device_secret), json);

    general_add(NVS_ATE, &setting);

    ESP_LOGE("ATE", "new paras %s %s %s \n", setting.host, setting.product_key, setting.device_secret);

    cJSON_Delete(res);
    free(msg); // [ mark]add

    newparaget(); //[ mark]打印数据，没有产生是实际的结果
}

//--------------------------------
void send_msg(int type, char *buff, int lenthg, char *ws)
{
    cloud_inv_msg buf;
    // int ret;
    // int id = msgget(0x6789, 0);

    memset(&buf, 0, sizeof(buf));
    if (lenthg)
        memcpy(buf.data, buff, lenthg);
    if (ws)
        memcpy(&buf.data[lenthg], ws, strlen((char *)ws));
    buf.type = type;
    buf.len = lenthg;

    ESP_LOGI(TAG, "send msg %s %d \n", buf.data, lenthg);
    // ret = msgsnd(id, &buf, sizeof(buf.data), 0);
    if (mq1 != NULL)
    {
        xQueueSend(mq1, (void *)&buf, (TickType_t)0);
        ESP_LOGI(TAG, "send cloud msg ok\n");
    }
}

//-----------------------------
int sent_newmsg(void)
{
    char ws[65] = {0};
    char msg[32] = {0};
    memset(ws, 'U', 64);
    msg[0] = 0;
    ESP_LOGI(TAG, "rebootms %d ws %s \n", msg[0], ws);
    send_msg(99, msg, 1, ws);
    return 0;
}

//------------------------------------------//

int asw_mqtt_setup(void)
{
    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("MQTT_CLIENT", ESP_LOG_VERBOSE);
    esp_log_level_set("MQTT_EXAMPLE", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_TCP", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_SSL", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT", ESP_LOG_VERBOSE);
    esp_log_level_set("OUTBOX", ESP_LOG_VERBOSE);

    aiotMqttSign(DEMO_PRODUCT_KEY, DEMO_DEVICE_NAME, DEMO_DEVICE_SECRET, client_id, username, password);
    ESP_LOGI(TAG, "[client_id] %s\n", client_id);
    ESP_LOGI(TAG, "[username] %s\n", username);
    ESP_LOGI(TAG, "[password] %s\n", password);

    return 0;
}
//=================================//
int get_mqtt_pub_ack(void)
{
    return mqtt_pub_res;
}

//---------------------------------//
int parse_mqtt_msg(char *payload)
{
    cJSON *json;
    json = cJSON_Parse(payload);
    cJSON *cmd_ = cJSON_GetObjectItem(json, "Action");
    if (cmd_ == NULL || cmd_->type == cJSON_NULL)
    {
        printf("mqtt msg can't found Action \n");
        return -1;
    }
    char *function = cJSON_GetObjectItem(json, "Action")->valuestring;
    printf("ACTION STR %s \n", function);
    if (function != NULL)
    {
        //{"Action":"setscan","value":{"setscan":15}}
        if (!(strcmp(function, "Scan")))
        {
            // char ws=0;
            char u8msg[128] = {0};
            unsigned int cnt = 0;
            cJSON *item = cJSON_GetObjectItem(json, "value");

            printf("cnt int %d \n", cJSON_GetObjectItem(item, "setscan")->valueint);

            cnt = cJSON_GetObjectItem(item, "setscan")->valueint;
            int byteLen = 1;
            u8msg[0] = cnt;
            printf("cnt int %d  %s\n", cnt, u8msg);
            send_msg(5, u8msg, byteLen, NULL);
        }
        else if (!(strcmp(function, "Full-Trans")))
        {
            char ws[65] = {0};
            char msg[300] = {0};
            cJSON *item = cJSON_GetObjectItem(json, "ws");
            strncpy(ws, item->valuestring, 64);
            item = cJSON_GetObjectItem(json, "message");
            strncpy(msg, item->valuestring, 300);

            printf("ms %s ws %s \n", msg, ws);
            send_msg(2, msg, strlen(msg), ws);
        }
        else if (!(strcmp(function, "reboot"))) // 0, esp32;1,inv
        {
            char ws[65] = {0};
            char msg[300] = {0};
            // cJSON *item = cJSON_GetObjectItem(json, "ws");
            // strncpy(ws, item->valuestring, 64);
            // item = cJSON_GetObjectItem(json, "message");
            // strncpy(msg, item->valuestring, 300);
            memset(ws, 'T', 64);
            msg[0] = cJSON_GetObjectItem(json, "reboot")->valueint;
            printf("ms %d ws %s \n", msg[0], ws);
            send_msg(99, msg, 1, ws);
        }
        else if (!(strcmp(function, "setmeter")))
        {
            cJSON *meter = cJSON_GetObjectItem(json, "value");
            // cJSON_AddNumberToObject(meter, "date", get_current_days());
            // printf("meter set %s \n", cJSON_PrintUnformatted(cJSON_GetObjectItem(json, "value")));
            printf("meter set %s \n", cJSON_PrintUnformatted(meter));
            meter_setdata mt = {0};
            mt.enb = cJSON_GetObjectItem(meter, "enb")->valueint;
            mt.mod = cJSON_GetObjectItem(meter, "mod")->valueint;
            mt.reg = cJSON_GetObjectItem(meter, "regulate")->valueint;
            mt.target = cJSON_GetObjectItem(meter, "target")->valueint;

          /* kaco-lanstick 20220811 - */
            // int abs0 = 0;
            // int offs = 0;
            // getJsonNum(&abs0, "abs", meter);
            // printf("abs0: %d\n", abs0);
            // getJsonNum(&offs, "offset", meter);
            // printf("offs: %d\n", offs);

            // if (mt.reg == 10)
            // {
            //     mt.reg = 0X100;
            //     if (abs0 == 1 && mt.target == 0)
            //     {
            //         if (offs <= 0 || offs > 100)
            //             offs = 5;

            //         mt.reg |= offs;
            //     }
            // }

            mt.date = get_current_days();
            write_meter_file(&mt);
            sleep(1);

            send_msg(6, "clearmeter", 9, NULL);
            // esp_restart();
            // write_meter_file(cJSON_PrintUnformatted(meter));
        }
        else if (!(strcmp(function, "upgrade")))
        {
            cJSON *item = cJSON_GetObjectItem(json, "value");
            char up_type = cJSON_GetObjectItem(item, "type")->valueint;
            printf("update type %d \n", up_type);

            cJSON *item1 = cJSON_GetObjectItem(item, "path");
            char *url0 = cJSON_GetObjectItem(item1, "directory")->valuestring;
            printf("url %s \n", url0);

            cJSON *item2 = cJSON_GetObjectItem(item1, "files");
            printf("file type %d %d \n", item2->type, cJSON_GetArraySize(item2));
            printf("meter str %s \n", cJSON_GetArrayItem(item2, 0)->valuestring);
            char *p[cJSON_GetArraySize(item2)];

            update_url download_url = {0};

            if (cJSON_GetArraySize(item2) == 1)
            {
                p[0] = cJSON_GetArrayItem(item2, 0)->valuestring;

                if (up_type == 1) // item2->type == 1)
                {
                    download_url.update_type = 1;
                    sprintf(download_url.down_url, "%s/%s", url0, p[0]);
                }
                else
                {
                    download_url.update_type = 2;
                    sprintf(download_url.down_url, "%s/%s", url0, p[0]);
                }
            }

            printf("dowan_loadurl %s  %d %d\n", download_url.down_url, download_url.update_type, strlen(download_url.down_url));

            TaskHandle_t download_task_handle = NULL;
            BaseType_t xReturned;

            xReturned = xTaskCreate(
                download_task,          /* Function that implements the task. */
                "download task",        /* Text name for the task. */
                10240,                  /* Stack size in words, not bytes. 50K = 25600 */
                (void *)&download_url,  /* Parameter passed into the task. */
                6,                      /* Priority at which the task is created. */
                &download_task_handle); /* Used to pass out the created task's handle. */
            if (xReturned != pdPASS)
            {
                ESP_LOGE(TAG, "create cgi task failed.\n");
            }
        }
        else if (!(strcmp(function, "setserver")))
        {
            cJSON *new_server = cJSON_GetObjectItem(json, "value");

            ESP_LOGI(TAG, "get sub newset %s \n", cJSON_PrintUnformatted(new_server));

            setting_new_server(new_server); //[  mark] define in the ATE

            sleep(1);
            // set_external_reboot(); [ mark] => inv_com_reboot_flg=1
            inv_com_reboot_flg = 1;
        }
    }
    return 0;
}

void mqtt_client_destroy_free(void)
{
    if (asw_mqtt_client != NULL && asw_mqtt_app_state_code != 5)
    {
        ESP_LOGE(TAG, "mqtt_client_destroy_free  state is %d\n", asw_mqtt_app_state_code);
        // esp_mqtt_client_stop(asw_mqtt_client);
        esp_mqtt_client_destroy(asw_mqtt_client);
        g_state_mqtt_connect = -1;
        netework_state = 1;

        asw_mqtt_app_state_code = 5; //-1-无状态，1-初始完毕，2-connect,3-disconnet,4-stop,5-free
    }
}

//[  lan]add
void asw_mqtt_client_stop()
{
    if (asw_mqtt_client != NULL && asw_mqtt_app_state_code != 4)
    {
        ESP_LOGE(TAG, "mqtt_client_stop \n");
        // esp_mqtt_client_stop(asw_mqtt_client);
        esp_mqtt_client_stop(asw_mqtt_client);
        g_state_mqtt_connect = -1;
        netework_state = 1;

        asw_mqtt_app_state_code = 4; //-1-无状态，1-初始完毕，2-connect,3-disconnet,4-stop,5-free
    }
}

int replace_string(char *str, char *outstr, char *oldstr, char *newstr)
{
    char bstr[strlen(str)];
    memset(bstr, 0, sizeof(bstr));
    int i = 0;
    for (i = 0; i < strlen(str); i++)
    {
        if (!strncmp(str + i, oldstr, strlen(oldstr)))
        {
            strcat(bstr, newstr);
            i += strlen(oldstr) - 1;
        }
        else
        {
            strncat(bstr, str + i, 1);
        }
    }

    strcpy(outstr, bstr);
    return 0;
}

int get_rrpc_restopic(char *rpc_topic, int rpc_len, char *response_topic)
{
    // int res = 0;
    // char *payload = (char *)cpayload;
    // char *payload = "inverter scan has completed";
    char resp_topicx[256] = {0};
    char resp_topic[256] = {0};
    if (rpc_len > 256)
        return -1;

    memcpy(resp_topicx, rpc_topic, rpc_len);
    replace_string(resp_topicx, resp_topic, "request", "response");
    printf("response publish %s\n", resp_topic);
    // res=asw_mqtt_publish(resp_topic, (uint8_t *)payload, strlen(payload), 0);
    memcpy(response_topic, resp_topic, strlen(resp_topic));
    return 0;
}

//---------------------------------//

int parse_mqtt_msg_rrpc(char *rpc_topic, int rpc_len, void *payload, int data_len)
{
    char *recv_ok = "{\"dat\":\"ok\"}";
    char *recv_er = "{\"dat\":\"err\"}";
    if (strlen(payload) < 3)
        return -1;
    cJSON *json;
    json = cJSON_Parse(payload);

    char resp_topic[256] = {0};
    if (get_rrpc_restopic(rpc_topic, rpc_len, resp_topic) < 0)
    {
        printf("mqtt rrpc topic too long \n");
        asw_mqtt_publish(resp_topic, (char *)recv_er, strlen(recv_er), 0);
        return -1;
    }
    printf("RRPC ACTION STR \n"); // item = cJSON_GetObjectItem(root, "semantic");
    // char *function = cJSON_GetObjectItem(json, "setscan")->valueint;

    if (cJSON_HasObjectItem(json, "setscan") == 1 && cJSON_GetObjectItem(json, "setscan")->valueint > 0)
    {
        // char ws=0;
        char u8msg[128] = {0};
        unsigned int cnt = 10;

        int byteLen = 1;
        u8msg[0] = cnt;
        printf("cnt int %d  %s\n", cnt, u8msg);
        send_msg(5, u8msg, byteLen, NULL);
        asw_mqtt_publish(resp_topic, (char *)recv_ok, strlen(recv_ok), 0); // res scan
        return 0;
    }
    else if (cJSON_HasObjectItem(json, "fdbg") == 1 && strlen(cJSON_GetObjectItem(json, "fdbg")->valuestring) > 5)
    {
        char ws[65] = {0};
        char msg[300] = {0};
        // cJSON *item = cJSON_GetObjectItem(json, "ws");
        // strncpy(ws, item->valuestring, 64);
        // item = cJSON_GetObjectItem(json, "message");
        // strncpy(msg, item->valuestring, 300);
        memset(ws, 'R', 64);
        strncpy(msg, cJSON_GetObjectItem(json, "fdbg")->valuestring, 300);

        printf("ms %s ws %s \n", msg, ws);
        memcpy(rrpc_res_topic, resp_topic, strlen(resp_topic));
        send_msg(2, msg, strlen(msg), ws);
        return 0;
    }
    else if (cJSON_HasObjectItem(json, "reboot") == 1)
    {
        // int reboot_num=cJSON_GetObjectItem(json, "reboot")->valueint;
        char ws[65] = {0};
        char msg[300] = {0};
        // cJSON *item = cJSON_GetObjectItem(json, "ws");
        // strncpy(ws, item->valuestring, 64);
        // item = cJSON_GetObjectItem(json, "message");
        // strncpy(msg, item->valuestring, 300);
        memset(ws, 'U', 64);
        // strncpy(msg, cJSON_GetObjectItem(json, "fdbg")->valuestring, 300);
        msg[0] = cJSON_GetObjectItem(json, "reboot")->valueint;
        printf("ms %d ws %s \n", msg[0], ws);
        // memcpy(rrpc_res_topic, resp_topic, strlen(resp_topic));
        send_msg(99, msg, 1, ws);
        asw_mqtt_publish(resp_topic, (char *)recv_ok, strlen(recv_ok), 0);
        return 0;
    }
    else if (cJSON_HasObjectItem(json, "setmeter") == 1 && cJSON_GetObjectItem(cJSON_GetObjectItem(json, "setmeter"), "enb")->valueint > -1)
    {
        cJSON *meter = cJSON_GetObjectItem(json, "setmeter");
        // cJSON_AddNumberToObject(meter, "date", get_current_days());
        // printf("meter set %s \n", cJSON_PrintUnformatted(cJSON_GetObjectItem(json, "value")));
        printf("meter set %s \n", cJSON_PrintUnformatted(meter));
        meter_setdata mt = {0};
        mt.enb = cJSON_GetObjectItem(meter, "enb")->valueint;
        mt.mod = cJSON_GetObjectItem(meter, "mod")->valueint;
        mt.reg = cJSON_GetObjectItem(meter, "regulate")->valueint;
        mt.target = cJSON_GetObjectItem(meter, "target")->valueint;

        int abs0 = 0;
        int offs = 0;
        getJsonNum(&abs0, "abs", meter);
        printf("abs0: %d\n", abs0);
        getJsonNum(&offs, "offset", meter);
        printf("offs: %d\n", offs);

        if (mt.reg == 10)
        {
            mt.reg = 0X100;
            if (abs0 == 1 && mt.target == 0)
            {
                if (offs <= 0 || offs > 100)
                    offs = 5;

                mt.reg |= offs;
            }
        }

        mt.date = get_current_days();
        write_meter_file(&mt);
        sleep(1);
        send_msg(6, "clearmeter", 9, NULL);
        asw_mqtt_publish(resp_topic, (char *)recv_ok, strlen(recv_ok), 0);
        return 0;
        // esp_restart();
        // write_meter_file(cJSON_PrintUnformatted(meter));
    }
    else if (cJSON_HasObjectItem(json, "upgrade") == 1 && cJSON_GetObjectItem(cJSON_GetObjectItem(json, "upgrade"), "type")->valueint > 0)
    {
        cJSON *item = cJSON_GetObjectItem(json, "upgrade");
        char up_type = cJSON_GetObjectItem(item, "type")->valueint;
        printf("update type %d \n", up_type);

        // p2p dan.wang
        if (cJSON_HasObjectItem(item, "p2p") == 1)
        {
            g_p2p_mode = cJSON_GetObjectItem(item, "p2p")->valueint;
            printf("p2p %d \n", g_p2p_mode);
        }

        cJSON *item1 = cJSON_GetObjectItem(item, "path");
        char *url0 = cJSON_GetObjectItem(item1, "directory")->valuestring;
        printf("url %s \n", url0);

        cJSON *item2 = cJSON_GetObjectItem(item1, "files");
        printf("file type %d %d \n", item2->type, cJSON_GetArraySize(item2));
        printf("meter str %s \n", cJSON_GetArrayItem(item2, 0)->valuestring);
        char *p[cJSON_GetArraySize(item2)];

        update_url download_url = {0};

        if (cJSON_GetArraySize(item2) == 1)
        {
            p[0] = cJSON_GetArrayItem(item2, 0)->valuestring;

            if (up_type == 1) // item2->type == 1)
            {
                download_url.update_type = 1;
                sprintf(download_url.down_url, "%s/%s", url0, p[0]);
            }
            else
            {
                download_url.update_type = 2;
                sprintf(download_url.down_url, "%s/%s", url0, p[0]);
            }
        }
        asw_mqtt_publish(resp_topic, (char *)recv_ok, strlen(recv_ok), 0); // res update
        printf("dowan_loadurl %s  %d %d\n", download_url.down_url, download_url.update_type, strlen(download_url.down_url));
        printf("RRPCRESP %s  %s \n", resp_topic, recv_ok);

        TaskHandle_t download_task_handle = NULL;
        BaseType_t xReturned;
        xReturned = xTaskCreate(
            download_task,          /* Function that implements the task. */
            "download task",        /* Text name for the task. */
            10240,                  /* Stack size in words, not bytes. 50K = 25600 */
            (void *)&download_url,  /* Parameter passed into the task. */
            6,                      /* Priority at which the task is created. */
            &download_task_handle); /* Used to pass out the created task's handle. */
        if (xReturned != pdPASS)
        {
            ESP_LOGE(TAG, "create cgi task failed.\n");
        }
        return 0;
    }
    else if (cJSON_HasObjectItem(json, "factory_reset") == 1)
    {
        asw_mqtt_publish(resp_topic, (char *)recv_ok, strlen(recv_ok), 0);
        factory_initial();
        return 0;
    }
    else if (cJSON_HasObjectItem(json, "get_systime") == 1)
    {
        char now_timex[30] = {0};
        char ipx[30] = {0};
        char bufx[100] = {0};

        get_time(now_timex, sizeof(now_timex));

        if (g_stick_run_mode == Work_Mode_LAN)
            get_eth_ip(ipx);
        else if (g_stick_run_mode == Work_Mode_STA)
            get_sta_ip(ipx);
        sprintf(bufx, "%s ip:%s ", now_timex, ipx);

        asw_mqtt_publish(resp_topic, (char *)bufx, strlen(bufx), 0);
        return 0;
    }
    else if (cJSON_HasObjectItem(json, "get_invsts") == 1)
    {
        char bufx[60] = {0};
        sprintf(bufx, "inverter comm error %d ", inv_comm_error);

        asw_mqtt_publish(resp_topic, (char *)bufx, strlen(bufx), 0);
        return 0;
    }
    else if (cJSON_HasObjectItem(json, "get_syslog") == 1)
    {
        char buff[1024] = {0};
        // get_sys_log(bufx);
        get_all_log(buff);
        ESP_LOGI(TAG, "rrpcsyslog %d--%s \n", strlen(buff), buff);

        if (strlen(buff))
            asw_mqtt_publish(resp_topic, (char *)buff, strlen(buff), 0);
        else
            asw_mqtt_publish(resp_topic, "no log", strlen("no log"), 0);

        return 0;
    }
    else if (cJSON_HasObjectItem(json, "get_hostcom") == 1) //[  mark] hostcomm
    {
        char buf[128] = {0};
        char puf[128] = {0};
        get_hostcomm(buf, puf);

        char bufx[300] = {0};
        sprintf(bufx, "hcomm:%spcomm:%srsh:%d", buf, puf, get_rssi());

        if (strlen(buf))
            asw_mqtt_publish(resp_topic, (char *)bufx, strlen(bufx), 0);
        else
            asw_mqtt_publish(resp_topic, "no hcomm", strlen("no hcomm"), 0);

        return 0;
    }

    /* kaco-lanstick 20220811 - */
    // else if (cJSON_HasObjectItem(json, "setserver") == 1)
    // {
    //     cJSON *new_server = cJSON_GetObjectItem(json, "setserver");

    //     printf("new_server set %s \n", cJSON_PrintUnformatted(new_server));

    //     setting_new_server(new_server);

    //     asw_mqtt_publish(resp_topic, (char *)recv_ok, strlen(recv_ok), 0);

    //     inv_com_reboot_flg = 1; // set_external_reboot();

    //     return 0;
    // }
    // RECCCVV DATA={"setserver":{"host":"ap-southeast-1","pdk":"a25dhqu0kTj","sec":"b6517171d7cf1560054cb4be88fec76b",psn:""}}
    // printf("=========>>>>>>>>>>>>>%d \n", cJSON_HasObjectItem(json, "setserver"));
    asw_mqtt_publish(resp_topic, (char *)recv_er, strlen(recv_er), 0);
    return -1;
}

//------------------------------------//
//@return: -1 means failure
int asw_mqtt_publish(const char *topic, const char *data, int data_len, int qos)
{
    int msg_id = 0;
    msg_id = esp_mqtt_client_publish(asw_mqtt_client, topic, data, data_len, qos, 1);
    ESP_LOGI(TAG, "published, msg_id=%d ************", msg_id);
    return msg_id;
}
//-------------------------------------//
int asw_publish(void *cpayload)
{
    int res = 0;
    char *payload = (char *)cpayload;

    // asw_mqtt_publish(topic, payload, strlen(payload), ini_para.mqtt_qos);
    res = asw_mqtt_publish(pub_topic, payload, strlen(payload), 1);

    // ESP_LOGI(TAG, "published, xxxxxxxxxxxxxxxxxx=%d ************", res);

    if (res < 0)
    {
        ESP_LOGE(TAG, "publish failed, res = %d", res);
        return -1;
    }
    return res;
}
//--------------------------------------
void asw_mqtt_subscribe_receive(char *topic, int topic_len, char *data, int data_len)
{
    ESP_LOGI(TAG, "RECVVV  TOPIC=%.*s\r\n", topic_len, topic);
    ESP_LOGI(TAG, "RECCCVV DATA=%.*s\r\n", data_len, data);

    if (strstr(topic, "rrpc"))
        parse_mqtt_msg_rrpc(topic, topic_len, data, data_len); // int parse_mqtt_msg_rrpc(char * rpc_topic, int rpc_len, void *payload, int data_len)
    else
        parse_mqtt_msg(data);
}

//------------------------------------------//
static esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event)
{
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    // your_context_t *context = event->context;
    switch (event->event_id)
    {
        // sub:
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        msg_id = esp_mqtt_client_subscribe(client, sub_topic, 1);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

        msg_id = esp_mqtt_client_subscribe(client, sub_topic_get, 1);
        ESP_LOGI(TAG, "sent get subscribe successful, msg_id=%d", msg_id);

        g_state_mqtt_connect = 0;
        asw_mqtt_app_state_code = 2; //-1-无状态，1-初始完毕，2-connect,3-disconnet,4-stop,5-free

        break;

    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        g_state_mqtt_connect = -1;
        asw_mqtt_app_state_code = 3; //-1-无状态，1-初始完毕，2-connect,3-disconnet,4-stop,5-free

        break;

    case MQTT_EVENT_SUBSCRIBED:

        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        mqtt_pub_res = event->msg_id;
        break;

        // sub msg recv:
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        asw_mqtt_subscribe_receive(event->topic, event->topic_len, event->data, event->data_len);
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_ESP_TLS)
        {
            ESP_LOGI(TAG, "Last error code reported from esp-tls: 0x%x", event->error_handle->esp_tls_last_esp_err);
            ESP_LOGI(TAG, "Last tls stack error number: 0x%x", event->error_handle->esp_tls_stack_err);
        }
        else if (event->error_handle->error_type == MQTT_ERROR_TYPE_CONNECTION_REFUSED)
        {
            ESP_LOGI(TAG, "Connection refused error: 0x%x", event->error_handle->connect_return_code);
        }
        else
        {
            ESP_LOGW(TAG, "Unknown error type: 0x%x", event->error_handle->error_type);
        }
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
    return ESP_OK;
}
//--------------------------------//

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    mqtt_event_handler_cb(event_data);
}

//----------------------------------------//

int mqtt_app_start(void)
{
    const esp_mqtt_client_config_t mqtt_cfg = {
        .uri = my_uri,
        .cert_pem = (const char *)cert, // MQTT_ROOT_CA_FILENAME, //mqtt_eclipse_org_pem_start,
        .client_id = client_id,
        .username = username,
        .password = password,

    };

    if (strlen(my_uri) == 0)
    {
        ESP_LOGW("mqtt app start error", "The server is not setting!!!");
        return -1;
    }
    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());

    // esp_mqtt_client_handle_t
    asw_mqtt_client = esp_mqtt_client_init(&mqtt_cfg);

    // asw_mqtt_client = client;
    esp_mqtt_client_register_event(asw_mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, &asw_mqtt_client);

    asw_mqtt_app_state_code = 0; //-1-无状态，0-初始完毕，1-start， 2-connect,3-disconnet,4-stop,5-free

    ESP_LOGW(TAG, "MQTT CLIENT BEFORE START");
    int res = esp_mqtt_client_start(asw_mqtt_client);

    if (res == ESP_OK)
    {
        return 0;
    }
    else
        return -1;
}

void asw_mqtt_client_start()
{
    if (g_state_mqtt_connect == -1 && asw_mqtt_app_state_code != 1)
    {
        ESP_LOGW(TAG, "mqtt client start");

        mqtt_app_start();
        asw_mqtt_app_state_code = 1;
    }
}
