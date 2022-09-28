#include "asw_fat.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "esp_vfs.h"
#include "esp_vfs_fat.h"
#include "esp_system.h"
#include <dirent.h>
#include "cJSON.h"
#include "diskio_impl.h"
#include "vfs_fat_internal.h"
#include "log.h"
//FAT based on wear leveling
extern int write_log_enable;

static const char *TAG = "asw_fat.c";

// Handle of the wear levelling library instance
static wl_handle_t s_wl_handle = WL_INVALID_HANDLE;

// Mount path for the partition
const char *base_path = "/home";
sys_log net_log = {0};
// mount path
int asw_fat_mount(void)
{
    // ESP_LOGI(TAG, "Mounting FAT filesystem");
    // To mount device we need name of device partition, define base_path
    // and allow format partition in case if it is new one and was not formated before
    const esp_vfs_fat_mount_config_t mount_config = {
        .max_files = 4,
        .format_if_mount_failed = true,
        .allocation_unit_size = CONFIG_WL_SECTOR_SIZE};
    esp_err_t err = esp_vfs_fat_spiflash_mount(base_path, "storage", &mount_config, &s_wl_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to mount storage FATFS (%s)", esp_err_to_name(err));
        return -1;
    }
    ESP_LOGI(TAG, "Mount storage FAT OK");
    return 0;
}

int asw_fat_release(void)
{
    ESP_LOGI(TAG, "Unmounting FAT");
    ESP_ERROR_CHECK(esp_vfs_fat_spiflash_unmount(base_path, s_wl_handle));
}

// Mount path for the partition
const char *data_path = "/inv";

// mount path
int inv_fat_mount(void)
{
    // ESP_LOGI(TAG, "Mounting FAT filesystem");
    // To mount device we need name of device partition, define base_path
    // and allow format partition in case if it is new one and was not formated before
    const esp_vfs_fat_mount_config_t mount_config = {
        .max_files = 4,
        .format_if_mount_failed = true,
        .allocation_unit_size = CONFIG_WL_SECTOR_SIZE};
    esp_err_t err = esp_vfs_fat_spiflash_mount(data_path, "invdata", &mount_config, &s_wl_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to mount invdata FATFS (%s)", esp_err_to_name(err));
        return -1;
    }
    ESP_LOGI(TAG, "Mount invdata FAT OK");
    return 0;
}

static void clear_dir(char *path)
{
    // These are data types defined in the "dirent" header
    DIR *theFolder = opendir(path);
    struct dirent *next_file;
    char filepath[256];

    while ((next_file = readdir(theFolder)) != NULL)
    {
        // build the path for each file in the folder
        if (strcmp(next_file->d_name, ".") == 0 || strcmp(next_file->d_name, "..") == 0)
            continue;
        sprintf(filepath, "%s/%s", path, next_file->d_name);
        remove(filepath);
        printf("removed: %s\n", filepath);
    }
    closedir(theFolder);
}

int get_file_size(char *path)
{
    FILE *fp = NULL;
    long lSize;

    fp = fopen(path, "rb");
    if (fp == NULL)
        return 0;
    fseek(fp, 0, SEEK_END);
    lSize = ftell(fp);
    fclose(fp);

    return (int)lSize;
}

void make_lower_case(char *str, int len)
{
    int i;
    for (i = 0; i < len; i++)
    {
        str[i] = tolower(str[i]);
    }
}

static void get_file_list_from_dir(char *path, cJSON *arr)
{
    struct dirent *next_file;
    char filepath[256] = {0};

    DIR *theFolder = opendir(path);
    while ((next_file = readdir(theFolder)) != NULL)
    {
        memset(filepath, 0, sizeof(filepath));
        if (strcmp(next_file->d_name, ".") == 0 || strcmp(next_file->d_name, "..") == 0)
            continue;
        sprintf(filepath, "%s/%s \r\n", path, next_file->d_name);
        if (check_file_exist(filepath) < 0)
            continue;

        int filesize = get_file_size(filepath);
        if (filesize > 0)
        {
            cJSON *item = cJSON_CreateObject();
            make_lower_case(next_file->d_name, strlen(next_file->d_name));
            cJSON_AddStringToObject(item, "file", next_file->d_name);
            cJSON_AddNumberToObject(item, "size", filesize);
            cJSON_AddItemToArray(arr, item);
        }
    }
    closedir(theFolder);
}

void clear_file_system(void)
{
    clear_dir(base_path);
    clear_dir(data_path);
}

void get_file_list(char *msg)
{
    cJSON *res = cJSON_CreateObject();
    cJSON *myArr = NULL;
    char *json_str = NULL;

    myArr = cJSON_AddArrayToObject(res, "filelist");

    get_file_list_from_dir(base_path, myArr);
    // get_file_list_from_dir(data_path, myArr);
    json_str = cJSON_PrintUnformatted(res);
    memcpy(msg, json_str, strlen(json_str));
    free(json_str);
    cJSON_Delete(res);
}

/** net log file***************************************************************/

// int netlog_write(char *msg)
// {
//     char *path = "/home/netlog.txt";
//     if (get_file_size(path) > 2048)
//     {
//         remove(path);
//     }

//     FILE *fp = fopen("/home/netlog.txt", "a+");
//     if (fp != NULL)
//     {
//         fwrite(msg, 1, strlen(msg), fp);
//         fclose(fp);
//     }
// }

int netlog_write(int msgid)
{
    char *path = "/home/netlog.txt";
    int er = -1;
    char now_time[30] = {0};
    char buf[300] = {0};

    get_time(now_time, sizeof(now_time));
    // get_sta_ip(ip);

    sprintf(buf, "%s %d\n", now_time, msgid);

    if (get_file_size(path) > 204800)
    {
        remove(path);
    }

    FILE *fp = fopen("/home/netlog.txt", "a+");
    if (fp != NULL)
    {
        er = fwrite(buf, 1, strlen(buf), fp);
        fclose(fp);
    }
}

extern char g_ip[17];
int network_log(char *msg)
{
    char now_time[30] = {0};
    char ip[30] = {0};
    char buf[300] = {0};

    get_time(now_time, sizeof(now_time));
    // get_sta_ip(ip);

    sprintf(buf, "%s ip:%s %s\r\n", now_time, g_ip, msg);
    //printf("write_log_enable#################### %d \n", write_log_enable);

    //if(write_log_enable)
    //  netlog_write(buf);

    // if((running_log_index + strlen(buf)) > 1024)
    // {
    //     memset(running_log, 0, 1024);
    //     running_log_index = 0;
    // }

    // memcpy(&running_log[running_log_index], buf, strlen(buf));
    // running_log_index += strlen(buf);

    if (strlen(net_log.tag) == 0)
        memcpy(net_log.tag, "Internet info\r\n", strlen("Internet info\r\n"));
    //printf("write_log_enab#######````````````` %s %d %d %d\n", net_log.tag, net_log.index+ strlen(buf), net_log.index,strlen(buf));
    if (net_log.index + strlen(buf) > 329)
    {
        char tmp[160] = {0};

        if (net_log.index > 260)
            memcpy(tmp, &net_log.log[100], 160);
        else if (net_log.index > 100 && net_log.index < 261)
            memcpy(tmp, &net_log.log[100], net_log.index - 100);

        memset(net_log.log, 0, sizeof(net_log.log));
        net_log.index = 0;

        memcpy(net_log.log, tmp, strlen(tmp));
        net_log.log[160] = '\n';
        memcpy(&net_log.log[161], buf, strlen(buf));

        net_log.index += (161 + strlen(buf));
        //printf("========================net_log new index---- %d \n", net_log.index);
    }
    else
    {
        memcpy(&net_log.log[net_log.index], buf, strlen(buf));
        net_log.index += strlen(buf);
    }

    if (net_log.index > 330)
    {
        memset(net_log.log, 0, sizeof(net_log.log));
        net_log.index = 0;
    }
}

int get_sys_log(char *buf)
{
    // memcpy(buf, running_log, strlen(running_log));
    // return 0;
    memcpy(buf, net_log.tag, strlen(net_log.tag));
    memcpy(&buf[strlen(net_log.tag)], net_log.log, net_log.index);
    return 0;
}
//invdata

int force_flash_reformart(char *partion)
{
    const esp_vfs_fat_mount_config_t mount_config = {
        .max_files = 4,
        .format_if_mount_failed = true,
        .allocation_unit_size = CONFIG_WL_SECTOR_SIZE};

    esp_vfs_fat_spiflash_unmount(base_path, s_wl_handle);
    esp_err_t err = esp_vfs_fat_spiflash_mount_force_format(base_path, partion, &mount_config, &s_wl_handle);
    printf("force_flash_reformart %s \n", partion);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_vfs_fat_spiflash_mount_force_format Failed to mount storage FATFS (%s)", esp_err_to_name(err));
        force_format(partion);
    }
    ESP_LOGI(TAG, "Mount storage FAT OK");
}

int force_format(char *partname)
{
    const esp_partition_t *partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, partname);
    assert(partition != NULL);

    ESP_ERROR_CHECK(esp_partition_erase_range(partition, 0, partition->size));
    printf("######force format %s#######\n", partname);
}

esp_err_t esp_vfs_fat_spiflash_mount_force_format(const char *base_path,
                                                  const char *partition_label,
                                                  const esp_vfs_fat_mount_config_t *mount_config,
                                                  wl_handle_t *wl_handle)
{
    esp_err_t result = ESP_OK;
    const size_t workbuf_size = 4096;
    void *workbuf = NULL;

    esp_partition_subtype_t subtype = partition_label ? ESP_PARTITION_SUBTYPE_ANY : ESP_PARTITION_SUBTYPE_DATA_FAT;
    const esp_partition_t *data_partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA,
                                                                     subtype, partition_label);

    printf("######esp_vfs_fat_spiflash_mount_force_format#######\n");
    if (data_partition == NULL)
    {
        ESP_LOGE(TAG, "Failed to find FATFS partition (type='data', subtype='fat', partition_label='%s'). Check the partition table.", partition_label);
        return ESP_ERR_NOT_FOUND;
    }

    result = wl_mount(data_partition, wl_handle);
    if (result != ESP_OK)
    {
        ESP_LOGE(TAG, "failed to mount wear levelling layer. result = %i", result);
        return result;
    }
    // connect driver to FATFS
    BYTE pdrv = 0xFF;
    if (ff_diskio_get_drive(&pdrv) != ESP_OK)
    {
        ESP_LOGD(TAG, "the maximum count of volumes is already mounted");
        return ESP_ERR_NO_MEM;
    }
    ESP_LOGD(TAG, "using pdrv=%i", pdrv);
    char drv[3] = {(char)('0' + pdrv), ':', 0};

    result = ff_diskio_register_wl_partition(pdrv, *wl_handle);
    if (result != ESP_OK)
    {
        ESP_LOGE(TAG, "ff_diskio_register_wl_partition failed pdrv=%i, error - 0x(%x)", pdrv, result);
        goto fail;
    }
    FATFS *fs;
    result = esp_vfs_fat_register(base_path, drv, mount_config->max_files, &fs);
    if (result == ESP_ERR_INVALID_STATE)
    {
        // it's okay, already registered with VFS
    }
    else if (result != ESP_OK)
    {
        ESP_LOGD(TAG, "esp_vfs_fat_spiflash_mount_force_format esp_vfs_fat_register failed 0x(%x)", result);
        goto fail;
    }

    FRESULT fresult = f_mount(fs, drv, 1);

    ESP_LOGW(TAG, "esp_vfs_fat_spiflash_mount_force_format f_mount failed (%d)", fresult);
    workbuf = ff_memalloc(workbuf_size);
    if (workbuf == NULL)
    {
        result = ESP_ERR_NO_MEM;
        goto fail;
    }
    size_t alloc_unit_size = esp_vfs_fat_get_allocation_unit_size(
        CONFIG_WL_SECTOR_SIZE,
        mount_config->allocation_unit_size);
    ESP_LOGI(TAG, "esp_vfs_fat_spiflash_mount_force_format FATFS partition, allocation unit size=%d", alloc_unit_size);
    fresult = f_mkfs(drv, FM_ANY | FM_SFD, alloc_unit_size, workbuf, workbuf_size);
    if (fresult != FR_OK)
    {
        result = ESP_FAIL;
        ESP_LOGE(TAG, "f_mkfs failed (%d)", fresult);
        goto fail;
    }
    free(workbuf);
    workbuf = NULL;
    ESP_LOGI(TAG, "esp_vfs_fat_spiflash_mount_force_format Mounting again");
    fresult = f_mount(fs, drv, 0);
    if (fresult != FR_OK)
    {
        result = ESP_FAIL;
        ESP_LOGE(TAG, "f_mount failed after formatting (%d)", fresult);
        goto fail;
    }
    return ESP_OK;

fail:
    free(workbuf);
    esp_vfs_fat_unregister_path(base_path);
    ff_diskio_unregister(pdrv);
    return result;
}

int read_temp_file(char *netlogx)
{
    char msg[500] = {0};
    char *netlog = "/home/netlog.txt";

    FILE *fp = NULL;
    fp = fopen(netlog, "r");
    if (fp != NULL)
    {
        char buf[100] = {0};
        while (!feof(fp)) // to read file
        {
            memset(buf, 0, sizeof(buf));
            int nread = fread(buf, 1, 100, fp);
            //printf("file %d############# %s \n",strlen(buf), buf);
            if (strlen(buf) < 2)
            {
                printf("FAT SYS error\n");
                return -1;
            }
        }
        fclose(fp);
    }
    else
    {
        printf("open file fail\n");
    }
    printf("FAT SYS OK\n");
    return 0;
}