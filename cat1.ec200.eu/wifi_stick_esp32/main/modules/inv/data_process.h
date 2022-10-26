
#ifndef DATA_PROCESS_H
#define DATA_PROCESS_H
#include "stdint.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#define SAVE_TIMING 300
#define INV_NUM 15

// enum UART_TYPE
// {
//     MAIN_UART = 0,
//     CAT1_UART = 2
// };

#define MAIN_UART 0
#define CAT1_UART 2

extern int total_rate_power;
extern char g_p2p_mode;
extern SemaphoreHandle_t invdata_store_sema;
extern char is_in_cloud_main_loop;
extern char g_meter_data_sync_enable;
extern char *g_cst_pub_topic;

typedef struct
{
    int active;
    char topic[100];
    char payload[1500];
} cld_rrpc_resp_t;

typedef struct
{
    int fd;
    uint8_t slave_id;
    uint16_t start_addr;
    uint16_t reg_num;
} mb_req_t;

typedef struct
{
    uint8_t frame[500];
    uint16_t len;
} mb_res_t;

typedef struct
{
    char file_name[50];
    char file_path[80];
    char part_num[50];
    int file_type;
    int file_len;
} Update_t, *Update_p;

typedef struct
{
    char uri[150];

    char device[50];
    char secret[50];
    char sn[50];
    char filename[50];
    char info[50];

    char action[50];
    /** 由于post的body比较大，使用动态内存 */
    char *body;
    int body_len;
    Update_p update_info;
} http_req_para_t;

typedef struct tagCur_Voltg
{
    uint16_t iVol;
    uint16_t iCur;
} Cur_Voltg, *Cur_Voltg_ptr;

typedef struct tagInv_data
{
    char psn[50];
    char time[64];
    uint16_t grid_voltg;        /// the grid voltage
    uint32_t e_today;           ///< The accumulated kWh of that day  ///meter E-Today(out)
    uint32_t e_total;           ///< Total energy to grid             ///meter e_total(in)
    uint32_t h_total;           ///< Total operation hours            ///meter E-Total( out) (old msg)
    uint8_t status;             ///< Inverter operation status
    uint16_t chk_tim;           ///< the check time of inverter
    int16_t col_temp;           ///< the cooling fin temperature
    int16_t U_temp;             ///< the inverter U phase temperature
    int16_t V_temp;             ///< the inverter V temperature
    int16_t W_temp;             ///< the inverter W temperature
    int16_t contrl_temp;        ///< the control board temperature
    int16_t rsv_temp;           ///< the cooling fin temperature
    int16_t bus_voltg;          ///< bus voltage                              ///meter Phase angle
    uint32_t con_stu;           ///< connect status   special for smart meter ///meter E-Today(in)
    Cur_Voltg PV_cur_voltg[10]; ///< the 10 voltage and current pair
    uint16_t istr[20];          ///<string current 1~20
    Cur_Voltg AC_vol_cur[3];    ///< L1,L2,L3 voltage and current
    uint16_t rs_voltg;          // rs line voltage
    uint16_t rt_voltg;          // rt line voltage
    uint16_t st_voltg;          // st line voltage
    uint16_t fac;               ///< AC frequence
    uint32_t sac;               ///< Total power to grid(3Phase)    ///meter SAC
    uint32_t pac;               ///< Total power to grid(3Phase)    ///meter pac
    uint32_t qac;               ///< Total power to grid(3Phase)    ///meter QAC
    int8_t cosphi;              ///< the AC phase                   ///meter cosphi
    uint8_t pac_stu;            ///< limit power status 0: not limit power , 1:limit power
    uint8_t err_stu;            ///< the error status   0: there is not error accident, 1: there is an error accident
    uint16_t error;             ///< Failure description for status 'failure'
    uint8_t warn[10];           ///< Failure description for status 'failure'
    uint8_t dc_sample;          ///< the Zevercloud sample time
    uint32_t rtc_time;          ///< the occure time
} Inv_data, *Inv_data_ptr;
enum ALL_MSG_TYPE
{
    MSG_FDBG,
    MSG_UPDATE_INV_MASTER,
    MSG_UPDATE_INV_SAFETY,
    MSG_SCAN_INVERTER
};
typedef struct
{
    int type;
    int len;
    char *data;
} fdbg_msg_t;

typedef struct
{
    long type;
    Inv_data data;
} Inv_data_msg_t;

/*! \brief This struct describes the attributes of inverter  */
// typedef struct tagInvDevice {
//     uint8_t          type ;        ///0x31: single phase inverter, 0x33 three phase inverter
//     uint8_t          md_add;       ///modbus address and zeversolar communication address
//     uint8_t          sn[33];       ///the inverter SN.
//     uint8_t          mode_name[17];///the inverter made name "ZL3000"
//     uint8_t          safety_type;  ///the inverter current safety
//     uint32_t         rated_pwr;    ///the inverter rate power
//     uint8_t          msw_ver[15];  ///master software version AISWEI料号: "V610-01037-00"
//     uint8_t          ssw_ver[15];  ///slave software version  AISWEI料号: "V610-01038-00"
//     uint8_t          tmc_ver[15];  ///safety software version AISWEI料号: "V610-10000-00"
//     uint8_t          protocol_ver[13];///the inverter communicate protocol version "MV2.00AV2.00"
//     uint8_t          mfr[17];      ///inverter manufacture "ZEVERSOLAR"
//     uint8_t          brand[17];    ///the inverter brand   "ZEVERSOLAR"
//     uint8_t          mach_type;    ///the inverter type 1-单相机1-3kw   2-单相机3-6kw
//     uint8_t          pv_num;       ///the inverter pv number
//     uint8_t          str_num;      ///the inverter string number
// }InvDevice, *InvDevice_ptr;

/*! \brief This struct describes the attributes of inverter  */
/*typedef struct tagInverter {

    uint16_t     last_error;
    uint16_t     last_warn;
    InvDevice   device;
    Inv_data    data;
    uint8_t      status;
    uint8_t      addr;
    uint32_t     online_time;
    uint32_t     sampnum;                //used to calc the average of pac
    int32_t     pac_sum;                //used to calc the average of pac
    uint32_t     fir_etotal;             //The current day's first E_Total value
    uint32_t     fir_etotal_out;         //The current day's first E_Total value
    uint8_t      private_data[64];       //TShe inverter's describe table
    uint8_t      syn_time;               //0: no synchronize time, 1: had synchronize yet
}Inverter, *Inverter_ptr;
*/

/*! \brief This struct describes the attributes of Cluster  */
typedef struct tagMonitor
{
    int8_t sn[16];
    int16_t type;
    int8_t name[13];
    int8_t model[13];
    int8_t mfr[16];
    int8_t brand[16];
    int8_t hw_ver[20];
    int8_t reg_key[17];
    int8_t prd_key[33];    //ProductKey
    int8_t dev_scrt[33];   //device screct len = 32
    int8_t alilyun_ip[33]; //alilyun ip
    int16_t alilyun_port;  //alilyun_port

    int8_t sw_ver[20];
    int8_t md_ver1[20];

    uint8_t inv_cunt; //0: automatically collect inverter data,the modbus address is 3. 1:read inverte list from flashs
    int8_t meter_en;  //whether is enable the collect meter data
    int8_t meter_mod; //the meter module type
    int8_t meter_add; //default is 1, maybe not set
} Monitor, *Monitor_ptr;

/*! \brief This struct describes the attributes of Cluster  */
// typedef struct tagInvList {
//     uint8_t sn[33];
//     uint8_t addr;
// }InvList, *InvList_ptr;

typedef struct
{
    char psn[64]; //device name
    char product_key[64];
    char device_secret[64];
    // int profile;
    // int pdptype;
    // int authproto;
    char apn[51];
    // char username[51];
    // char password[51];

    char host[128];
    int port;
    //int req_time_ms;
    //int keep_alive_ms;
    //int write_buf_size;
    //int read_buf_size;

    //int inv_cunt;
    char reg_key[51];
    char hw[51];
    char brw[51];
    char mfr[51];
    char nam[51];
    //char sw[51];
    //char sys[51];
    char md1[51];
    char md2[51];
    char cmd[51];
    int typ;
    char mod[51];
    char wsw[51];
    int meter_en;
    int meter_mod;
    int meter_add;
} Setting_Para, *Setting_Para_Ptr;

typedef struct
{
    char host[128];
    int port;
    char client_id[150];
    char username[64];
    char password[65];
    char pub_topic[100];
    char sub_topic[100];
    int ssl_enable;
} _3rd_mqtt_para_t;

typedef struct
{
    char apn[128];
    char username[128];
    char password[128];
} apn_para_t;

typedef char ssl_file_t[2000];

typedef struct
{
    char ssid[32];
    char password[64];
} wifi_sta_para_t;

typedef struct
{
    char state[50];
} Update_State, *Update_State_Ptr;
/*
typedef struct
{
    int type;
    int len;
    char data[1024];
    int ws;
} msg;*/

/*! \brief This struct describes the attributes of inverter  */
//static part, need to save.
typedef struct
{
    uint8_t type;             ///0x31: single phase inverter, 0x33 three phase inverter
    uint8_t modbus_id;        ///modbus address and zeversolar communication address
    uint8_t sn[33];           ///the inverter SN.
    uint8_t mode_name[17];    ///the inverter made name "ZL3000"
    uint8_t safety_type;      ///the inverter current safety
    uint32_t rated_pwr;       ///the inverter rate power
    uint8_t msw_ver[15];      ///master software version AISWEI料号: "V610-01037-00"
    uint8_t ssw_ver[15];      ///slave software version  AISWEI料号: "V610-01038-00"
    uint8_t tmc_ver[15];      ///safety software version AISWEI料号: "V610-10000-00"
    uint8_t protocol_ver[13]; ///the inverter communicate protocol version "MV2.00AV2.00"
    uint8_t mfr[17];          ///inverter manufacture "ZEVERSOLAR"
    uint8_t brand[17];        ///the inverter brand   "ZEVERSOLAR"
    uint8_t mach_type;        ///the inverter type 1-单相机1-3kw   2-单相机3-6kw
    uint8_t pv_num;           ///the inverter pv number
    uint8_t str_num;          ///the inverter string number
    uint8_t syn_time;         //0: no synchronize time, 1: had synchronize yet
} InvRegister, *InvRegister_ptr;

typedef InvRegister Inv_reg_arr_t[INV_NUM]; //should save to database.

/*! \brief This struct describes the attributes of inverter  */
//total:static and dynamic part.don't save dynamic part.
typedef struct
{
    uint16_t last_error;
    uint16_t last_warn;
    InvRegister regInfo;
    uint8_t status;
    uint32_t online_time;
    uint32_t sampnum;         //used to calc the average of pac
    int32_t pac_sum;          //used to calc the average of pac
    uint32_t fir_etotal;      //The current day's first E_Total value
    uint32_t fir_etotal_out;  //The current day's first E_Total value
    uint8_t private_data[64]; //TShe inverter's describe table
    Inv_data invdata;
    uint32_t last_save_time;
} Inverter, *Inverter_ptr;

typedef Inverter Inv_arr_t[INV_NUM];

typedef struct
{
    char type;
    char ws;
    char data[512];
    char len;
} cloud_inv_msg;

typedef struct
{
    char ssid[32];
    char password[64];
} wifi_ap_para_t;

typedef char ate_reboot_t[50];

/*status of monitor*/
#define SYNC_TIME_FROM_CLOUD_INDEX 0X01 // status means whether synchronize the time from cloud
#define SYNC_TIME_FROM_INV_INDEX 0X02   // status means whether synchronize the time from inverter
#define MON_OTA_CMD_INDEX 0X04          // status means there is a inverter with error serial number.// unuseful
#define MON_OTA_INV_CMD_INDEX 0X08      // status means rtc time is error.// unuseful

#define INV_FOUND_METER_INDEX 0X10  // status means there is a smart meter in system.
#define INV_SCAN_STATE_INDEX 0X20   // status means whether monitor is in scanning
#define INV_REGIST_STATE_INDEX 0X40 // status means whether monitor has sent the registry command.

#define INV_SYNC_DEV_INDEX 0X01      // status means whether synchronize the inverter information.
#define INV_CHECK_ONLINE_INDEX 0X02  // status means whether needs to check the inverter information; unused
#define INV_ONLINE_STATUS_INDEX 0X04 // status means whether the inverter is on line.
#define INV_SYNC_INV_STATE 0X08
#define INV_SYNC_PMU_STATE 0X10
#define START_ADD 0x03

//int save_inv_data(Inv_data, unsigned int *time);
//int save_inv_data(Inv_data p_data, unsigned int *time, unsigned short * error_code);
#endif
