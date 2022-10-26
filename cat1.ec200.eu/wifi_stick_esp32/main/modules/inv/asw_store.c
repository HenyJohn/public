#include "asw_store.h"

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "driver/gpio.h"
#include "string.h"
#include "data_process.h"
#include "meter.h"
/*
inv data key: use uint64_t count.
namespace and key: 15 characters limits.
*/
#define INV_NAMESPACE "inv_namespace"
#define ATE_NAMESPACE "ate_namespace"

#define REGISTER_KEY "register_key"
#define SETTING_KEY "setting_key"
#define STA_PARA_KEY "sta_para_key"
#define WORK_MODE_KEY "work_mode_key"
#define INVTERADD_KEY "invt_info_key"
#define METERSET_KEY "mter_info_key"
#define METERGEN_KEY "mter_gen_key"
#define AP_PARA_KEY "ap_para_key"
#define ATE_REBOOT_KEY "ate_reboot_key"
#define BLUFI_NAME_KEY "blufi_name_key"
#define _3RD_MQTT_KEY "_3rd_key"
#define _3RD_CA_CERT_KEY "_3rd_ca"
#define APN_PARA_KEY "apn_para_key"

SemaphoreHandle_t xSemaphore;

//only initial once unless reset all

static nvs_handle_t inv_nvs_handle;
static nvs_handle_t ate_nvs_handle;

//global ini nvs:
//1. inverter data
//2. general data
asw_db_open(void)
{
    esp_err_t err;
    uint32_t head_index = 0;
    uint32_t tail_index = 0;

    //###########################################################
    xSemaphore = xSemaphoreCreateMutex();
    //###########################################################

    if (xSemaphoreTake(xSemaphore, (TickType_t)6000 / portTICK_RATE_MS) == pdTRUE)
    {
        err = nvs_open_from_partition("my_nvs", INV_NAMESPACE, NVS_READWRITE, &inv_nvs_handle);
        if (err != ESP_OK)
            goto RES_ERR;

        err = nvs_open_from_partition("my_nvs", ATE_NAMESPACE, NVS_READWRITE, &ate_nvs_handle);
        if (err != ESP_OK)
            goto RES_ERR;

        // nvs_erase_all(inv_nvs_handle);
        // nvs_erase_all(ate_nvs_handle);

        if (nvs_get_u32(inv_nvs_handle, "head_index", &head_index) != ESP_OK)
        {
            printf("[ini] head index\n");
            err = nvs_set_u32(inv_nvs_handle, "head_index", 0); //0 is start, write begin with it
            if (err != ESP_OK)
                goto RES_ERR;

            err = nvs_commit(inv_nvs_handle);
            if (err != ESP_OK)
                goto RES_ERR;
        }

        if (nvs_get_u32(inv_nvs_handle, "tail_index", &tail_index) != ESP_OK)
        {
            printf("[ini] tail index\n");
            err = nvs_set_u32(inv_nvs_handle, "tail_index", 0); //0 is start, read begin with it
            if (err != ESP_OK)
                goto RES_ERR;

            err = nvs_commit(inv_nvs_handle);
            if (err != ESP_OK)
                goto RES_ERR;
        }

    RES_OK:
        xSemaphoreGive(xSemaphore);
        return 0;

    RES_ERR:
        printf("ERR: inv_data_ini\n");
        xSemaphoreGive(xSemaphore);
        return -1;
    }
}

// this namespace is for inv data
int inv_data_add(Inv_data_ptr p_data)
{
    if (xSemaphoreTake(xSemaphore, (TickType_t)6000 / portTICK_RATE_MS) == pdTRUE)
    {
        esp_err_t err;
        uint32_t head_index = 0;
        int error_code = 0;

        if (nvs_get_u32(inv_nvs_handle, "head_index", &head_index) != ESP_OK)
        {
            error_code = 1;
            goto RES_ERR;
        }
        char key[16] = {0};
        sprintf(key, "%u", head_index);
        printf("add key: %s\n", key);
        char payload[300] = {0};
        // sprintf(payload, "{\"SSSSSS\":\"AAAAAAA\",\"sn\":\"%s\","
        //                                   "\"smp\":5,\"tim\":\"%s\","
        //                                   "\"fac\":%d,\"pac\":%d,\"er\":%d,"
        //                                   "\"cos\":%d,\"tem\":%d,\"eto\":%d,\"etd\":%d,\"hto\":%d,",
        //                          p_data->psn,
        //                          p_data->time,
        //                          p_data->fac, p_data->pac, p_data->error,
        //                          p_data->cosphi, p_data->col_temp, p_data->e_total, p_data->e_today, p_data->h_total);
        // printf("save db %s \n", payload);

        err = nvs_set_blob(inv_nvs_handle, key, (const char *)p_data, sizeof(Inv_data));
        printf("inv_data_add res=%x \n", err);
        if (err != ESP_OK)
        {
            printf("err:%d\n", err);
            error_code = 2;
            goto RES_ERR;
        }

        head_index++;
        err = nvs_set_u32(inv_nvs_handle, "head_index", head_index);
        if (err != ESP_OK)
        {
            error_code = 3;
            goto RES_ERR;
        }

        err = nvs_commit(inv_nvs_handle);
        if (err != ESP_OK)
        {
            error_code = 4;
            goto RES_ERR;
        }

    RES_OK:
        xSemaphoreGive(xSemaphore);
        return 0;

    RES_ERR:
        printf("ERR: inv_data_add, %d\n", error_code);
        xSemaphoreGive(xSemaphore);
        return -1;
    }
}

int inv_data_query(Inv_data_ptr p_data)
{
    if (xSemaphoreTake(xSemaphore, (TickType_t)6000 / portTICK_RATE_MS) == pdTRUE)
    {
        esp_err_t err;
        uint32_t tail_index = 0;
        int error_code = 0;

        if (nvs_get_u32(inv_nvs_handle, "tail_index", &tail_index) != ESP_OK)
        {
            error_code = 2;
            goto RES_ERR;
        }
        // printf("ttt:tail index: %u----------------------------------------2\n", tail_index);
        char key[16] = {0};
        sprintf(key, "%u", tail_index);
        size_t len = sizeof(Inv_data);
        err = nvs_get_blob(inv_nvs_handle, key, p_data, &len);
        //printf("read key: %s\n", key);
        printf("inv_data_get res=%x \n", err);
        char payload[300] = {0};
        // sprintf(payload, "{\"SSSSSS\":\"OOOOOOO\",\"sn\":\"%s\","
        //                                   "\"smp\":5,\"tim\":\"%s\","
        //                                   "\"fac\":%d,\"pac\":%d,\"er\":%d,"
        //                                   "\"cos\":%d,\"tem\":%d,\"eto\":%d,\"etd\":%d,\"hto\":%d,",
        //                          p_data->psn,
        //                          p_data->time,
        //                          p_data->fac, p_data->pac, p_data->error,
        //                          p_data->cosphi, p_data->col_temp, p_data->e_total, p_data->e_today, p_data->h_total);
        // printf("save db %s \n", payload);

        if (err != ESP_OK)
        {
            error_code = 3;
            goto RES_ERR;
        }
    RES_OK:
        xSemaphoreGive(xSemaphore);
        return 0;

    RES_ERR:
        //printf("NO data inv_data_query, %d %d\n", error_code, err);
        xSemaphoreGive(xSemaphore);
        return -1;
    }
}

int inv_data_delete_one(void)
{
    if (xSemaphoreTake(xSemaphore, (TickType_t)6000 / portTICK_RATE_MS) == pdTRUE)
    {
        esp_err_t err;
        uint32_t tail_index = 0;
        int error_code;

        if (nvs_get_u32(inv_nvs_handle, "tail_index", &tail_index) != ESP_OK)
        {
            error_code = 1;
            goto RES_ERR;
        }
        // printf("ttt:tail index: %u----------------------------------------1312332dc\n", tail_index);
        char key[16] = {0};
        sprintf(key, "%u", tail_index);

        if (nvs_erase_key(inv_nvs_handle, key) != ESP_OK)
        {
            error_code = 2;
            goto RES_ERR;
        }
        tail_index++;
        // printf("ttt:tail index: %u----------------------------------------sfsdf33\n", tail_index);
        err = nvs_set_u32(inv_nvs_handle, "tail_index", tail_index); //tail fresh
        if (err != ESP_OK)
        {
            goto RES_ERR;
            error_code = 3;
        }

        err = nvs_commit(inv_nvs_handle);
        if (err != ESP_OK)
        {
            goto RES_ERR;
            error_code = 4;
        }

    RES_OK:
        xSemaphoreGive(xSemaphore);
        return 0;

    RES_ERR:
        printf("ERR: inv_data_delete_one, %d %d\n", error_code, err);
        xSemaphoreGive(xSemaphore);
        return -1;
    }
}

int inv_data_delete_all(void)
{
    int res = 0;
    while (1)
    {
        res = inv_data_delete_one();
        if (res < 0)
        {
            printf("[ok] all inv data delete\n");
            break;
        }
    }
    return 0;
}

//these <general_xxx> api used to store only 1 item data.
//distinguish by enum type.
//************************************************************* General add
int general_add(int type, void *p_data)
{
    if (xSemaphoreTake(xSemaphore, 6000 / portTICK_RATE_MS) == pdTRUE) //10 / portTICK_RATE_MS,(TickType_t)100
    {
        esp_err_t err;
        int error_code = 0;
        char key[16] = {0};
        size_t blob_len = 0;
        nvs_handle_t my_handle = 0;

        switch (type)
        {
        case NVS_INV:
            memcpy(key, REGISTER_KEY, strlen(REGISTER_KEY));
            blob_len = sizeof(Inv_reg_arr_t);
            my_handle = inv_nvs_handle;
            break;

        case NVS_ATE:
            memcpy(key, SETTING_KEY, strlen(SETTING_KEY));
            blob_len = sizeof(Setting_Para);
            my_handle = ate_nvs_handle;
            break;
        case NVS_INVTER_DB:
            memcpy(key, INVTERADD_KEY, strlen(INVTERADD_KEY));
            blob_len = sizeof(Inv_reg_arr_t);
            my_handle = ate_nvs_handle;
            break;
        case NVS_METER_SET:
            memcpy(key, METERSET_KEY, strlen(METERSET_KEY));
            blob_len = sizeof(meter_setdata);
            my_handle = ate_nvs_handle;
            break;
        case NVS_METER_GEN:
            memcpy(key, METERGEN_KEY, strlen(METERGEN_KEY));
            blob_len = sizeof(meter_gendata);
            my_handle = ate_nvs_handle;
            break;
        case NVS_AP_PARA:
            memcpy(key, AP_PARA_KEY, strlen(AP_PARA_KEY));
            blob_len = sizeof(wifi_ap_para_t);
            my_handle = ate_nvs_handle;
            break;
        case NVS_ATE_REBOOT:
            memcpy(key, ATE_REBOOT_KEY, strlen(ATE_REBOOT_KEY));
            blob_len = sizeof(ate_reboot_t);
            my_handle = ate_nvs_handle;
            break;
        case NVS_3RD_MQTT_PARA:
            memcpy(key, _3RD_MQTT_KEY, strlen(_3RD_MQTT_KEY));
            blob_len = sizeof(_3rd_mqtt_para_t);
            my_handle = ate_nvs_handle;
            break;
        case NVS_3RD_CA_CERT:
            memcpy(key, _3RD_CA_CERT_KEY, strlen(_3RD_CA_CERT_KEY));
            blob_len = sizeof(ssl_file_t);
            my_handle = ate_nvs_handle;
            break;
        case NVS_APN_PARA:
            memcpy(key, APN_PARA_KEY, strlen(APN_PARA_KEY));
            blob_len = sizeof(apn_para_t);
            my_handle = ate_nvs_handle;
            break;
        default:
            goto RES_ERR;
            break;
        }

        err = nvs_set_blob(my_handle, key, p_data, blob_len);
        //printf("%d add blob_len=%d res=%d %d\n", type, blob_len, err, ((meter_setdata *)p_data)->reg);

        if (err != ESP_OK)
        {
            printf("err:%d\n", err);
            error_code = 1;
            goto RES_ERR;
        }

        err = nvs_commit(my_handle);
        if (err != ESP_OK)
        {
            error_code = 2;
            goto RES_ERR;
        }

    RES_OK:
        xSemaphoreGive(xSemaphore);
        return 0;

    RES_ERR:
        printf("ERR: general_add, %d\n", error_code);
        xSemaphoreGive(xSemaphore);
        return -1;
    }
}

//************************************************************* General query
int general_query(int type, void *p_data)
{
    if (xSemaphoreTake(xSemaphore, 6000 / portTICK_RATE_MS) == pdTRUE)
    {
        esp_err_t err;
        int error_code = 0;
        char key[16] = {0};
        size_t blob_len = 0;
        nvs_handle_t my_handle = 0;

        //open
        switch (type)
        {
        case NVS_INV:
            memcpy(key, REGISTER_KEY, strlen(REGISTER_KEY));
            blob_len = sizeof(Inv_reg_arr_t);
            my_handle = inv_nvs_handle;
            break;

        case NVS_ATE:
            memcpy(key, SETTING_KEY, strlen(SETTING_KEY));
            blob_len = sizeof(Setting_Para);
            my_handle = ate_nvs_handle;
            break;
        case NVS_INVTER_DB:
            memcpy(key, INVTERADD_KEY, strlen(INVTERADD_KEY));
            blob_len = sizeof(Inv_reg_arr_t);
            my_handle = ate_nvs_handle;
            break;

        case NVS_METER_SET:
            memcpy(key, METERSET_KEY, strlen(METERSET_KEY));
            blob_len = sizeof(meter_setdata);
            my_handle = ate_nvs_handle;
            break;

        case NVS_METER_GEN:
            memcpy(key, METERGEN_KEY, strlen(METERGEN_KEY));
            blob_len = sizeof(meter_gendata);
            my_handle = ate_nvs_handle;
            break;

        case NVS_AP_PARA:
            memcpy(key, AP_PARA_KEY, strlen(AP_PARA_KEY));
            blob_len = sizeof(wifi_ap_para_t);
            my_handle = ate_nvs_handle;
            break;
        case NVS_ATE_REBOOT:
            memcpy(key, ATE_REBOOT_KEY, strlen(ATE_REBOOT_KEY));
            blob_len = sizeof(ate_reboot_t);
            my_handle = ate_nvs_handle;
            break;
        case NVS_3RD_MQTT_PARA:
            memcpy(key, _3RD_MQTT_KEY, strlen(_3RD_MQTT_KEY));
            blob_len = sizeof(_3rd_mqtt_para_t);
            my_handle = ate_nvs_handle;
            break;
        case NVS_3RD_CA_CERT:
            memcpy(key, _3RD_CA_CERT_KEY, strlen(_3RD_CA_CERT_KEY));
            blob_len = sizeof(ssl_file_t);
            my_handle = ate_nvs_handle;
            break;
        case NVS_APN_PARA:
            memcpy(key, APN_PARA_KEY, strlen(APN_PARA_KEY));
            blob_len = sizeof(apn_para_t);
            my_handle = ate_nvs_handle;
            break;

        default:
            goto RES_ERR;
            break;
        }

        err = nvs_get_blob(my_handle, key, p_data, &blob_len);
        //printf("%d query blob_len=%d res=%d %s\n", type, blob_len, err, p_data);

        if (err != ESP_OK)
        {
            //printf("blob_len%d err:%d\n", blob_len, err);
            error_code = 1;
            goto RES_ERR;
        }

        err = nvs_commit(my_handle);
        if (err != ESP_OK)
        {
            printf("%d err:%d\n", type, err);
            error_code = 2;
            goto RES_ERR;
        }

    RES_OK:
        xSemaphoreGive(xSemaphore);
        return 0;

    RES_ERR:
        //printf("%d no data found general_query, %d\n", type, error_code);
        xSemaphoreGive(xSemaphore);
        return -1;
    }
}

//************************************************************* General delete
int general_delete(int type)
{
    if (xSemaphoreTake(xSemaphore, (TickType_t)6000 / portTICK_RATE_MS) == pdTRUE)
    {
        esp_err_t err;
        int error_code = 0;
        char key[16] = {0};
        nvs_handle_t my_handle = 0;

        //open
        switch (type)
        {
        case NVS_INV:
            memcpy(key, REGISTER_KEY, strlen(REGISTER_KEY));
            my_handle = inv_nvs_handle;
            break;

        case NVS_ATE:
            memcpy(key, SETTING_KEY, strlen(SETTING_KEY));
            my_handle = ate_nvs_handle;
            break;
        case NVS_ATE_REBOOT:
            memcpy(key, ATE_REBOOT_KEY, strlen(ATE_REBOOT_KEY));
            my_handle = ate_nvs_handle;
            break;
        case NVS_INVTER_DB:
            memcpy(key, INVTERADD_KEY, strlen(INVTERADD_KEY));
            my_handle = ate_nvs_handle;
            break;
        case NVS_METER_SET:
            memcpy(key, METERSET_KEY, strlen(METERSET_KEY));
            my_handle = ate_nvs_handle;
            break;
        case NVS_METER_GEN:
            memcpy(key, METERGEN_KEY, strlen(METERGEN_KEY));
            my_handle = ate_nvs_handle;
            break;
        case NVS_3RD_MQTT_PARA:
            memcpy(key, _3RD_MQTT_KEY, strlen(_3RD_MQTT_KEY));
            my_handle = ate_nvs_handle;
            break;
        case NVS_3RD_CA_CERT:
            memcpy(key, _3RD_CA_CERT_KEY, strlen(_3RD_CA_CERT_KEY));
            my_handle = ate_nvs_handle;
            break;
        case NVS_APN_PARA:
            memcpy(key, APN_PARA_KEY, strlen(APN_PARA_KEY));
            my_handle = ate_nvs_handle;
            break;
        default:
            goto RES_ERR;
            break;
        }

        err = nvs_erase_key(my_handle, key);

        if (err != ESP_OK)
        {
            printf("err:%d\n", err);
            error_code = 1;
            goto RES_ERR;
        }

        err = nvs_commit(my_handle);
        if (err != ESP_OK)
        {
            error_code = 2;
            goto RES_ERR;
        }

    RES_OK:
        xSemaphoreGive(xSemaphore);
        return 0;

    RES_ERR:
        printf("ERR: general_delete, %d\n", error_code);
        xSemaphoreGive(xSemaphore);
        return -1;
    }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//HOW TO USE:

void nvs_asw_example(void)
{
    Inv_data inv_data = {0};
    int i;
    int res = 0;

    printf("1st read:------------------------\n");
    //query and delete all
    while (1)
    {
        memset(&inv_data, 0, sizeof(inv_data));
        res = inv_data_query(&inv_data);
        printf("[query] res: %d, ", res);
        if (res < 0)
            break;
        printf("nvs etotal: %d, ", inv_data.e_total);
        res = inv_data_delete_one();
        // printf("ttt----------------------------------------1\n");
        printf("[del] res: %d\n", res);
    }
    printf("\r\n");
    //write some data
    printf("1st write:------------------------\n");
    for (i = 0; i < 10; i++)
    {
        inv_data.e_total = i;
        res = inv_data_add(&inv_data);
        printf("[add] res: %d >>>>>>>\n", res);
    }

    //***************************************************general
    Inv_reg_arr_t my_reg = {0};
    Setting_Para my_setting = {0};
    int res1, res2;
    //query
    res1 = general_query(NVS_INV, &my_reg);
    res2 = general_query(NVS_ATE, &my_setting);

    printf("[query] modbus id: %d, res1: %d >>>>>>>\n", my_reg[0].modbus_id, res1);
    //printf("[query] port: %d , res2: %d >>>>>>>\n", my_setting.port, res2);

    //write
    my_reg[0].modbus_id = 111;
    //my_setting.port = 222;

    res1 = general_add(NVS_INV, &my_reg);
    res2 = general_add(NVS_ATE, &my_setting);
    printf("[add] res1: %d >>>>>>>\n", res1);
    printf("[add] res2: %d >>>>>>>\n", res2);

    // res1 = general_delete(NVS_INV);
    // res2 = general_delete(NVS_ATE);
    // printf("[del] res1: %d >>>>>>>\n", res1);
    // printf("[del] res2: %d >>>>>>>\n", res2);
}

void nvs_life_test(void)
{
    int value = 999;
    Setting_Para my_setting = {0};
    int res, i = 0;

    res = general_query(NVS_ATE, &my_setting);
    //printf("first read: %d\n", my_setting.port);

    //my_setting.port = value;
    while (1)
    {
        // my_setting.port = i;
        res = general_add(NVS_ATE, &my_setting);
        if (res < 0)
            break;

        //my_setting.port = 0;
        res = general_query(NVS_ATE, &my_setting);
        //if (res < 0 || my_setting.port != i)
        break;
        //if (i % 1000 == 0)
        ////printf("var: %d\n", my_setting.port);
        i++;
    }
}

void nvs_capacity_test(void)
{
    Inv_data inv_data = {0};
    int i;
    int res = 0;

    //query and delete all
    while (1)
    {
        memset(&inv_data, 0, sizeof(inv_data));
        res = inv_data_query(&inv_data);
        printf("[query] res: %d, ", res);
        if (res < 0)
            break;
        printf("nvs etotal: %d, ", inv_data.e_total);
        res = inv_data_delete_one();

        printf("[del] res: %d\n", res);
    }
    printf("\r\n");
    //write some data
    i = 0;
    while (1)
    {
        inv_data.e_total = i;
        res = inv_data_add(&inv_data);
        if (res < 0)
        {
            printf("no space, add res: %d\n", res);
            break;
        }
        printf("[add]: %d\n", i);
        i++;
    }
}

void factory_reset_nvs()
{
    general_delete(NVS_INVTER_DB);
    general_delete(NVS_METER_SET);
    general_delete(NVS_METER_GEN);
    general_delete(NVS_APN_PARA);
}

int get_device_sn(void)
{
    Setting_Para para = {0};
    general_query(NVS_ATE, &para);
    printf("psn: %s \n", para.psn);
    if (strlen(para.psn) > 0 && strlen(para.mod) > 0)
        return 0;
    return -1;
}

void esp32rf_nvs_clear(void)
{
    nvs_flash_deinit_partition("nvs");
    ESP_ERROR_CHECK(nvs_flash_erase_partition("nvs"));
    nvs_flash_init_partition("nvs");
    printf("delet esp32 rf data \n");
}
