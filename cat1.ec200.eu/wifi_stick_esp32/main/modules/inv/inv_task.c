#include "inv_task.h"
#include "inv_com.h"
#include "data_process.h"
#include "device_data.h"
#include <stdio.h>
#include <string.h>

extern Inv_arr_t cgi_inv_arr;

void inv_task(void *pvParameters)
{
    // nvs_asw_example();
    // nvs_life_test();
    // nvs_capacity_test();
    //##################################################
    char trans_fun = 0;
    cloud_inv_msg trans_data = {0};
    //scan_register();
    printf("start_serial_thread...\r\n");
    if (serialport_init(MAIN_UART) == 0)
    {
        printf("main uart ini succ\r\n");
    }
    else
    {
        printf("main uart ini fail\r\n");
    }
    //char meter_enable=0; //initial meter enable or disable
    //scan_register();
    //while (1);

    SERIAL_STATE task_state = TASK_IDLE;

    while (1)
    {
        switch (task_state)
        {
        case TASK_IDLE:                                              //control the command order
            task_state = inv_task_schedule(&trans_fun, &trans_data); //inv_task_schedule();
            break;

        case TASK_QUERY_DATA: //send the query data command
            task_state = query_data_proc();
            break;

        case TASK_TRANS_UP_INV:
            task_state = upinv_transdata_proc(trans_fun, trans_data);
            break;

        case TASK_TRANS_SCAN_INV:
            task_state = scan_register(); //scan_register_num(trans_fun, trans_data);
            break;

        case CLEAR_METER_SET:
            task_state = clear_meter_setting(trans_fun);
            break;

        case TASK_QUERY_METER:
            task_state = query_meter_proc(0);
            break;

        case RESTART:
            task_state = upinv_transdata_proc(trans_fun, trans_data);
            break;

        default:
            task_state = TASK_IDLE;
            break;
        }
        usleep(50 * 1000);
    }
}
