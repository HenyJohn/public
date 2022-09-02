/**
 * @file asw_nvs.h
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2022-04-06
 *
 * @copyright Copyright (c) 2022
 *
 */

#ifndef _ASW_NVS_H
#define _ASW_NVS_H

#include "Asw_global.h"
#include <nvs_flash.h>
#include "data_process.h"

#include "asw_nvs.h"
// #include "meter_md_process.h"
#include "meter.h"

typedef enum
{
    NVS_INV = 1,
    NVS_ATE,
    NVS_STA_PARA,
    NVS_WORK_MODE,
    NVS_INVTER_DB,
    NVS_METER_SET, //
    NVS_METER_GEN,
    NVS_AP_PARA,
    NVS_ATE_REBOOT,
    NVS_BLUFI_NAME,
    /* kaco-lanstick v1.0.0 0801 + */
    NVS_INV_SETFLG,
    TM_ZONE_SETFLG,
    NVS_KACO_MODE
    // NVS_CONFIG,
    // NVS_SCHED_BAT
} Enum_NVS_NameSpace;

// void asw_nvs_init(void);
int8_t asw_db_open(void); // int8_t asw_nvs_open(void); [    update]
int8_t get_device_sn(void);

int8_t general_query(Enum_NVS_NameSpace type, void *p_data);
int8_t general_add(Enum_NVS_NameSpace type, void *p_data);

int8_t general_delete(int type);
// void factory_reset_config(void);  [    update] delete

// void esp32rf_nvs_clear(void); [    update] delete
void factory_reset_nvs();

// kaco-lanstick v1.0.0
int get_kaco_run_mode(void);
void set_kaco_run_mode(int mode);

#endif