#include "inv_task.h"
#include "meter.h"
/*********************************************/
static const char *TAG = "inv_task.c";
/*********************************************/
//-------------------------------------------//
void inv_task(void *pvParameters)
{
    char trans_fun = 0;
    cloud_inv_msg p_trans_data = {0};
    SERIAL_STATE task_state = TASK_IDLE;

    ASW_LOGW("inv task start..");

    if (serialport_init(INV_UART) == 0)
    {
        ESP_LOGI(TAG, "inv uart ini ok ");
    }
    else
    {
        ESP_LOGE(TAG, "inv uart ini fail");
    }

    while (1)
    {
        switch (task_state)
        {
        case TASK_IDLE:
            task_state = inv_task_schedule(&trans_fun, &p_trans_data);
            break;

        case TASK_QUERY_DATA: // send the query data command
            task_state = query_data_proc();
            break;

        case TASK_TRANS_UP_INV:
            task_state = upinv_transdata_proc(trans_fun, p_trans_data);
            break;

        case TASK_TRANS_SCAN_INV:
            task_state = scan_register(); //
            break;

        case CLEAR_METER_SET:
            task_state = clear_meter_setting(trans_fun);
            break;

        case TASK_QUERY_METER:
            task_state = query_meter_proc();
            break;

        case RESTART:
            task_state = upinv_transdata_proc(trans_fun, p_trans_data);
            break;

        case TASK_KACO_SET_ADDR:
            task_state = md_kaco_set_addr();
            break;

        default:
            task_state = TASK_IDLE;
            break;
        }

        vTaskDelay(10 / portTICK_PERIOD_MS); // usleep(10 * 1000);
    }
}
