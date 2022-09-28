#include "data_process.h"
#include "time.h"
#include "sys/time.h"
#include "device_data.h"
#include <pthread.h>
#include <stdio.h>
// #include <sys/ipc.h>
// #include <sys/msg.h>
#include <string.h>

InvRegister g_inv_info = {0};
char g_inv_ma_ver[20] = {0};
char g_inv_sf_ver[20] = {0};

//Update_t update_info = {0};
// extern pthread_mutex_t sqlite_mutex;
char g_p2p_mode = 0;
int netework_state = 1;
SemaphoreHandle_t invdata_store_sema = NULL;
char is_in_cloud_main_loop = 0;

uint16_t event_group_0 = 0;
int g_fdbg_mqtt_index = 0;

void global_mutex_init(void)
{
    invdata_store_sema = xSemaphoreCreateMutex();
}

// int inv2cloud_msg_id;
// int inv2cloud_mq_ini(void)
// {
//     key_t key;
//     key = ftok("inv2cloud", 100);
//     inv2cloud_msg_id = msgget(key, 0666 | IPC_CREAT);
// }

// int inv2ate_msg_id;
// int inv2ate_mq_ini(void)
// {
//     key_t key;
//     key = ftok("ate2inv", 100);
//     inv2ate_msg_id = msgget(key, 0666 | IPC_CREAT);
// }

// int save_inv_data(Inv_data p_data, unsigned int * time)
// {

//     Inv_data _data = p_data;
//     //save data every five minute
//     struct timeval tv;
//     //static uint32_t save_data_time = 0; // the time for sampling inverter data
//     int period = 0;

//     /*save data every 1 minte*/
//     gettimeofday(&tv, NULL);

//     period = (tv.tv_sec - *time);

//     if (period >= SAVE_TIMING || period < 0)
//     {
//         printf("save period %d\r\n", period);
//         //save_data_time = tv.tv_sec;
//         *time = tv.tv_sec;
//     }
//     else
//     {
//         return 0;
//     }
//     /* pass to Message Queue */
//     Inv_data_msg_t inv_data_msg={0};
//     inv_data_msg.type = 1;
//     memcpy(&inv_data_msg.data, &_data, sizeof(Inv_data));

//     // int flag = (inv_data_add(&_data));
//     if (0==netework_state)
//         msgsnd(inv2cloud_msg_id, &inv_data_msg, sizeof(inv_data_msg.data), IPC_NOWAIT);
//     else if(1==netework_state)
//         inv_data_add(&_data);
//     return 0;
// }

int is_found_onoff_cmd(char *ascii_data, int len)
{
    if (len == 16)
    {
        if (strncmp(ascii_data + 2, "0600C80001", 10) == 0 || strncmp(ascii_data + 2, "0600c80001", 10) == 0)
        {
            return 1;
        }
        else if (strncmp(ascii_data + 2, "0600C80000", 10) == 0 || strncmp(ascii_data + 2, "0600c80000", 10) == 0)
        {
            return 1;
        }
    }
    return 0;
}

int is_found_onoff_cmd_bytes(char *buf, int len)
{
    char pattern1[] = {0x06, 0x00, 0x0C, 0x00, 0x01};
    char pattern2[] = {0x06, 0x00, 0x0C, 0x00, 0x00};
    if (len == 8)
    {
        if (memcmp(buf + 1, pattern1, 5) == 0 || memcmp(buf + 1, pattern2, 5) == 0)
        {
            return 1;
        }
    }
    return 0;
}
