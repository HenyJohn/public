#ifndef TIANHE_H
#define TIANHE_H
#include "data_process.h"
#include "asw_store.h"

/** 天合协议版本号*/
#define TIANHE_VER 0x0113

/** send cmd type*/
#define CMD_LOGIN 0x0001
#define CMD_KEEP_ALIVE 0x0002
#define CMD_GET_TIME 0x0003
#define CMD_SEND_FDBG 0x000B
#define CMD_SEND_INV_DATA 0x0103
#define CMD_SEND_HIST_INV_DATA 0x0104
#define CMD_SEND_COMPONENT_DATA 0x0201
#define CMD_SEND_HIST_COMPONENT_DATA 0x0206

/** recv cmd type*/
#define CMD_SET_SERVER 0x0004
#define CMD_UPGRADE 0x0005
#define CMD_OFF 0x0006
#define CMD_ON 0x0007
#define CMD_PMU_RESTART 0x0008
#define CMD_DEV_INFO 0x0009
#define CMD_SET_SAFE_VOLT 0x000A
#define CMD_RECV_FDBG 0x000C
#define CMD_SET_PWR_RATE 0x000D
#define CMD_GET_SAFE_VOLT 0x000E
#define CMD_GET_INV_SW_VER 0x000F
#define CMD_GET_SIM_INFO 0x0010
#define CMD_RSD_ENABLE 0x0202
#define CMD_COMPONENT_RESTART 0x0203
#define CMD_COMPONENT_UPGRAGE 0x0204
#define CMD_GET_ECU_INV_CMPNT_MAP 0x0205

int get_login_frame(uint8_t *msg, login_para_t para);
int get_keepalive_frame(uint8_t *msg);

int tianhe_login(login_para_t para);
int tianhe_keepalive(void);
int tianhe_get_server_time(void);
int tianhe_send_invdata(Inv_data invdata);
int tianhe_send_hist_invdata(Inv_data invdata);

#endif
