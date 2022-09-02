/**
 * @file kaco_tcp_server.h
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2022-08-01
 *
 * @copyright Copyright (c) 2022
 *
 */
#ifndef _KACO_TCP_SERVER_H
#define _KACO_TCP_SERVER_H
#include "Asw_global.h"

extern uint8_t is_modbus_tcp_in_use;
extern uint8_t g_tcp_conn_state;

void start_kaco_tcp_server(void);
void tcp_damon(uint32_t *discnt_time);

#endif