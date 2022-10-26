#include "data_process.h"

typedef struct
{
    int enb;
    int mod;
    int reg;
    int target;
    int date;
    int enb_pf;
    int target_pf;
} meter_setdata;
//"enb":1, "mod":1, "regulate":1, "target":2000, "date":21

typedef struct
{
    int e_total;
    int h_total;
    char day;
} meter_gendata;

typedef struct _meter_data
{
    unsigned int type;
    Inv_data data;
} meter_cloud_data;

typedef struct tagMD_RTU_WR_SNG_REQ_Packet
{
    uint8_t dev_addr;
    uint8_t fun_code;
    uint16_t reg_addr;
    uint16_t data;
} TMD_RTU_WR_SNG_REQ_Packet;

//write the multiple registry request command
typedef struct tagMD_RTU_WR_MTP_REQ_Packet
{
    uint16_t reg_addr;
    uint16_t reg_num;
    uint8_t len;
    uint8_t data[256];
} TMD_RTU_WR_MTP_REQ_Packet;
