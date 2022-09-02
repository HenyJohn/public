/**
 * @file save_inv_data.h
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2022-04-12
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef _SAVE_DATA_H
#define _SAVE_DATA_H

#include "Asw_global.h"
#include <dirent.h>
#include "data_process.h"
#include "asw_fat.h"

#include "esp32_time.h"
#include "app_mqtt.h"


extern int net_com_reboot_flg;  //[mark]

int write_change_apconfig(void);

 /* kaco-lanstick 20220802 + -  */
// int recive_invdata(int *flag, Inv_data *inv_data, Inv_data *meter_data, int *lost_flag);
int recive_invdata(int *flag, kaco_inv_data_t *inv_data, Inv_data *meter_data, int *lost_flag);


void check_resetnet_reboot(void);

//kaco-lanstick 20220802 +
int find_local_data(Inv_data *invdata, int *line, int *fileof, int *meter_flg); 
int delet_local_data(int line, int file_tail);
uint32_t read_timezone(uint32_t utc_sec);

#endif
