#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "driver/uart.h"
#include "driver/gpio.h"

extern char p2p_upgrade_inv_enable;
extern char inv_upgrade_file_type;

typedef struct
{
    char down_url[256];
    char update_type;
} update_url;

typedef struct
{
    char file_name[128];
    char file_path[128];
    char part_num[50];
    int file_type;
    int file_len;
} update_info;

typedef enum tagRst_code
{
    RST_YES = 0, //!< Success

    ERR_TIMEOUT_S = -3,    //!< Operation timed out// ERR_TIMEOUT has been defined in err.h line 69
    ERR_BAD_DATA = -4,     //!< Data integrity check failed
    ERR_PROTOCOL = -5,     //!< Protocol error
    ERR_NO_MEMORY = -7,    //!< Insufficient memory
    ERR_NO_DATA = -8,      //!< Insufficient memory
    ERR_INVALID_ARG = -9,  //!< Invalid argument
    ERR_BAD_ADDRESS = -10, //!< Bad address
    ERR_BUSY = -11,        //!< Resource is busy
    ERR_BAD_FORMAT = -12,  //!< Data format not recognized
    ERR_NO_TIMER = -13,    //!< No timer available

    ERR_FILE_NAME = -13,  //!< Error file name
    ERR_FILE_SIZE = -14,  //!< Error file size
    ERR_FILE_CHECK = -15, //!< Error file crc
    ERR_HW_CHECK = -16,   //!< File content HW check error
    ERR_SW_CHECK = -17,   //!< File content SW check error
    ERR_FILE_RULE = -18,  //!< File upate rule error

    ERR_ILLEGAL_FUNC = -20,
    ERR_INVALID_ADDR = -21,
    ERR_INVALID_VALUE = -22,
    ERR_DEVICE_BUSY = -23,
    ERR_PARITY_ERROR = -24,

    ERR_RTCS_INIT = -25,
    ERR_CREATE_SOCKET = -26,
    ERR_TCP_CONNCET = -27,
    ERR_TCP_SEND = -28,
    ERR_TCP_BIND = -29,
    ERR_LOGIN_SLD = -30,
    ERR_DNS_SERVER = -31,
    ERR_CN_ROUTER = -32,

    ERR_WIFI_SEND = -35,
    ERR_WIFI_HEAD = -36,
    ERR_WIFI_DATA = -37,
    ERR_WIFI_LENGTH = -38,

    ERR_CREATE_WEB_SERVER_TASK = -40,
    ERR_CREATE_TCP_SERVER_TASK = -41,
    ERR_CREATE_TCP_CLIENT_TASK = -42,
    ERR_CREATE_UDP_SERVER_TASK = -43,
    ERR_CREATE_UDP_CLIENT_TASK = -44,
    ERR_CREATE_FTP_SERVER_TASK = -45,
    ERR_CREATE_FTP_CLIENT_TASK = -46,

    ERR_NET_LINK = -50,
    EER_NET_INIT = -51,

    ERR_SN_EMPTY = -52,
    ERR_SN_INVALID = -53,
    ERR_SN_SPACE = -54,
    ERR_SN_LEN = -55,

    ERR_NO_UPDATE = -59,
    ERR_NO_SUPPORT = -60,

    ERR_CRC = -61,
    ERR_DEV_ADDR = -62,
    ERR_FUN_CODE = -63,
    ERR_REG_ADDR = -64,
    ERR_DATA = -65,
} rst_code,
    *rst_code_ptr;

/*! \brief Enumerate different kind of inverter commands */
typedef enum tagInverter_cmd
{
    CMD_CLR_DATA = 0,          ///< Clear data command.
    CMD_ACTIVE_PWR = 1,        ///< Active power limitation command.
    CMD_INACTIVE_PWR = 2,      ///< Inactive setpoint command.
    CMD_READ_INV_DATA = 3,     ///< Read inv data command.
    CMD_READ_SAFETY_TYPE = 4,  ///< Get inv Safety.
    CMD_WRITE_SAFETY_TYPE = 5, ///< Set inv Safety.
    CMD_READ_SAFETY_DATA = 6,  ///< Get inv Safety.
    CMD_WRITE_SAFETY_DATA = 7, ///< Set inv Safety.
    CMD_READ_ADV_PARA = 8,     ///< Get inv adv para.
    CMD_WRITE_ADV_PARA = 9,    ///< Set inv adv para.
    CMD_WRITE_START_FRAME = 10,
    CMD_WRITE_DATA_FRAME = 11,
    CMD_READ_FRAME_RES = 12,
    CMD_WRITE_IP = 13,            ///< Set ethernet ip to inv.
    CMD_WRITE_CLD_STATE = 14,     ///< Set Zevercloud state to inv.
    CMD_INV_HD_DEBUG = 15,        ///< read the debug information of the inverter
    CMD_INV_FD_DEBUG = 16,        ///< read the debug information of the inverter
    CMD_INV_ONOFF_CTRL = 17,      ///< control inverter power on or off
    CMD_WRITE_FUN_MODE = 18,      ///< control inverter function enable flag
    CMD_WRITE_DRMS_PWR = 19,      ///< Set inverter DRMS power cotrol
    CMD_WRITE_FREQ_CURE = 20,     ///< Set inverter over freq cure
    CMD_WRITE_VOLT_CURE = 21,     ///< Set inverter over freq cure
    CMD_INV_LOAD_SPEED = 22,      ///< Set inverter over freq load speed
    CMD_TRANSPARENT_CLD = 23,     ///<transparent zevercloud data and return the result
    CMD_TRANSPARENT_DBG_CLD = 24, ///<transparent zevercloud data and return the result
    CMD_READ_FREQ_CURE = 25,      ///< Get inverter over freq cure
    CMD_MD_READ_INV_INFO = 26,    //modbus protocol read the inverter information
    CMD_MD_METER_DATA = 27,
    CMD_MD_READ_INV_TIME = 28,  //read time from inverter
    CMD_MD_WRITE_INV_TIME = 29, //write time to inverter
    CMD_SET_INV_BAUDRATE = 30,
} Inv_cmd;