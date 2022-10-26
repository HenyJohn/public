#ifndef MQUEUE_H
#define MQUEUE_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

QueueHandle_t mq0;
QueueHandle_t mq1;
QueueHandle_t mq2;
QueueHandle_t to_inv_fdbg_mq;
QueueHandle_t to_cgi_fdbg_mq;
QueueHandle_t _3rd_mq;

int create_queue_msg(void);

#endif