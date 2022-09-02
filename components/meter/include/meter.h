/**
 * @file meter.h
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2022-03-30
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#ifndef _METER_H
#define _METER_H

// #include "Asw_global.h"
// #include "meter_md_process.h"

#include "Asw_global.h"
#include "data_process.h"

#include "asw_modbus.h"
#include "inv_com.h"
#include "utility.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "wifi_sta_server.h"
//------------------------------//
/** Aimeter电表类型,从7开始。7以下的是之前的老电表*/
typedef enum
{
    METER_IREADER_IM1281B = 7,
    METER_ACREL_ADL200 = 100
} aimeter_types;
//-----------------------//
typedef struct
{
    int e_total;
    int h_total;
    char day;
} meter_gendata;
//-----------------------//
typedef struct
{
    /* data */
    int enb;
    int mod;
    int reg;
    int target;
    int date;
    int enb_pf;
    int target_pf;
} meter_setdata;

//----------------------//
typedef struct 
{
    uint16_t reg_addr;
    uint16_t reg_num;
    uint8_t len;
    uint8_t data[256];
} TMD_RTU_WR_MTP_REQ_Packet;

//--------------------//
typedef struct _meter_data
{
    unsigned int type;
    Inv_data data;
} meter_cloud_data;
//-------------------------------//

extern int total_rate_power;
extern int total_curt_power;
// extern meter_data_t g_meter_inv_meter;
extern int16_t meter_real_factory;
extern Inverter g_inv_meter ; //  change inv_meter->g_inv_meter
extern meter_setdata g_merter_config ; //  change mtconfig->g_merter_config

SERIAL_STATE clear_meter_setting(char msg);
SERIAL_STATE query_meter_proc(void);
void get_meter_info(char mod, char *meterbran, char *metername);
int write_meter_file(meter_setdata *mtconfigx);


//kaco-lanstick 20220802 +
int get_meter_mem_data(Inv_data *data);
int get_meter_index(void);

#endif