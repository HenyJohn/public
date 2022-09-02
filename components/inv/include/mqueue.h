/**
 * @file mqueue.h
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2022-04-07
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef MQUEUE_H
#define MQUEUE_H

#include "Asw_global.h"
#include "freertos/queue.h"
#include "data_process.h"


// #include <stdio.h>
// #include "mqueue.h"
// #include "freertos/queue.h"

// #include "esp_log.h"
#include "meter.h"

QueueHandle_t mq0;
QueueHandle_t mq1;
QueueHandle_t mq2;
QueueHandle_t to_inv_fdbg_mq;
QueueHandle_t to_cgi_fdbg_mq;
QueueHandle_t to_tcp_task_mq;

int8_t create_queue_msg(void);

#endif