/**
 * @file inv_com.h
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2022-03-29
 *
 * @copyright Copyright (c) 2022
 *
 */
#ifndef INV_COM_H
#define INV_COM_H

#include "Asw_global.h"
#include "driver/uart.h"

#include "driver/gpio.h"
#include "data_process.h"
#include "esp32_time.h"
#include "asw_nvs.h"
#include "asw_modbus.h"

#include "asw_fat.h"
// #include "log.h"
#include "utility.h"
#include "crc16.h"
#include "update.h"

#define BATTERY_CHARGING_STOP 1 // stop charging/discharging
#define BATTERY_CHARGING 2      // charging
#define BATTERY_DISCHARGING 3   // discharging

//-----------------------------------//

typedef enum
{
    MAIN_UART = 0,
    INV_UART = 1,
    ATE_UART = 2
} UART_TYPE;

//-----------------------------------//

// void drm_handler(int drm); [  update] delete
// void com_idle_work(void);

int check_inv_states();

//-----------------------------------//

// extern Inv_arr_t ipc_inv_arr;
// extern uint8_t inv_real_num;

extern Inv_arr_t cgi_inv_arr;
extern Inv_arr_t cld_inv_arr;

extern Inv_arr_t inv_arr;
extern int inv_comm_error;

extern int inv_com_reboot_flg;

// extern int8_t g_monitor_state;  //[  mark] change monitor_state->g_monitor_state;

// int16_t get_cur_week_key();
// void set_cur_week_key(int16_t day);

// int get_estore_inv_arr_first(void);
int8_t is_has_inv_online(void);
// int is_com_has_estore(void); [  update] delete
int last_com_data(unsigned int index);

int8_t serialport_init(UART_TYPE type);
int chk_msg(cloud_inv_msg *trans_data);
// int is_inv_has_estore(void);

// void drm_handler(int drm);
uint8_t load_reg_info(void);
void flush_serial_port(uint8_t);
int internal_inv_log(int typ);

int inv_com_log(int index, int err);
int8_t legal_check(char *text);
// int8_t is_has_inv_online(void);

int8_t get_sum(uint8_t *buf, int len, uint8_t *high_byte, uint8_t *low_byte);
int8_t check_sum(uint8_t *buf, int len);
int8_t parse_sn(uint8_t *buf, int len, char *psn);
int8_t parse_modbus_addr(uint8_t *buf, int len, uint8_t *addr);
int8_t save_reg_info(void);
void reg_info_log(void);
int8_t clear_inv_list(void);
int8_t get_modbus_id(uint8_t *id);
// void check_estore_become_online(void);

int8_t recv_bytes_frame_transform(int fd, uint8_t *res_buf, uint16_t *res_len);

SERIAL_STATE inv_task_schedule(char *func, cloud_inv_msg *data);
SERIAL_STATE query_data_proc();
SERIAL_STATE upinv_transdata_proc(char func, cloud_inv_msg tans_data);
SERIAL_STATE scan_register(void);

int8_t recv_bytes_frame_waitting_nomd(int fd, uint8_t *res_buf, uint16_t *res_len);
int8_t recv_bytes_frame_waitting_nomd_tcp(int fd, uint8_t *res_buf, uint16_t *res_len); /* kaco-lanstick 20220801 + */

//-----------------外部component调用------------------//
// int is_cgi_has_estore(void);
// uint8_t get_cgi_inv_arr_first(void);
int get_inv_log(char *buf, int typ);
int recv_bytes_frame_meter(int fd, uint8_t *res_buf, uint16_t *res_len); //[  update] 被meter调用
//---------------------------------------//
 //kaco-lanstick 20220811 +
SERIAL_STATE md_kaco_set_addr(void); 
void kaco_change_modbus_addr_then(void);


#endif
