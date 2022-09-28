#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>

#include "esp_timer.h"
#include <stdio.h>
#include <stdint.h>
#include "data_process.h"
#include "mqueue.h"
#include "inv_task.h"
#include "cloud_task.h"
#include "data_process.h"
#include "asw_store.h"
#include <sys/types.h>
#include <dirent.h>
#include "common.h"
#include "esp_log.h"

extern void start_new_server(void);
extern void scan_ap_once(void);
extern void cat1_cloud_task(void *pvParameters);
extern void led_task(void *pvParameters);
// extern void smartconfig_led(void *pvParameters);

// TaskHandle_t cgi_task_handle = NULL;
TaskHandle_t inv_task_handle = NULL;
TaskHandle_t cloud_task_handle = NULL;
TaskHandle_t led_task_handle = NULL;
void app_main()
{
    global_mutex_init();
    boot_rollback_check();
    BaseType_t xReturned;

    // work_mode = AP_PROV;
    // wifi_sta_para_t sta_para = {0};
    ESP_LOGI("STARTING", "total memory: %d bytes,monitor VER %s", esp_get_free_heap_size(), "2021101-003R 1.18");
    asw_nvs_init();
    asw_db_open();

    asw_fat_mount();
    inv_fat_mount();

    asw_ap_start();

    create_queue_msg();

    remove("/home/inv.bin");
    remove("/home/update.bin");
    //remove("/home/netlog.txt");
    // remove("/inv/lost.db");
    // remove("/inv/index.db");
    //remove("/inv/lost.db");
    /* Create the task, storing the handle. */

    //vTaskSuspend(NULL);
    gpio_ini();

    xReturned = xTaskCreate(
        inv_task,     /* Function that implements the task. */
        "inv task",   /* Text name for the task. */
        10240 + 2048, /* Stack size in words, not bytes. 50K = 25600 */
        (void *)1,    /* Parameter passed into the task. */
        5,            /* Priority at which the task is created. */
        &inv_task_handle);
    if (xReturned != pdPASS)
    {
        printf("create inv task failed.\n");
    }

    xReturned = xTaskCreate(
        cloud_task,   /* Function that implements the task. */
        "cloud task", /* Text name for the task. */
        25600 + 5000, /* Stack size in words, not bytes. 50K = 25600 20K = 10240*/
        (void *)1,    /* Parameter passed into the task. */
        5,            /* Priority at which the task is created. */
        &cloud_task_handle);
    if (xReturned != pdPASS)
    {
        printf("create cloud task failed.\n");
    }

    xReturned = xTaskCreate(
        led_task,   /* Function that implements the task. */
        "led task", /* Text name for the task. */
        5120,       /* Stack size in words, not bytes. 50K = 25600 20K = 10240*/
        (void *)1,  /* Parameter passed into the task. */
        5,          /* Priority at which the task is created. */
        &led_task_handle);
    if (xReturned != pdPASS)
    {
        printf("led task failed.\n");
    }
}

// void create_wlan_configled(void)
// {
//     TaskHandle_t config_led_task_handle = NULL;
//     BaseType_t xReturned;
//     xReturned = xTaskCreate(
//         smartconfig_led,        /* Function that implements the task. */
//         "wlan config led task", /* Text name for the task. */
//         5120,                   /* Stack size in words, not bytes. 50K = 25600 20K = 10240*/
//         (void *)1,              /* Parameter passed into the task. */
//         5,                      /* Priority at which the task is created. */
//         &config_led_task_handle);
//     if (xReturned != pdPASS)
//     {
//         printf("wlan led task failed.\n");
//     }
// }

void suspend_all(void)
{
    vTaskSuspend(inv_task_handle);
    vTaskSuspend(led_task_handle);
}

void resume_all(void)
{
    vTaskResume(inv_task_handle);
    vTaskResume(led_task_handle);
}

void suspend_cloud(void)
{
    vTaskSuspend(cloud_task_handle);
}

void resume_cloud(void)
{
    vTaskResume(cloud_task_handle);
}