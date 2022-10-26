#include <stdio.h>
#include "mqueue.h"
#include "freertos/queue.h"
#include "data_process.h"
#include "esp_log.h"
#include "meter.h"

static const char *TAG = "mqueue";

int create_queue_msg(void)
{
    mq0 = xQueueCreate(5, sizeof(Inv_data)); //inv--->cloud
    if (mq0 == NULL)
    {
        ESP_LOGI(TAG, "create mq0 error");
        return -1;
    }

    mq1 = xQueueCreate(1, sizeof(cloud_inv_msg)); //cloud--->inv
    if (mq1 == NULL)
    {
        ESP_LOGI(TAG, "create mq1 error");
        return -1;
    }

    mq2 = xQueueCreate(1, sizeof(meter_cloud_data)); //meter-->cloud
    if (mq2 == NULL)
    {
        ESP_LOGI(TAG, "create mq2 error");
        return -1;
    }
    /** cgi和inv之间，通过2个消息队列实现双向通信*/
    to_inv_fdbg_mq = xQueueCreate(1, sizeof(cloud_inv_msg));
    if (to_inv_fdbg_mq == NULL)
    {
        ESP_LOGI(TAG, "create error: to_inv_fdbg_mq");
        return -1;
    }

    to_cgi_fdbg_mq = xQueueCreate(1, sizeof(fdbg_msg_t));
    if (to_cgi_fdbg_mq == NULL)
    {
        ESP_LOGI(TAG, "create error: to_cgi_fdbg_mq");
        return -1;
    }

    _3rd_mq = xQueueCreate(5, sizeof(Inv_data));
    if (_3rd_mq == NULL)
    {
        ESP_LOGI(TAG, "create error: _3rd_mq");
        return -1;
    }
    return 0;
}