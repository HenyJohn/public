#include "stdint.h"
#include "stdio.h"
#include "string.h"
#include "esp_flash.h"
#include "esp_flash_spi_init.h"
#include "esp_partition.h"
#include "esp_vfs.h"
#include "esp_vfs_fat.h"
#include "esp_system.h"
#include <stdlib.h>
#include "mqueue.h"
#include "data_process.h"

extern int netework_state;
extern int8_t monitor_state;
// extern int mqtt_connect_state;
// extern int ap_enable;
extern int inv_com_reboot_flg;


int write_lost_data_cst(Inv_data * inv_data)
{   
    if(check_inv_pv(inv_data) < 0)
        return -1;

    FILE *fp = fopen("/inv/cstlost.db", "ab+");//"wb"
    if (fp == NULL) {
        ESP_LOGE("cstff", "Failed to open file for writing");
        return -2;
    }

    fseek(fp, 0, SEEK_END);
    ESP_LOGE("CSTTAG", "file len  %d \n", ftell(fp));    

    if(ftell(fp) > 1827000/2)         //5 invs 7day 1827000
    {
        ESP_LOGE("cst write lostdata", "file too large move it");
        fclose(fp);
        transfer_another_file_cst();
        
        FILE *fp = fopen("/inv/cstlost.db", "ab+");//"wb"
        if (fp == NULL) {
            ESP_LOGE("cstff", "Failed to open file for writing");
            return -3;
        }
    }

    fseek(fp, 0, SEEK_SET);        

    char buff[300]={0};
    memcpy(buff, (char *)inv_data, sizeof(Inv_data));

    uint16_t crc = crc16_calc(buff, sizeof(Inv_data));
    buff[sizeof(Inv_data)] =    crc & 0xFF;
    buff[sizeof(Inv_data)+1] = (crc >> 8)& 0xFF;
    int i=0;

    fwrite(buff , sizeof(char), sizeof(Inv_data)+2, fp);

    fclose(fp);
    return 0;
}


int write_lost_index_cst(int index)
{
    if((-1 == check_file_exist("/inv/cstlost.db")) || (index<1))
        return -1;

    if(access("/inv/cstindex.db", 0) != 0)
    {
        ESP_LOGE("cstwrite_lost_index", "index.db not existed \n");
        if(index > 1) 
            return -1;
    }

    FILE *fp = fopen("/inv/cstindex.db", "wb");//"wb"
    if (fp == NULL) {
        ESP_LOGE("cstff", "Failed to open file for writing");
        return -1;
    }
    char buff[32]={0};
   
    sprintf(buff, "%d", index);
    ESP_LOGE("cstwrite index", "all write*%d#%s#--- ", index, buff);
    fwrite(buff , sizeof(char), sizeof(buff), fp);
    fclose(fp);
    return 0;
}

int read_lost_index_cst(void)
{
    int tmp_index=0;

    if(access("/inv/cstindex.db", 0) != 0)
    {
        ESP_LOGE("cstread_index_data", "file don't existed \n");
        return -1;
    }

    FILE *fp = fopen("/inv/cstindex.db", "rb");
    if (fp == NULL) {
        ESP_LOGE("cstread_lost_index", "Failed to open file for readindex");
        return 0;
    }

    char buff[32]={0};
    char i=0;
    i=fread(buff, sizeof(char), 32, fp);
    ESP_LOGE("cstread index", "all read*%d#%s#--- ", i, buff);

    tmp_index = atoi(buff);
    ESP_LOGE("cstread index", "read index is %d \n", tmp_index);
    if(tmp_index < 8000/2)
        return tmp_index;
    else
        return 0;
}

int transfer_another_file_cst(void)
{
    FILE *new_fp = NULL;
    int nread = 0;
    int blob_size = 0;
    char buf[5900] = {0};
    int read_index = 0;
    char days[10]={0};
    int line=0;

    FILE *fp = fopen("/inv/cstlost.db", "rb");
    if (fp == NULL) {
        ESP_LOGE("cst_read_lost_data", "Failed to open file for lostdata");
        return -1;
    }

    blob_size = sizeof(Inv_data) + 2;

    line=find_new_days(fp, days);
    if(line > 0)
    {
        fseek(fp, blob_size * line, SEEK_SET);
        line=find_next_day(fp, days, line);
        if(line < 2)
        {
            fclose(fp);
            remove("/inv/cstlost.db");
            remove("/inv/cstindex.db");
            ESP_LOGE("cst move data", "all data repeat delet lost.db \n");
            return -1;
        }
    }
    else
    {
        fclose(fp);
        remove("/inv/cstlost.db");
        remove("/inv/cstindex.db");
        ESP_LOGE("cst move data", "not found available day delet lost.db\n");
        return -1;
    }
    // ESP_LOGE("transfer_another_file ", "reseek line  %d \n",  line);   
    if (fp != NULL)
    {
        fseek(fp, blob_size * line, SEEK_SET);
        new_fp = fopen("/inv/cstnew.db", "wb");
        while (!feof(fp)) // to read file
        {
            nread = fread(buf, 1, blob_size*20, fp);
            ESP_LOGE("cst move data", "read bytes %d \n", nread);

            if ( 0 == (nread % blob_size))
            {
                fwrite(buf, 1, nread, new_fp);
            }
            usleep(10000);
        }
        fclose(fp);
        fclose(new_fp);
        remove("/inv/cstlost.db");
        rename("/inv/cstnew.db", "/inv/cstlost.db");
        read_index = read_lost_index_cst();
        // ESP_LOGE("transfer_another_file ", "redefine index %d %d \n", read_index, line);
        if (read_index > line)
        {
            write_lost_index_cst(read_index - line);
        }
        else
        {
            remove("/inv/cstindex.db");
        }
    }
    return 0;
}


int read_lost_data_cst(Inv_data * inv_data, int * line)
{
    if(access("/inv/cstlost.db", 0) != 0)
    {
        ESP_LOGE("CSTread_lost_data", "file don't existed \n");
        return -1;
    }

    if(*line < 0)
        *line = 0;
        
    FILE *fp = fopen("/inv/cstlost.db", "rb");
    if (fp == NULL) {
        ESP_LOGE("CSTTAG", "Failed to open file for reading");
        //remove("/inv/cstlost.db");
        return -1;
    }
    char buff[300]={0};
    int tmp_line=0;
    tmp_line = *line;

    fseek(fp, (*line)*290, SEEK_SET);

    //while (!feof(fp) )
    {
        tmp_line++;
        fread(buff, sizeof(char), sizeof(Inv_data)+2, fp);
        ESP_LOGE("TAG", "cstall read--%d--%d--  %s ", tmp_line, *line, buff);

        if(tmp_line > *line)
        {
            ESP_LOGE("cstTAG", "find new line --%d--%d--  %s ", tmp_line, *line, buff);
            //break;
        }
    }

    if(feof(fp))
    {
        ESP_LOGE("read lost data","read file end delete file------------>>>>>>\n");
        remove("/inv/cstlost.db");
        remove("/inv/cstindex.db");
    }
    fclose(fp);
    
    int i=0;
    printf("cstread put invdata %d crc=%d \n", sizeof(Inv_data)+2, crc16_calc(buff, sizeof(Inv_data)+2));

    if(0!=crc16_calc(buff, sizeof(Inv_data)+2))
    {
        printf("cstread lost data crc error \n");
        *line=tmp_line;
        return -2;
    }

    for(;i<sizeof(Inv_data)+2; i++)
        printf("%02x ", buff[i]);
    printf("cstreadput end\n");


    memcpy(inv_data, buff, sizeof(Inv_data));
    *line=tmp_line;
    ESP_LOGE("cstTAG", "read %d 'st line  invdata time %s %s\n", *line, inv_data->time, inv_data->psn);
    return 0;
}