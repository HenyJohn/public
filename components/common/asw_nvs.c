#include "asw_nvs.h"
#include "freertos/semphr.h"

#define INV_NAMESPACE "inv_namespace"
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
#define CONFIG_KEY "config_key"
#define SCHED_BAT_KEY "sched_bat_key"

/* kaco-lanstick v1.0.0 220801 + */
#define INVSET_NAME_KEY "inv_set_key"
#define TMZ_NAME_SETFLG "time_zones"
#define KACO_MODE_KEY "kaco_mode_key"
#define KACO_ADDR_KEY "kaco_addr_key"


//----------------------

#define ATE_NAMESPACE "ate_namespace" //[ update] 新增版本

//---------------

EXT_RAM_ATTR static nvs_handle_t inv_nvs_handle;
EXT_RAM_ATTR static nvs_handle_t ate_nvs_handle;

static const char *TAG = "asw_fat.h";
/* 只在本文件调用 */
// static nvs_handle_t my_nvs_handle; //只能被本文件可见 
SemaphoreHandle_t xSemaphore;

// EXT_RAM_ATTR MonitorPara g_monitor_para = {0};
// this namespace is for inv data
//********************************//
int restore_ap(void)
{
    Setting_Para para = {0};
    general_query(NVS_ATE, &para);
    ESP_LOGI(TAG, "reset get psn %s \n", para.psn);

    if (strlen(para.psn) > 10)
    {
#if 0 // kaco-lanstick 20220803 -
        char tmpssid[16] = {0};
        char lastsn[5] = {0};
        strncpy(lastsn, para.psn + (strlen(para.psn) - 4), 4);
        ESP_LOGI(TAG, "last sn %s \n", lastsn);

        memcpy(tmpssid, "AISWEI-", 7);
        memcpy(&tmpssid[7], lastsn, 4);
#endif
        wifi_ap_para_t _ap_para = {0};
        memcpy(_ap_para.ssid, para.psn, strlen(para.psn));

        ESP_LOGI(TAG, "restore ap ssid: %s\n", _ap_para.ssid);

        wifi_ap_para_t ap_para_tmp = {0};
        general_query(NVS_AP_PARA, &ap_para_tmp);
        memcpy(_ap_para.password, ap_para_tmp.password, sizeof(ap_para_tmp.password));

        general_add(NVS_AP_PARA, &_ap_para);
    }
    return 0;
}

//------------------------------//
/**   reset wifi environment  **/
//-----------------------------//
void factory_reset_nvs()
{
    // v2.0.0 add
    int work_mode = Work_Mode_AP_PROV;
    general_add(NVS_WORK_MODE, &work_mode);
    set_kaco_run_mode(1); // kaco-lanstick 20220803 +

    restore_ap();
    general_delete(NVS_INVTER_DB);
    general_delete(NVS_STA_PARA);
    general_delete(NVS_METER_SET); //[  update]add
    general_delete(NVS_METER_GEN);
}

//------------------------------------//
// void asw_nvs_init(void)
// {
//     esp_err_t ret = nvs_flash_init_partition("my_nvs");
//     if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
//     {

//         ESP_ERROR_CHECK(nvs_flash_erase_partition("my_nvs"));
//         ret = nvs_flash_init_partition("my_nvs");
//     }

//     ESP_ERROR_CHECK(ret);
// }

// global ini nvs:
// 1. inverter data
// 2. general data
int8_t asw_db_open(void) //[  update] asw_nvs_open
{

    esp_err_t err;
    uint32_t head_index = 0;
    uint32_t tail_index = 0;

    //###########################################################
    xSemaphore = xSemaphoreCreateMutex();
    //###########################################################

    if (xSemaphoreTake(xSemaphore, (TickType_t)0) == pdTRUE)
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
            ESP_LOGI(TAG, "[ini] head index\n");
            err = nvs_set_u32(inv_nvs_handle, "head_index", 0); // 0 is start, write begin with it
            if (err != ESP_OK)
                goto RES_ERR;

            err = nvs_commit(inv_nvs_handle);
            if (err != ESP_OK)
                goto RES_ERR;
        }

        if (nvs_get_u32(inv_nvs_handle, "tail_index", &tail_index) != ESP_OK)
        {
            ESP_LOGI(TAG, "[ini] tail index\n");
            err = nvs_set_u32(inv_nvs_handle, "tail_index", 0); // 0 is start, read begin with it
            if (err != ESP_OK)
                goto RES_ERR;

            err = nvs_commit(inv_nvs_handle);
            if (err != ESP_OK)
                goto RES_ERR;
        }

        xSemaphoreGive(xSemaphore);
        return ASW_OK;

    RES_ERR:
        ESP_LOGE(TAG, "ERR: inv_data_ini\n");
        xSemaphoreGive(xSemaphore);
        return ASW_FAIL;
    }
    return ASW_FAIL;
}

int8_t general_query(Enum_NVS_NameSpace type, void *p_data)
{
    esp_err_t err;
    // int error_code = 0;
    char key[16] = {0};
    size_t blob_len = 0;
    nvs_handle_t my_handle = 0;

    /** 最长阻塞1秒，增强可靠性*/
    if (xSemaphoreTake(xSemaphore, 1000 / portTICK_RATE_MS) == pdTRUE)
    {
        // open
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
        case NVS_STA_PARA:
            memcpy(key, STA_PARA_KEY, strlen(STA_PARA_KEY));
            blob_len = sizeof(wifi_sta_para_t);
            my_handle = ate_nvs_handle;
            break;
        case NVS_WORK_MODE:
            memcpy(key, WORK_MODE_KEY, strlen(WORK_MODE_KEY));
            blob_len = sizeof(int);
            my_handle = ate_nvs_handle;
            break;

        case NVS_INVTER_DB:
            memcpy(key, INVTERADD_KEY, strlen(INVTERADD_KEY));
            blob_len = sizeof(Inv_reg_arr_t);
            my_handle = ate_nvs_handle;
            break;
        case NVS_METER_GEN:
            memcpy(key, METERGEN_KEY, strlen(METERGEN_KEY));
            blob_len = sizeof(meter_gendata);
            my_handle = ate_nvs_handle;
            break;

        case NVS_METER_SET: //[update]
            memcpy(key, METERSET_KEY, strlen(METERSET_KEY));
            blob_len = sizeof(meter_setdata);
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
        case NVS_BLUFI_NAME:
            memcpy(key, BLUFI_NAME_KEY, strlen(BLUFI_NAME_KEY));
            blob_len = sizeof(blufi_name_t);
            my_handle = ate_nvs_handle;
            break;

            /* kaco-lanstick v1.0.0 20220801+ */
        case NVS_INV_SETFLG:
            memcpy(key, INVSET_NAME_KEY, strlen(INVSET_NAME_KEY));
            blob_len = sizeof(int);
            my_handle = ate_nvs_handle;
            break;
        case TM_ZONE_SETFLG:
            memcpy(key, TMZ_NAME_SETFLG, strlen(TMZ_NAME_SETFLG));
            blob_len = sizeof(int);
            my_handle = ate_nvs_handle;
            break;
        case NVS_KACO_MODE:
            memcpy(key, KACO_MODE_KEY, strlen(KACO_MODE_KEY));
            blob_len = sizeof(int);
            my_handle = ate_nvs_handle;
            break;

        default:
            goto RES_ERR;
            break;
        }

        err = nvs_get_blob(my_handle, key, p_data, &blob_len);

        if (err != ESP_OK)
        {
            // printf("blob_len%d err:%d\n", blob_len, err);
            // error_code = 1;
            goto RES_ERR;
        }

        err = nvs_commit(my_handle);
        if (err != ESP_OK)
        {
            // printf("%d err:%d\n", type, err);
            ESP_LOGE(TAG, "%d err:%d\n", type, err);
            // error_code = 2;
            goto RES_ERR;
        }

        // RES_OK:
        xSemaphoreGive(xSemaphore);
        return ASW_OK;

    RES_ERR:
        // printf("%d no data found general_query, %d\n", type, error_code);
        xSemaphoreGive(xSemaphore);
        return ASW_FAIL;
    }
    return ASW_FAIL;
}
//-----------------------------//

//--------------------------------//
int8_t general_delete(int type)
{
    /** 最长阻塞1秒，增强可靠性*/
    if (xSemaphoreTake(xSemaphore, /*6000 / portTICK_RATE_MS*/ ((TickType_t)0)) == pdTRUE)
    {
        esp_err_t err;
        int error_code = 0;
        char key[16] = {0};
        nvs_handle_t my_handle = 0;

        // open
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
        case NVS_STA_PARA:
            memcpy(key, STA_PARA_KEY, strlen(STA_PARA_KEY));
            my_handle = ate_nvs_handle;
            break;
        // case NVS_AP_PARA:
        //     memcpy(key, AP_PARA_KEY, strlen(AP_PARA_KEY));
        //     my_handle = ate_nvs_handle;
        //     break;
        case NVS_WORK_MODE:
            memcpy(key, WORK_MODE_KEY, strlen(WORK_MODE_KEY));
            my_handle = ate_nvs_handle;
            break;
        case NVS_ATE_REBOOT:
            memcpy(key, ATE_REBOOT_KEY, strlen(ATE_REBOOT_KEY));
            my_handle = ate_nvs_handle;
            break;
        case NVS_BLUFI_NAME:
            memcpy(key, BLUFI_NAME_KEY, strlen(BLUFI_NAME_KEY));
            my_handle = ate_nvs_handle;
            break;
        case NVS_INVTER_DB:
            memcpy(key, INVTERADD_KEY, strlen(INVTERADD_KEY));
            my_handle = ate_nvs_handle;
            break;
        case NVS_METER_GEN:
            memcpy(key, METERGEN_KEY, strlen(METERGEN_KEY));
            my_handle = ate_nvs_handle;
            break;
        case NVS_METER_SET:
            memcpy(key, METERSET_KEY, strlen(METERSET_KEY));
            my_handle = ate_nvs_handle;
            break;

        /* kaco-lanstick v1.0.0 20220801 + */
        case NVS_INV_SETFLG:
            memcpy(key, INVSET_NAME_KEY, strlen(INVSET_NAME_KEY));
            my_handle = ate_nvs_handle;
            break;
        case TM_ZONE_SETFLG:
            memcpy(key, TMZ_NAME_SETFLG, strlen(TMZ_NAME_SETFLG));
            my_handle = ate_nvs_handle;
            break;
        case NVS_KACO_MODE:
            memcpy(key, KACO_MODE_KEY, strlen(KACO_MODE_KEY));
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

        xSemaphoreGive(xSemaphore);
        return 0;

    RES_ERR:
        ESP_LOGE(TAG, "ERR: general_delete, %d\n", error_code);
        xSemaphoreGive(xSemaphore);
        return ASW_FAIL;
    }
    return ASW_OK;
}

//---------------------------//

int8_t get_device_sn(void)
{
    Setting_Para para = {0};
    general_query(NVS_ATE, &para);
    // printf("first get psn %s \n", para.psn);

    if (strlen(para.psn) > 0)
    {
        ESP_LOGI(TAG, "first get psn %s \n", para.psn);
        return ASW_OK;
    }

    return ASW_FAIL;
}

//------------------------//
// kaco-lanstick v1.0.0
int get_kaco_run_mode(void)
{
    int mode = 1;
    general_query(NVS_KACO_MODE, &mode);

    printf("\n=====debug kaco run mode: %d\n", mode);
    if (mode != 1 && mode != 2)
        mode = 1;
    return mode;
}

// kaco-lanstick v1.0.0
void set_kaco_run_mode(int mode)
{
    printf("\n=====  debug kaco write mode AAAAAAAAAAAAAAAAAAAA  =========\n");
    if (mode == 1 || mode == 2)
    {
        general_add(NVS_KACO_MODE, &mode);

        printf("\n=====debug kaco write mode =========\n");

        printf("\n=====debug kaco write mode: %d\n", mode);
        printf("\n=====debug kaco write mode =========\n");
    }
}

//----------------------------//

int8_t general_add(Enum_NVS_NameSpace type, void *p_data)
{
    /** 最长阻塞1秒，增强可靠性*/
    if (xSemaphoreTake(xSemaphore, 1000 / portTICK_RATE_MS) == pdTRUE) // 10 / portTICK_RATE_MS,(TickType_t)100
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
            my_handle = inv_nvs_handle; // my_nvs_handle;
            break;

        case NVS_ATE:
            memcpy(key, SETTING_KEY, strlen(SETTING_KEY));
            blob_len = sizeof(Setting_Para);
            my_handle = ate_nvs_handle; // my_nvs_handle;
            break;
        case NVS_STA_PARA:
            memcpy(key, STA_PARA_KEY, strlen(STA_PARA_KEY));
            blob_len = sizeof(wifi_sta_para_t);
            my_handle = ate_nvs_handle; // my_nvs_handle;
            break;
        case NVS_WORK_MODE:
            memcpy(key, WORK_MODE_KEY, strlen(WORK_MODE_KEY));
            blob_len = sizeof(int);
            my_handle = ate_nvs_handle; // my_nvs_handle;
            break;
        case NVS_INVTER_DB:
            memcpy(key, INVTERADD_KEY, strlen(INVTERADD_KEY));
            blob_len = sizeof(Inv_reg_arr_t); // 跟NVS_INV数据内容时候一样的？？
            my_handle = ate_nvs_handle;       // my_nvs_handle;
            break;
        case NVS_METER_SET:
            memcpy(key, METERSET_KEY, strlen(METERSET_KEY));
            blob_len = sizeof(meter_setdata); // 跟NVS_INV数据内容时候一样的？？
            my_handle = ate_nvs_handle;       // my_nvs_handle;
            break;
        case NVS_METER_GEN:
            memcpy(key, METERGEN_KEY, strlen(METERGEN_KEY));
            blob_len = sizeof(meter_gendata);
            my_handle = ate_nvs_handle; // my_nvs_handle;
            break;
        case NVS_AP_PARA:
            memcpy(key, AP_PARA_KEY, strlen(AP_PARA_KEY));
            blob_len = sizeof(wifi_ap_para_t);
            my_handle = ate_nvs_handle; // my_nvs_handle;
            break;
        case NVS_ATE_REBOOT:
            memcpy(key, ATE_REBOOT_KEY, strlen(ATE_REBOOT_KEY));
            blob_len = sizeof(ate_reboot_t);
            my_handle = ate_nvs_handle; // my_nvs_handle;
            break;
        case NVS_BLUFI_NAME:
            memcpy(key, BLUFI_NAME_KEY, strlen(BLUFI_NAME_KEY));
            blob_len = sizeof(blufi_name_t);
            my_handle = ate_nvs_handle; // my_nvs_handle;
            break;

        /* kaco-lanstick v1.0.0 220801 + */
        case TM_ZONE_SETFLG:
            memcpy(key, TMZ_NAME_SETFLG, strlen(TMZ_NAME_SETFLG));
            blob_len = sizeof(int);
            my_handle = ate_nvs_handle;
            break;
        case NVS_INV_SETFLG:
            memcpy(key, INVSET_NAME_KEY, strlen(INVSET_NAME_KEY));
            blob_len = sizeof(int);
            my_handle = ate_nvs_handle;
            break;
        case NVS_KACO_MODE:
            memcpy(key, KACO_MODE_KEY, strlen(KACO_MODE_KEY));
            blob_len = sizeof(int);
            my_handle = ate_nvs_handle;
            break;

        default:
            goto RES_ERR;
            break;
        }

        err = nvs_set_blob(my_handle, key, p_data, blob_len);
        // esp_err_tnvs_set_blob(nvs_handle_thandle, const char *key, const void *value, size_t length)   mark

        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "err:%d\n", err);
            // printf("err:%d\n", err);
            error_code = 1;
            goto RES_ERR;
        }

        err = nvs_commit(my_handle);
        if (err != ESP_OK)
        {
            error_code = 2;
            goto RES_ERR;
        }

        xSemaphoreGive(xSemaphore);
        ESP_LOGI(TAG, "OK, nvs write: %s\n", key);
        // printf("OK, nvs write: %s\n", key);
        return ASW_OK;

    RES_ERR:
        ESP_LOGE(TAG, "ERR: general_add, %d\n", error_code);
        // printf("ERR: general_add, %d\n", error_code);
        xSemaphoreGive(xSemaphore);
        return ASW_FAIL;
    }
    return ASW_FAIL;
}