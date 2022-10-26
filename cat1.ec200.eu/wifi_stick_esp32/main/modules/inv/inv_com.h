
#ifndef INV_COM_H
#define INV_COM_H

#include "data_process.h"
// #include "modbus.h"
#include <pthread.h>
#define QUERY_TIMING 10

extern uint8_t single_multi_mode;
#define MODE_SINGE 0x01
#define MODE_MULTI 0x02

extern int8_t clear_register_tab_evt_flag;
// extern modbus_t *global_ctx;
extern pthread_mutex_t inv_arr_mutex;

extern Inv_arr_t inv_arr;
extern Inv_arr_t ipc_inv_arr;
extern int now_read_inv_index;

// int load_reg_info(void);

#define NORMAL_MODE 0x0001 //正常模式
#define MODBUS_MODE 0x0002 //modbus透传

#define PASS_THROUGH_START_MODE 0x0004  //普通透传
#define PASS_THROUGH_FINISH_MODE 0x0008 //普通透传

#define INV_UPDATE_START_MODE 0x0010  //
#define INV_UPDATE_FINISH_MODE 0x0020 //

/*
enum ATE_or_Normal_Mode
{
    ATE_MODE,
    NORMAL_MODE
};
*/
typedef enum SERIAL_STATE
{
    TASK_IDLE,
    TASK_QUERY_DATA,
    TASK_TRANS_UP_INV,
    TASK_TRANS_SCAN_INV,
    TASK_QUERY_METER,
    CLEAR_METER_SET,
    RESTART
} SERIAL_STATE;

#define TASK_QUERY_INV 1
#define TASK_UPGRADE_INV 3
#define TASK_PAUSE 4
#define TASK_SET_IP 5
#define TASK_SET_CLD_STATE 6
#define TASK_PWR_CTRL 7
#define TASK_SAFETY_TYPE 8
#define TASK_GET_SAFETY 9
#define TASK_SET_SAFETY 10
#define TASK_SET_ADV 11
#define TASK_DSP_HD_DG 12
#define TASK_DSP_FD_DG 13
#define TASK_METER_TEST 14
#define TASK_TRANSPARENT_COMM 15
#define TASK_START_REGISTRY 16

#endif
