#include "asw_protocol_ftp.h"
#include "esp_timer.h"
#include "data_process.h"
#include "mqueue.h"

#define ABS(x) (((x) > 0) ? (x) : -(x))
#define CONSTRAIN(amt, low, high) ((amt) < (low) ? (low) : ((amt) > (high) ? (high) : (amt)))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

#define SAFE_FREE(p)   \
	do                 \
	{                  \
		if (NULL != p) \
		{              \
			free(p);   \
			p = NULL;  \
		}              \
	} while (0)

static const char *TAG = "FTP";
static char *MOUNT_POINT = "/inv";
static uint8_t s_kaco_ftp_start = 0;
static int s_ftp_res_value = -1;
// // ftp-client 20220905
// EventGroupHandle_t gHandle_FtpEventGroup;
// #define ASW_FTP_SEND_DATA_BIT_0 (1UL << 0UL)
// #define ASW_FTP_SEND_RES_BIT_1 (1UL << 1UL)

xSemaphoreHandle gSemFtpClientSync; // = xSemaphoreCreateBinary();

// char *data_path = "/inv";
// char *base_path = "/home";

#define CONFIG_FTP_SERVER "ftp.sspcdn-a.net" //"192.168.32.106" //"demo.wftpserver.com"
#define CONFIG_FTP_PORT 21
// #define CONFIG_FTP_USER "demo"
// #define CONFIG_FTP_PSWD "demo"

#define CONFIG_FTP_USER "DEVSN686291" // "DEVSN658230" //"DEVSN686595"  //"tian"
#define CONFIG_FTP_PSWD "dffbc99dfc"  //"f5eb76b9e9"  //"c88fa9bb62"  //"1234"
#define CONFIG_NUM_CLIENT 5

char srcFileName[70];
char dstFileName[64];
char srcFileDtTm[40] = {0};
// char srcFileName[64];

FtpClient *pftpClient = NULL;
static NetBuf_t *ftpClientNetBuf = NULL;
static esp_timer_handle_t oneshot_timer = NULL;

int s_ftp_filelist_num = 0;
char str_ftp_filelist[CONFIG_NUM_CLIENT][60] = {0};

//------------------------------//
void asw_ftp_close();

//------------------------------//
static void closeftp_timer_callback(void *arg)
{
	int64_t time_since_boot = esp_timer_get_time();
	ESP_LOGI(TAG, "One-shot timer called, time since boot: %lld us", time_since_boot);
	asw_ftp_close();

	ESP_LOGW(TAG, "ftp clinet is closed!!!");
}

void asw_ftp_init()
{
	// Open FTP server
	ESP_LOGI(TAG, "ftp server:%s,port:%d ", CONFIG_FTP_SERVER, CONFIG_FTP_PORT); // CONFIG_FTP_SERVER
	ESP_LOGI(TAG, "ftp user  :%s", CONFIG_FTP_USER);

	pftpClient = getFtpClient();

	/*创建定时器*/

	const esp_timer_create_args_t oneshot_timer_args = {
		.callback = &closeftp_timer_callback,
		.name = "one-shot"}; //单次定时器配置
	ESP_ERROR_CHECK(esp_timer_create(&oneshot_timer_args, &oneshot_timer));

	s_ftp_filelist_num = 0;
	s_kaco_ftp_start = 0;
}

int get_ftp_filelist_size()
{
	printf("\n=== get ftp file list size: %d\n", s_ftp_filelist_num);
	return s_ftp_filelist_num;
}

void asw_ftp_create_file(char *psn, char *content)
{
	printf("\n content: %s\n", psn);

	memset(srcFileName, 0, sizeof(srcFileName));
	memset(srcFileDtTm, 0, sizeof(srcFileDtTm));
	/**************************************/

	struct tm curr_time;
	time_t t = time(NULL);

	localtime_r(&t, &curr_time);
	// curr_time.tm_year += 1900;

	sprintf(srcFileDtTm, "%04d_%02d%02d_%02d%02d", curr_time.tm_year + 1900, curr_time.tm_mon + 1, curr_time.tm_mday, curr_time.tm_hour, curr_time.tm_min);

	/*************************************/

	sprintf(str_ftp_filelist[s_ftp_filelist_num], "%s_%s.csv", psn, srcFileDtTm);

	sprintf(srcFileName, "%s/%s", MOUNT_POINT, str_ftp_filelist[s_ftp_filelist_num]);

	s_ftp_filelist_num++;

	FILE *f = fopen(srcFileName, "w");
	if (f == NULL)
	{
		ESP_LOGE(TAG, "Failed to open file for writing %s", srcFileName);
		return;
	}
	int res = fprintf(f, "%s\r\n", content);

	if (res > 0)
	{

		if (s_ftp_filelist_num > CONFIG_NUM_CLIENT)
		{
			s_ftp_filelist_num = 0;
			ESP_LOGW(TAG, " WARNIGN  filelist is full num > 5");
		}

		ESP_LOGI(TAG, "SUCCESS Wrote the text on %s-%d \n", srcFileName, s_ftp_filelist_num);
	}
	else
	{
		ESP_LOGE(TAG, "ERRO  Wrote the text on %s", srcFileName);
	}

	fclose(f);
}

int asw_ftp_connect()
{
	int res = -1;

	if (s_kaco_ftp_start)
	{
		ESP_LOGI("--Kaco ftp --", "kaco ftp client is connncted.");
		return 0;
	}

	//----------------- crate time to close -----------//
	/* Start the timers */

	//----------------------------------------------------//

	if (pftpClient == NULL)
		pftpClient = getFtpClient();

	int connect = pftpClient->ftpClientConnect(CONFIG_FTP_SERVER, CONFIG_FTP_PORT, &ftpClientNetBuf); // 21
	ESP_LOGI(TAG, "connect=%d", connect);
	if (connect == 0)
	{
		ESP_LOGE(TAG, "FTP server connect fail");
		res = -1;
		goto END;
	}
	// CONFIG_FTP_USER
	// Login FTP server
	int login = pftpClient->ftpClientLogin(CONFIG_FTP_USER, CONFIG_FTP_PSWD, ftpClientNetBuf); // CONFIG_FTP_PASSWORD
	ESP_LOGI(TAG, "login=%d", login);
	if (login == 0)
	{
		ESP_LOGE(TAG, "FTP server login fail");
		res = -2;
		goto END;
	}
	else
		return 0;

END:

	if (ftpClientNetBuf != NULL)
		pftpClient->ftpClientQuit(ftpClientNetBuf);
	pftpClient = NULL;
	ftpClientNetBuf = NULL;

	return res;
}

void asw_ftp_close()
{
	if (ftpClientNetBuf != NULL)
		pftpClient->ftpClientQuit(ftpClientNetBuf);
	pftpClient = NULL;
	ftpClientNetBuf = NULL;
	s_ftp_filelist_num = 0;

	s_kaco_ftp_start = 0;
}

/////////////////////////////////////////

/////////////////////////////////////////

int asw_ftp_upload(char *psn, char *content, int len)
{
	// int res = -1;
	uint8_t index = 0;

	kaco_ftp_data_t mFtpSendData = {0};

	EventBits_t xFtpResEventGroupValue;

	memset(srcFileName, 0, sizeof(srcFileName));
	memset(srcFileDtTm, 0, sizeof(srcFileDtTm));

	// if (esp_timer_is_active(oneshot_timer))
	// 	esp_timer_stop(oneshot_timer); // 停止定时器

	////---------------------------------------------////

	struct tm curr_time;
	time_t t = time(NULL);

	localtime_r(&t, &curr_time);
	// curr_time.tm_year += 1900;

	sprintf(srcFileDtTm, "%04d_%02d%02d_%02d%02d", curr_time.tm_year + 1900, curr_time.tm_mon + 1, curr_time.tm_mday, curr_time.tm_hour, curr_time.tm_min);

	/*************************************/

	sprintf(srcFileName, "%s_%s.csv", psn, srcFileDtTm);

	///------------------------------------------------///

	ESP_LOGW(TAG, "file name :%s", srcFileName);

	// assert(pftpClient != NULL);

	///////////////////////////////////////////

	if (to_ftp_task_mq != NULL) // kaco-lanstick 20220802 +
	{
		mFtpSendData.msgType = 1; //实时数据

		memcpy(mFtpSendData.rmFileName, srcFileName, sizeof(srcFileName));
		assert(content != NULL);
		mFtpSendData.payload = calloc(len + 20, sizeof(char)); //[  mark]注意free的调用
		if (mFtpSendData.payload == NULL)
		{
			ESP_LOGE("--kaco ftp upload Erro-", "calloc payload is error ,no space .");
			return -1;
		}
		mFtpSendData.len = len;
		memcpy(mFtpSendData.payload, content, len + 1);

		xQueueSend(to_ftp_task_mq, (void *)&mFtpSendData, 50 / portTICK_PERIOD_MS);

		ESP_LOGW(TAG, "----->>>>>to_ftp_task_mq send ok<<<<<-------\n");
	}

	////////////////////////////////////////

	// res = pftpClient->asw_ftpClientPut(content, srcFileName, FTP_CLIENT_TEXT, ftpClientNetBuf, len);

	// ESP_LOGI(TAG, "ftpClientPut res:%d", res);

	// ESP_ERROR_CHECK(esp_timer_start_once(oneshot_timer, 60 * 1000 * 1000)); // 60s后关闭ftp
	printf("\n==================== COMDDDDDDDDDDDDDDDDDD ===============\n");
	// /* 获取事件位 */
	// xFtpResEventGroupValue = xEventGroupWaitBits(gHandle_FtpEventGroup,		 /* 事件组的句柄 */
	// 											 ASW_FTP_SEND_RES_BIT_1,	 /* 待测试的事件位 */
	// 											 pdTRUE,					 /* 满足添加时清除上面的事件位 */
	// 											 pdTRUE,					 /* 任意事件位被设置就会退出阻塞态 */
	// 											 6000 / portTICK_PERIOD_MS); /* 没有超时 */

	/* 根据相应的事件位输出提示信息 */
	// if ((xFtpResEventGroupValue & ASW_FTP_SEND_RES_BIT_1) != 0 && s_ftp_res_value)

	UBaseType_t res = xSemaphoreTake(gSemFtpClientSync, 8000 / portTICK_PERIOD_MS);

	printf("\n==================== DDDDDDDDDD99999999999 ===============\n");
	SAFE_FREE(mFtpSendData.payload);

	if (res == pdPASS && s_ftp_res_value)
	{
		s_ftp_filelist_num++;

		printf("\n==== ftp upload <<<  current ftp file list size:%d\n", s_ftp_filelist_num);
		return 0;
	}
	else
	{
		ESP_LOGW("--Ftp Data Res ERRO--", " ftp connnect is time out  res:%d ", s_ftp_res_value);
		return -1;
	}
}

/**
 * @brief  上传文件名fileName的csv文件到ftp 服务器
 *
 * @param fileName
 * @return int 0:成功；非0：失败
 */
int asw_ftp_upload_file_byName(char *localFileName, char *ftpFileName)
{

	assert(ftpFileName != NULL);
	assert(localFileName != NULL);

	kaco_ftp_data_t mFtpSendFile = {0};
	EventBits_t xFtpResEventGroupValue;

	// if (esp_timer_is_active(oneshot_timer))
	// 	esp_timer_stop(oneshot_timer); // 停止定时器

	/*从本地的filaName 上传到 ftp服务器，ftp库函数调用*/
	// assert(pftpClient != NULL);

	if (access(localFileName, 0) != 0)
	{
		ESP_LOGE("-- Kaco Read localFile History FileList ERRO -- ", "[%s] hisotry filelist not exited. ", localFileName);
		return -1;
	}

	///----------------------------------////

	if (to_ftp_task_mq != NULL) // kaco-lanstick 20220802 +
	{
		mFtpSendFile.msgType = 2; //历史数据

		memcpy(mFtpSendFile.lcFileName, localFileName, sizeof(mFtpSendFile.lcFileName));
		memcpy(mFtpSendFile.rmFileName, ftpFileName, sizeof(mFtpSendFile.rmFileName));

		xQueueSend(to_ftp_task_mq, (void *)&mFtpSendFile, 50 / portTICK_PERIOD_MS);

		ESP_LOGW(TAG, "----->>>>>to_ftp_task_mq send file  ok<<<<<-------\n");
	}

	printf("\n=========================commmmm---AAAA=====================\n");

	/* 获取事件位 */
	// xFtpResEventGroupValue = xEventGroupWaitBits(gHandle_FtpEventGroup,		 /* 事件组的句柄 */
	// 											 ASW_FTP_SEND_RES_BIT_1,	 /* 待测试的事件位 */
	// 											 pdTRUE,					 /* 满足添加时清除上面的事件位 */
	// 											 pdTRUE,					 /* 任意事件位被设置就会退出阻塞态 */
	// 											 6000 / portTICK_PERIOD_MS); /* 没有超时 */

	// ESP_LOGI(TAG, "ftpClientPut %s ---> %s,res:%d", localFileName, ftpFileName, s_ftp_res_value);

	// xSemaphoreTake(server_ready, portMAX_DELAY);
	UBaseType_t res = xSemaphoreTake(gSemFtpClientSync, 95000 / portTICK_PERIOD_MS);
	printf("\n=========================commmmm--BBBB  =====================\n");

	/* 根据相应的事件位输出提示信息 */
	// if ((xFtpResEventGroupValue & ASW_FTP_SEND_RES_BIT_1) != 0 && s_ftp_res_value)
	if (res == pdPASS && s_ftp_res_value)
	{

		ESP_LOGI("-- Kaco Upload History File Succss", " Succeed upload file:%s to ftp server", ftpFileName);

		return 0;
	}
	else
	{

		ESP_LOGW("--Ftp File Res ERRO--", " ftp connnect is time out ");
		return -1;
	}

	// ///--------------------------------////

	// // int res = pftpClient->ftpClientPut(localFileName, ftpFileName, FTP_CLIENT_TEXT, ftpClientNetBuf);

	// ESP_ERROR_CHECK(esp_timer_start_once(oneshot_timer, 60 * 1000 * 1000)); // 60s后关闭ftp

	// if (res != 1)
	// {
	// 	ESP_LOGE(TAG, "ftp client put is ERRO: %d", res);
	// 	return -1;
	// }
	// else
	// {
	// 	return 0;
	// 	/*返回结果 处理*/
	// }
}
//////////////////////////////////////////
void asw_ftp_client_task(void *pvParameters)
{

	// gHandle_FtpEventGroup = xEventGroupCreate();

	gSemFtpClientSync = xSemaphoreCreateBinary();
	EventBits_t xFtpEventGroupValue;
	kaco_ftp_data_t mstrFtpSendData = {0};
	asw_ftp_init();
	char *mpPayload = NULL;

	while (1)
	{

		if (to_ftp_task_mq != NULL && xQueueReceive(to_ftp_task_mq, &mstrFtpSendData, 50 / portTICK_PERIOD_MS) == pdPASS)
		{
			if (esp_timer_is_active(oneshot_timer))
				esp_timer_stop(oneshot_timer); // 停止定时器

			asw_ftp_connect();

			/* 判断是发送文件还是发送实时数据 */
			if (mstrFtpSendData.msgType == 1) //实时数据
			{

				mpPayload = calloc(mstrFtpSendData.len + 6, sizeof(char)); //[  mark]注意free的调用
				if (mpPayload == NULL)
				{
					ESP_LOGE("--kaco  ftp send Erro-", "calloc payload is error ,no space .");
					s_ftp_res_value = -1;
				}
				else
				{
					memcpy(mpPayload, mstrFtpSendData.payload, mstrFtpSendData.len);
					s_ftp_res_value = pftpClient->asw_ftpClientPut(mpPayload, mstrFtpSendData.rmFileName,
																   FTP_CLIENT_TEXT, ftpClientNetBuf, mstrFtpSendData.len);
				}
			}
			else // if(mstrFtpSendData.msgType == 2) 历史数据
			{
				s_ftp_res_value = pftpClient->ftpClientPut(mstrFtpSendData.lcFileName, mstrFtpSendData.rmFileName, FTP_CLIENT_TEXT, ftpClientNetBuf);
			}
			// 处理标志位：1.发送或连接状态值-全局变量 2,处理标志位

			if (mpPayload)
			{
				free(mpPayload);
				mpPayload = NULL;
			}

			xSemaphoreGive(gSemFtpClientSync);

			// xEventGroupSetBits(gHandXle_FtpEventGroup, ASW_FTP_SEND_RES_BIT_1);
			printf("\n================ jjjjjjjjjjjjjjjjjjj ======\n");
			assert(oneshot_timer != NULL);
			ESP_ERROR_CHECK(esp_timer_start_once(oneshot_timer, 60 * 1000 * 1000)); // 90s后回调
			ESP_LOGI(TAG, "Started timers, time since boot: %lld us", esp_timer_get_time());
		}

		vTaskDelay(100 / portTICK_PERIOD_MS); // usleep(50 * 1000);
	}
}

////////////////////////////////////////

void asw_ftp_client_start()
{
	int xReturned = xTaskCreate(
		asw_ftp_client_task,   /* Function that implements the task. */
		"asw_ftp_client_task", /* Text name for the task. */
		8 * 1024,			   /* Stack size in bytes, 15+10 KB */
		(void *)1,			   /* Parameter passed into the task. */
		5,					   /* Priority at which the task is created. */
		NULL);
	if (xReturned != pdPASS)
	{
		ESP_LOGE(TAG, "create asw ftp client task failed.\n");
	}
}
