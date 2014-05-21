/****************************************************************************
 * Copyright(c), 2001-2060, ******************************* 版权所有
 ****************************************************************************
 * 文件名称             : cloud.c
 * 版本                 : 0.0.1
 * 作者                 : 许龙杰
 * 创建日期             : 2014年5月9日
 * 描述                 : 
 ****************************************************************************/
#include "common_define.h"
#include "upgrade.h"

#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <ctype.h>
#include "openssl/crypto.h"
#include "openssl/ssl.h"
#include "openssl/err.h"
#include "openssl/rand.h"


#include "json/json.h"


/*
 * 函数名      : CloudTombDestroy
 * 功能        : 云在线状态销毁，从系统中删除共享内存信息，
 * 参数        : s32Handle[in] (int32_t类型): CloudInit返回的句柄
 * 返回值      : 无
 * 作者        : 许龙杰
 */
void CloudTombDestroy(int32_t s32Handle)
{
	StCloudHandle *pHandle;
	if (s32Handle == 0)
	{
		return;
	}
	pHandle = (StCloudHandle *)s32Handle;

	if (pHandle->pCloud != NULL)
	{
		shmdt(pHandle->pCloud);
	}
	ReleaseAShmId(pHandle->s32SHMHandle);
	LockClose(pHandle->s32LockHandle);
	free(pHandle);
}

/*
 * 函数名      : CloudInit
 * 功能        : 初始化云在线状态共享内存链表，并返回句柄，必须与CloudDestroy成对使用，
 * 				 不一定要与CloudTombDestroy成对
 * 参数        : pErr[out] (int32_t *): 在指针不为NULL的情况下返回错误码
 * 返回值      : int32_t 型数据, 0失败, 否则成功
 * 作者        : 许龙杰
 */
int32_t CloudInit(int32_t *pErr)
{
	StCloudHandle *pHandle = 0;
	int32_t s32Err = 0;
	const char *pName = CLOUD_NAME;

	pHandle = calloc(1, sizeof(StCloudHandle));
	if (pHandle == NULL)
	{
		return MY_ERR(_Err_Mem);
	}
	pHandle->s32LockHandle = -1;
	pHandle->s32SHMHandle = -1;

	pHandle->s32LockHandle = LockOpen(pName);
	if (pHandle->s32LockHandle < 0)
	{
		s32Err = pHandle->s32LockHandle;
		goto err1;
	}

	pHandle->s32SHMHandle = GetTheShmId(pName, sizeof(StCloud));
	if (pHandle->s32SHMHandle < 0)
	{
		s32Err = pHandle->s32SHMHandle;
		goto err1;
	}

	pHandle->pCloud = (StCloud *)shmat(pHandle->s32SHMHandle, NULL, 0);
	if (pHandle->pCloud == (StCloud *)(-1))
	{
		pHandle->pCloud = NULL;
		s32Err = MY_ERR(_Err_SYS + errno);
		goto err1;
	}

	s32Err = LockLock(pHandle->s32LockHandle);
	if (s32Err < 0)
	{
		goto err1;
	}
	if ((pHandle->pCloud->stMemCheck.s32FlagHead == 0x01234567) &&
			(pHandle->pCloud->stMemCheck.s32FlagTail == 0x89ABCDEF) &&
			(pHandle->pCloud->stMemCheck.s32CheckSum ==
					CheckSum((int32_t *)pHandle->pCloud,
							offsetof(StMemCheck, s32CheckSum) / sizeof(int32_t))))
	{
		PRINT("the shared memory has been initialized\n");
		goto ok;
	}

	PRINT("I will initialize the shared memory\n");
	memset(pHandle->pCloud, 0, sizeof(StCloud));
	pHandle->pCloud->stMemCheck.s32FlagHead = 0x01234567;
	pHandle->pCloud->stMemCheck.s32FlagTail = 0x89ABCDEF;

	{
		int32_t i;
		for (i = 0; i < 8; i++)
		{
			pHandle->pCloud->stMemCheck.s16RandArray[i] = rand();
		}
	}
	pHandle->pCloud->stMemCheck.s32CheckSum =
		CheckSum((int32_t *)pHandle->pCloud, offsetof(StMemCheck, s32CheckSum) / sizeof(int32_t));

ok:
	LockUnlock(pHandle->s32LockHandle);
	return (int32_t)pHandle;
err1:
	CloudTombDestroy((int32_t)pHandle);
	if (pErr != NULL)
	{
		*pErr = s32Err;
	}
	return 0;
}

/*
 * 函数名      : CloudGetStat
 * 功能        : 通过句柄得到云状态
 * 参数        : s32Handle [in] (int32_t): CloudInit返回的句柄
 * 			   : pStat[out] (StCloudStat *): 保存云状态，详见定义
 * 返回值      : int32_t 型数据, 0成功, 否则表示错误码
 * 作者        : 许龙杰
 */
int32_t CloudGetStat(int32_t s32Handle, StCloudStat *pStat)
{
	StCloudHandle *pHandle = (StCloudHandle *)s32Handle;
	int32_t s32Err = 0;
	if ((pStat == NULL) || (pHandle == NULL))
	{
		return MY_ERR(_Err_InvalidParam);
	}
	s32Err = LockLock(pHandle->s32LockHandle);
	if (s32Err == 0)
	{
		*pStat = pHandle->pCloud->stStat;
		LockUnlock(pHandle->s32LockHandle);
	}
	return s32Err;
}

/*
 * 函数名      : CloudSetStat
 * 功能        : 通过句柄设置云状态
 * 参数        : s32Handle [in] (int32_t): CloudInit返回的句柄
 * 			   : pStat[out] (StCloudStat *): 保存云状态，详见定义
 * 返回值      : int32_t 型数据, 0失败, 否则成功
 * 作者        : 许龙杰
 */
int32_t CloudSetStat(int s32Handle, StCloudStat *pStat)
{
	StCloudHandle *pHandle = (StCloudHandle *)s32Handle;
	int32_t s32Err = 0;
	if ((pStat == NULL) || (pHandle == NULL))
	{
		return MY_ERR(_Err_InvalidParam);
	}
	s32Err = LockLock(pHandle->s32LockHandle);
	if (s32Err == 0)
	{
		pHandle->pCloud->stStat = *pStat;
		LockUnlock(pHandle->s32LockHandle);
	}
	return s32Err;
}

/*
 * 函数名      : CloudDestroy
 * 功能        : 释放该进程空间中的句柄占用的资源与CloudInit成对使用
 * 参数        : s32Handle[in] (int32_t): CloudInit的返回值
 * 返回值      : 无
 * 作者        : 许龙杰
 */
void CloudDestroy(int32_t s32Handle)
{
	StCloudHandle *pHandle = (StCloudHandle *)s32Handle;

	if (NULL == pHandle)
	{
		return;
	}

	shmdt(pHandle->pCloud);
	free(pHandle);
}


void SingalPIPE(int32_t s32Singal)
{
}

/*
 * 函数名      : SSL_Init
 * 功能        : OPENSSL库初始化，仅需在进程启动开始调用一次
 * 参数        : 无
 * 返回        : 无
 * 作者        : 许龙杰
 */
void SSL_Init(void)
{
	SSL_library_init();
	SSL_load_error_strings();
	signal(SIGPIPE, SingalPIPE);/* do not anything for this signal */
}

/*
 * 函数名      : SSL_Destory
 * 功能        : OPENSSL库初销毁，仅需在进程结束时调用一次
 * 参数        : 无
 * 返回        : 无
 * 作者        : 许龙杰
 */
void SSL_Destory(void)
{
	ERR_free_strings();
}

/*
 * 函数名      : CloudSendAndGetReturn
 * 功能        : 向云发送数据并得到返回，保存在pMap中
 * 参数        : pStat [in] (StCloudStat * 类型): 云状态，详见定义
 *             : pSendInfo [in] (StSendInfo 类型): 发送信息，详见定义
 *             : pMap [out] (StMMap *): 保存返回内容，详见定义，使用过之后必须使用CloudMapRelease销毁
 * 返回        : 正确返回0, 错误返回错误码
 * 作者        : 许龙杰
 */
int32_t CloudSendAndGetReturn(StCloudStat *pStat, StSendInfo *pSendInfo, StMMap *pMap)
{
	if ((pStat == NULL) || (pSendInfo == NULL) || (pMap == NULL))
	{
		return MY_ERR(_Err_InvalidParam);
	}

	if (pStat->emStat != _Cloud_IsOnline)
	{
		return MY_ERR(_Err_Cloud_IsNotOnline);
	}

	{
	int32_t s32Err = 0;
	int32_t s32Socket = -1;
	struct sockaddr_in stServerAddr, stClientAddr;
	struct hostent *pHost;

	SSL *pSSL;
	SSL_CTX *pSSLCTX;
	int32_t s32SSL = -1;

	if ((pHost = gethostbyname(pStat->c8ServerURL)) == NULL)
	{
		/* get the IP address of the server */
		PRINT("gethostbyname error, %s\n", strerror(errno));
		return MY_ERR(_Err_SYS + errno);
	}

	bzero(&stServerAddr, sizeof(stServerAddr));
	stServerAddr.sin_family = AF_INET;
	stServerAddr.sin_addr = *((struct in_addr *) pHost->h_addr);
	stServerAddr.sin_port = htons(443);	/* https */

	bzero(&stClientAddr, sizeof(stClientAddr));
	stClientAddr.sin_family = AF_INET;    /* Internet protocol */
	if (inet_aton(pStat->c8ClientIPV4, &stClientAddr.sin_addr) == 0)
	{
		PRINT("client IP address error!\n");
		return MY_ERR(_Err_SYS + errno);
	}
	stClientAddr.sin_port = htons(0);    /* the system will allocate a port number for it */

	if ((s32Socket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		PRINT("socket error:%s\n", strerror(errno));
		return MY_ERR(_Err_SYS + errno);
	}

	if (bind(s32Socket, (struct sockaddr*) &stClientAddr, sizeof(struct sockaddr_in)) != 0)
	{
		PRINT("bind error:%s\n", strerror(errno));
		s32Err = MY_ERR(_Err_SYS + errno);
		goto err;
	}

	{
		int32_t s32Mode;

		/*First we make the socket nonblocking*/
		s32Mode = fcntl(s32Socket,F_GETFL,0);
		s32Mode |= O_NONBLOCK;
		if (fcntl(s32Socket, F_SETFL, s32Mode) == -1)
		{
			PRINT("bind error:%s\n", strerror(errno));
			s32Err = MY_ERR(_Err_SYS + errno);
			goto err;
		}
	}

	PRINT("begin connect\n");
	if (connect(s32Socket, (struct sockaddr *) (&stServerAddr), sizeof(struct sockaddr_in)) == -1)
	{
		if (errno != EINPROGRESS)
		{
			PRINT("connect error:%s\n", strerror(errno));
			s32Err = MY_ERR(_Err_SYS + errno);
			goto err;
		}
		else
		{
			struct timeval stTimeVal;
			fd_set stFdSet;
			stTimeVal.tv_sec = 10;
			stTimeVal.tv_usec = 0;
			FD_ZERO(&stFdSet);
			FD_SET(s32Socket, &stFdSet);
			s32Err = select(s32Socket + 1, NULL, &stFdSet, NULL, &stTimeVal);
			if (s32Err > 0)
			{
				socklen_t u32Len = sizeof(int32_t);
				/* for firewall */
				getsockopt(s32Socket, SOL_SOCKET, SO_ERROR, &s32Err, &u32Len);
				if (s32Err != 0)
				{
					PRINT("connect error:%s\n", strerror(errno));
					s32Err = MY_ERR(_Err_SYS + errno);
					goto err;
				}
				PRINT("connect ok via select\n");
			}
			else
			{
				PRINT("connect error:%s\n", strerror(errno));
				s32Err = MY_ERR(_Err_SYS + errno);
				goto err;
			}
		}
	}
	PRINT("connect ok\n");
	{
		int32_t s32Mode;

		/*First we make the socket nonblocking*/
		s32Mode = fcntl(s32Socket,F_GETFL,0);
		s32Mode &= (~O_NONBLOCK);
		if (fcntl(s32Socket, F_SETFL, s32Mode) == -1)
		{
			PRINT("bind error:%s\n", strerror(errno));
			s32Err = MY_ERR(_Err_SYS + errno);
			goto err;
		}
	}


	pSSLCTX = SSL_CTX_new(SSLv23_client_method());
	if (pSSLCTX == NULL)
	{
		PRINT("SSL_CTX_new error:%s\n", strerror(errno));
		s32Err = MY_ERR(_Err_SYS + errno);
		goto err;
	}

	pSSL = SSL_new(pSSLCTX);
	if (pSSL == NULL)
	{
		PRINT("SSL_new error:%s\n", strerror(errno));
		s32Err = MY_ERR(_Err_SYS + errno);
		goto err1;
	}

	/* link the socket to the SSL */
	s32SSL = SSL_set_fd(pSSL, s32Socket);
	if (s32SSL == 0)
	{
		s32Err = SSL_get_error(pSSL, s32SSL);
		PRINT("SSL_set_fd error %d\n", s32Err);
		s32Err = MY_ERR(_Err_SSL + s32Err);
		goto err2;
	}

	RAND_poll();
	while (RAND_status() == 0)
	{
		time_t s32Time = time(NULL);
		RAND_seed(&s32Time, sizeof(time_t));
	}

	s32SSL = SSL_connect(pSSL);
	if (s32SSL != 1)
	{
		s32Err = SSL_get_error(pSSL, s32SSL);
		PRINT("SSL_set_fd error %d\n", s32Err);
		s32Err = MY_ERR(_Err_SSL + s32Err);
		goto err2;
	}

	{
	char c8Request[1024];
	c8Request[0] = 0;
	sprintf(c8Request,
			"%s /%s HTTP/1.1\r\n"
			"Accept: */*\r\n"
			/* "Accept-Language: zh-cn\r\n"
			"User-Agent: Mozilla/4.0 (compatible; MSIE 5.01; Windows NT 5.0)\r\n" maybe not useful */
			"Host: %s:%d\r\n"
			"Content-Length: %d\r\n"
			"Connection: Close\r\n\r\n",
			pSendInfo->boIsGet ? "GET" : "POST",
			pSendInfo->pFile, pStat->c8ServerURL, 443,
			pSendInfo->s32BodySize);
	PRINT("%s", c8Request);
	/* send */
	{
	int32_t s32Totalsend = 0;
	int32_t s32Size = strlen(c8Request);
	while (s32Totalsend < s32Size)
	{
		int32_t s32Send = SSL_write(pSSL, c8Request + s32Totalsend, s32Size - s32Totalsend);
		if (s32Send <= 0)
		{
			s32Err = SSL_get_error(pSSL, s32Send);
			PRINT("SSL_set_fd error %d\n", s32Err);
			s32Err = MY_ERR(_Err_SSL + s32Err);
			goto err3;
		}
		s32Totalsend += s32Send;
	}
	if (pSendInfo->pSendBody != NULL)
	{
		const char *pBody = pSendInfo->pSendBody;
		s32Size = pSendInfo->s32BodySize;
		if(s32Size < 0)
		{
			s32Size = strlen(pSendInfo->pSendBody);
		}
		s32Totalsend = 0;
		while (s32Totalsend < s32Size)
		{
			int32_t s32Send = SSL_write(pSSL, pBody + s32Totalsend, s32Size - s32Totalsend);
			if (s32Send <= 0)
			{
				s32Err = SSL_get_error(pSSL, s32Send);
				PRINT("SSL_set_fd error %d\n", s32Err);
				s32Err = MY_ERR(_Err_SSL + s32Err);
				goto err3;
			}
			s32Totalsend += s32Send;
		}
	}
	}
	/* receive */
#if 0
	FILE *pFile = fopen("test.html", "wb+");
	if (pFile == NULL)
	{
		PRINT("fopen error:%s\n", strerror(errno));
		s32Err = MY_ERR(_Err_SYS + errno);
		goto err3;
	}
	while(1)
	{
		int32_t s32RecvTmp = 0;
		char c8Buf[PAGE_SIZE];
		s32RecvTmp = SSL_read(pSSL, c8Buf, PAGE_SIZE);
		if (s32RecvTmp <= 0)
		{
			break;
		}
		fwrite(c8Buf, s32RecvTmp, 1, pFile);
		fflush(pFile);
	}
	fclose(pFile);
#else
	{
	FILE *pFile = tmpfile();
	int32_t s32Fd = fileno(pFile);
	int32_t s32RecvTotal = 0;
	void *pMapAddr = NULL;
	if (pFile == NULL)
	{
		PRINT("fopen error:%s\n", strerror(errno));
		s32Err = MY_ERR(_Err_SYS + errno);
		goto err3;
	}
	while(1)
	{
		int32_t s32RecvTmp = 0, s32Recv = 0;
		bool boIsFinished = false;
		s32Err = ftruncate(s32Fd, s32RecvTotal + PAGE_SIZE);
		if (s32Err != 0)
		{
			s32Err = MY_ERR(_Err_SYS + errno);
			goto err4;
		}
		if (pMapAddr != NULL)
		{
			munmap(pMapAddr, PAGE_SIZE);
		}
		pMapAddr = mmap(0, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, s32Fd, s32RecvTotal);
		if (pMapAddr == NULL)
		{
			s32Err = MY_ERR(_Err_SYS + errno);
			goto err4;
		}
		PRINT("mmap ok, begin to read\n");
		while (s32Recv < PAGE_SIZE)
		{
			s32RecvTmp = SSL_read(pSSL, (void *)((uint32_t)pMapAddr + s32Recv), PAGE_SIZE - s32Recv);
			if (s32RecvTmp <= 0)
			{
				boIsFinished = true;
				break;
			}
			s32Recv += s32RecvTmp;
		}
		s32RecvTotal += s32Recv;
		PRINT("read ok, total receive is %d\n", s32RecvTotal);
		if (boIsFinished)
		{
			break;
		}
	}
	pMap->pFile = pFile;
	if (pMapAddr != NULL)
	{
		munmap(pMapAddr, PAGE_SIZE);
	}
	pMapAddr = mmap(0, s32RecvTotal, PROT_READ | PROT_WRITE, MAP_SHARED, s32Fd, 0);
	if (pMapAddr == NULL)
	{
		s32Err = MY_ERR(_Err_SYS + errno);
		goto err4;
	}
	pMap->pMap = pMapAddr;
	pMap->u32MapSize = s32RecvTotal;
	goto err3;
err4:
	if (pMapAddr != NULL)
	{
		munmap(pMapAddr, PAGE_SIZE);
	}

	fclose(pFile);
	}
#endif
	}

err3:
	SSL_shutdown(pSSL);
err2:
	SSL_free(pSSL);
err1:
	SSL_CTX_free(pSSLCTX);
err:
	close(s32Socket);
	return s32Err;
	}

	return 0;
}

/*
 * 函数名      : CloudMapRelease
 * 功能        : 销毁CloudSendAndGetReturn的返回内容
 * 参数        : pMap [out] (StMMap *): 返回的内容，详见定义
 * 返回        : 无
 * 作者        : 许龙杰
 */
void CloudMapRelease(StMMap *pMap)
{
	if (pMap == NULL)
	{
		return;
	}
	if (pMap->pFile != NULL)
	{
		fclose(pMap->pFile);
	}
	if (pMap->pMap != NULL)
	{
		munmap(pMap->pMap, pMap->u32MapSize);
	}
}

/* get http return code HTTP/1.1 200 OK ect. */
int32_t GetHttpReturnCode(StMMap *pMap)
{
	if (pMap != NULL)
	{
		if (pMap->pMap != NULL)
		{
			int32_t s32ReturnCode = -1;
			const char *pTmp = strchr(pMap->pMap, ' ');
			if (pTmp != NULL)
			{
				sscanf(pTmp + 1, "%d", &s32ReturnCode);
			}
			return s32ReturnCode;
		}
	}
	return -1;
}

/* get the content of the return  */
const char *GetHttpBody(StMMap *pMap)
{
	if (pMap != NULL)
	{
		if (pMap->pMap != NULL)
		{
			const char *pTmp = strstr(pMap->pMap, "\r\n\r\n");
			if (pTmp == NULL)
			{
				return NULL;
			}
			return strchr(pTmp, '{');
		}
	}
	return NULL;
}

/* get the json of the return */
json_object *GetHttpJsonBody(StMMap *pMap, int32_t *pErr)
{
	json_object *pReturn = NULL;
	int32_t s32Err;
	const char *pBody;
	/* analysis the return text */
	s32Err = GetHttpReturnCode(pMap);
	if (s32Err != 200)
	{
		s32Err = MY_ERR(_Err_HTTP + s32Err);
		goto end;
	}
	pBody = GetHttpBody(pMap);
	if (pBody == NULL)
	{
		s32Err =MY_ERR(_Err_Cloud_Body);
		goto end;
	}
	pReturn = json_tokener_parse(pBody);
	if (pReturn == NULL)
	{
		s32Err = MY_ERR(_Err_Cloud_JSON);
		goto end;
	}
end:
	*pErr = s32Err;
	return pReturn;
}

/* resolve the command of 0xFA from the cloud, the c8R will save the cloud R */
int32_t ResolveCmd_0xFA(json_object *pJsonObj, char c8R[RAND_NUM_CNT])
{
	{
	json_object *pCmd = NULL;
	if (!json_object_object_get_ex(pJsonObj, "Command", &pCmd))
	{
		return MY_ERR(_Err_Cloud_CMD);
	}
	if (strcmp("0xFA", json_object_to_json_string(pCmd)) != 0)
	{
		return  MY_ERR(_Err_Cloud_CMD);
	}
	}
	/* command data */
	{

	json_object *pData = NULL;
	json_object *pArray = NULL;
	int32_t s32ArrayCnt, i;
	if (!json_object_object_get_ex(pJsonObj, "Data", &pData))
	{
		return MY_ERR(_Err_Cloud_Data);
	}
	if (!json_object_object_get_ex(pJsonObj, "RandNum", &pArray))
	{
		return MY_ERR(_Err_Cloud_Data);
	}
	if (json_object_get_type(pArray) != json_type_array)
	{
		return MY_ERR(_Err_Cloud_Data);
	}
	s32ArrayCnt = json_object_array_length(pArray);
	if (s32ArrayCnt != RAND_NUM_CNT)
	{
		return MY_ERR(_Err_Cloud_Data);
	}
	for (i = 0; i < s32ArrayCnt; i++)
	{
		json_object *pObj = json_object_array_get_idx(pArray, i);
		const char *pRTmp = json_object_to_json_string(pObj);
		sscanf(pRTmp, "0x%02hhx", c8R + i);
	}
	}

	return 0;
}

/* resolve the command of 0xFB from the cloud, the c8ClientR is code R from client */
int32_t ResolveCmd_0xFB(json_object *pJsonObj, char c8ClientR[RAND_NUM_CNT])
{
	{
	json_object *pCmd = NULL;
	if (!json_object_object_get_ex(pJsonObj, "Command", &pCmd))
	{
		return MY_ERR(_Err_Cloud_CMD);
	}
	if (strcmp("0xFB", json_object_to_json_string(pCmd)) != 0)
	{
		if (strcmp("0xFD", json_object_to_json_string(pCmd)) != 0)
		{
			return  MY_ERR(_Err_Cloud_CMD);
		}
		else
		{
			return MY_ERR(_Err_Cloud_Authentication);
		}
	}
	}
	/* command data */
	{

	json_object *pData = NULL;
	json_object *pArray = NULL;
	int32_t s32ArrayCnt, i;
	char c8R[RAND_NUM_CNT];
	if (!json_object_object_get_ex(pJsonObj, "Data", &pData))
	{
		return MY_ERR(_Err_Cloud_Data);
	}
	if (!json_object_object_get_ex(pJsonObj, "CodingRandNum1", &pArray))
	{
		return MY_ERR(_Err_Cloud_Data);
	}
	if (json_object_get_type(pArray) != json_type_array)
	{
		return MY_ERR(_Err_Cloud_Data);
	}
	s32ArrayCnt = json_object_array_length(pArray);
	if (s32ArrayCnt != RAND_NUM_CNT)
	{
		return MY_ERR(_Err_Cloud_Data);
	}
	for (i = 0; i < s32ArrayCnt; i++)
	{
		json_object *pObj = json_object_array_get_idx(pArray, i);
		const char *pRTmp = json_object_to_json_string(pObj);
		sscanf(pRTmp, "0x%02hhx", c8R + i);
		if (c8R[i] != c8ClientR[i])	/* compare the R from the cloud with the R from client */
		{
			return MY_ERR(_Err_Authentication);
		}
	}
	}

	return 0;
}

/* buildup the command of 0xFA, 0xFC, 0xFD for client */
json_object *BuildupCmd_0xFA_C_D(char c8Cmd, int32_t *pErr)
{
	json_object * pJsonObj = json_object_new_object();
	if (pJsonObj == NULL)
	{
		*pErr = MY_ERR(_Err_JSON);
		return NULL;
	}

	json_object_object_add(pJsonObj, "sc", json_object_new_string(AUTHENTICATION_SC));
	json_object_object_add(pJsonObj, "sv", json_object_new_string(AUTHENTICATION_SV));
	{
	char c8Tmp[8];
	sprintf(c8Tmp, "0x%02hhX", c8Cmd);
	json_object_object_add(pJsonObj, "Command", json_object_new_string(c8Tmp));
	}

	return pJsonObj;
}

/* buildup the command of 0xFA, 0xFC, 0xFD for client */
json_object *BuildupCmd_0xFB(char c8CloudR[RAND_NUM_CNT], char c8ClientR[RAND_NUM_CNT],
		const char c8ID[PRODUCT_ID_CNT], const char c8Key[XXTEA_KEY_CNT_CHAR],
		int32_t *pErr)
{
	int32_t s32Err = 0;
	int32_t i;
	json_object *pJsonObj = json_object_new_object();
	if (pJsonObj == NULL)
	{
		PRINT("json_object_new_object");
		s32Err = MY_ERR(_Err_JSON);
		goto err;
	}

	json_object_object_add(pJsonObj, "sc", json_object_new_string(AUTHENTICATION_SC));
	json_object_object_add(pJsonObj, "sv", json_object_new_string(AUTHENTICATION_SV));
	json_object_object_add(pJsonObj, "Command", json_object_new_string("0xFB"));

	btea((int32_t *)c8CloudR, RAND_NUM_CNT / sizeof(int32_t), (int32_t *)c8Key);
	{
	json_object *pCloudRArr, *pClientRArr, *pData;
	pCloudRArr = json_object_new_array();
	if (pCloudRArr == NULL)
	{
		PRINT("json_object_new_object");
		s32Err = MY_ERR(_Err_JSON);
		goto err;
	}
	pClientRArr = json_object_new_array();
	if (pClientRArr == NULL)
	{
		PRINT("json_object_new_object");
		json_object_put(pCloudRArr);
		s32Err = MY_ERR(_Err_JSON);
		goto err;
	}
	pData = json_object_new_object();
	if (pData == NULL)
	{
		PRINT("json_object_new_object");
		json_object_put(pCloudRArr);
		json_object_put(pClientRArr);
		s32Err = MY_ERR(_Err_JSON);
		goto err;
	}

	srand(time(NULL));
	for (i = 0; i < RAND_NUM_CNT; i++)
	{
		char c8Tmp[8];
		c8ClientR[i] = rand();
		sprintf(c8Tmp, "0x%02hhx", c8ClientR[i]);
		json_object_array_add(pClientRArr, json_object_new_string(c8Tmp));
		sprintf(c8Tmp, "0x%02hhx", c8CloudR[i]);
		json_object_array_add(pCloudRArr, json_object_new_string(c8Tmp));
	}
	json_object_object_add(pData, "CodingRandNum", pCloudRArr);
	json_object_object_add(pData, "ProductID", json_object_new_string_len(c8ID, PRODUCT_ID_CNT));
	json_object_object_add(pData, "RandNum1", pClientRArr);

	json_object_object_add(pJsonObj, "Data", pData);
	}
	*pErr = s32Err;
	return pJsonObj;
err:
	if (pJsonObj != NULL)
	{
		json_object_put(pJsonObj);
	}
	*pErr = s32Err;
	return NULL;
}

#define TEST_CLOUD		0

typedef struct _tagStIDPS
{
	char c8ProtocolString[16];					/* com.jiuan.HG3 */
	char c8AccessoryName[16];					/* HG3 */
	char c8AccessoryFirmware[3];				/* 1.0.0 */
	char c8AccessoryHardware[3];				/* 1.0.1 */
	char c8AccessoryManufacturer[16];			/* HG3 */
	char c8AccessoryModelNumber[16];			/* HG3 12312 */
	char c8AccessorySerialNumber[16];			/* 128912387145897 */

}StIDPS;
/*
 * 函数名      : CloudSendIDPS
 * 功能        : 发送IDPS
 * 参数        : pStat [in] (StCloudStat * 类型): 云状态，详见定义
 *             : pIDPS [in] (StIDPS * 类型): 产品IDPS
 * 返回        : 正确返回0, 错误返回错误码
 * 作者        : 许龙杰
 */
int32_t CloudSendIDPS(StCloudStat *pStat, const char c8ID[PRODUCT_ID_CNT], const char c8Key[XXTEA_KEY_CNT_CHAR])
{
	return 0;
}


/*
 * 函数名      : CloudAuthentication
 * 功能        : 云认证
 * 参数        : pStat [in] (StCloudStat * 类型): 云状态，详见定义
 *             : c8ID [in] (const char * 类型): 产品ID
 *             : c8Key [in] (const char *): 产品KEY
 * 返回        : 正确返回0, 错误返回错误码
 * 作者        : 许龙杰
 */
int32_t CloudAuthentication(StCloudStat *pStat, const char c8ID[PRODUCT_ID_CNT], const char c8Key[XXTEA_KEY_CNT_CHAR])
{
	int32_t s32Err = 0;
	json_object *pJsonObj = NULL;

	char c8CloudR[RAND_NUM_CNT];
	char c8ClientR[RAND_NUM_CNT];
	char c8Char[PAGE_SIZE];

	if ((pStat == NULL) || (c8ID == NULL) || (c8Key == NULL))
	{
		return MY_ERR(_Err_InvalidParam);
	}

	pJsonObj = BuildupCmd_0xFA_C_D(0xFA, &s32Err);
	if (pJsonObj == NULL)
	{
		return MY_ERR(_Err_JSON);
	}

	sprintf(c8Char, "Body:%s", json_object_to_json_string(pJsonObj));
	json_object_put(pJsonObj);
	pJsonObj = NULL;
	PRINT("%s\n", c8Char);
#if TEST_CLOUD
	{
	StSendInfo stSendInfo = {false, AUTHENTICATION_ADDR, NULL, -1};
	StMMap stMap = {NULL};
	json_object *pReturnValue;
	stSendInfo.pSendBody = c8Char;
	s32Err = CloudSendAndGetReturn(pStat, &stSendInfo, &stMap);
	if (s32Err != 0)
	{
		return s32Err;
	}
	/* analysis the return text */
	pJsonObj = GetHttpJsonBody(&stMap, &s32Err);
	CloudMapRelease(&stMap);
	if (pJsonObj == NULL)
	{
		return s32Err;
	}
	if (!json_object_object_get_ex(pJsonObj, "ReturnValue", &pReturnValue))
	{
		s32Err = MY_ERR(_Err_Cloud_JSON);
		goto end;
	}
	s32Err = ResolveCmd_0xFA(pReturnValue, c8CloudR);
	if (s32Err != 0)
	{
		goto end;
	}
	json_object_put(pJsonObj);
	pJsonObj = NULL;

	}
#endif


	pJsonObj = BuildupCmd_0xFB(c8CloudR, c8ClientR, c8ID, c8Key, &s32Err);
	if (pJsonObj == NULL)
	{
		PRINT("json_object_new_object");
		return s32Err;
	}
	sprintf(c8Char, "Body:%s", json_object_to_json_string(pJsonObj));
	json_object_put(pJsonObj);
	pJsonObj = NULL;
	PRINT("%s\n", c8Char);

#if TEST_CLOUD
	{
	StSendInfo stSendInfo = {false, AUTHENTICATION_ADDR, NULL, -1};
	StMMap stMap = {NULL};
	json_object *pReturnValue;
	stSendInfo.pSendBody = c8Char;
	s32Err = CloudSendAndGetReturn(pStat, &stSendInfo, &stMap);
	if (s32Err != 0)
	{
		return s32Err;
	}
	/* analysis the return text */
	pJsonObj = GetHttpJsonBody(&stMap, &s32Err);
	CloudMapRelease(&stMap);
	if (pJsonObj == NULL)
	{
		return s32Err;
	}
	if (!json_object_object_get_ex(pJsonObj, "ReturnValue", &pReturnValue))
	{
		s32Err = MY_ERR(_Err_Cloud_JSON);
		goto end;
	}
	btea((int32_t *)c8ClientR, RAND_NUM_CNT / sizeof(int32_t), (int32_t *)c8Key);
	s32Err = ResolveCmd_0xFB(pReturnValue, c8ClientR);
	json_object_put(pJsonObj);
	pJsonObj = NULL;
	}
#endif
	if (s32Err == 0)
	{
		pJsonObj = BuildupCmd_0xFA_C_D(0xFC, &s32Err);
		if (pJsonObj == NULL)
		{
			return MY_ERR(_Err_JSON);
		}

		sprintf(c8Char, "Body:%s", json_object_to_json_string(pJsonObj));
		json_object_put(pJsonObj);
		pJsonObj = NULL;
		PRINT("%s\n", c8Char);
#if TEST_CLOUD
		{
		StSendInfo stSendInfo = {false, AUTHENTICATION_ADDR, NULL, -1};
		StMMap stMap = {NULL};

		stSendInfo.pSendBody = c8Char;
		s32Err = CloudSendAndGetReturn(pStat, &stSendInfo, &stMap);
		if (s32Err != 0)
		{
			return s32Err;
		}
		/* analysis the return text */
		pJsonObj = GetHttpJsonBody(&stMap, &s32Err);
		CloudMapRelease(&stMap);
		if (pJsonObj == NULL)
		{
			return s32Err;
		}
		json_object_put(pJsonObj);
		pJsonObj = NULL;
		}
#endif

	}
	else if (s32Err == MY_ERR(_Err_Authentication))
	{
		pJsonObj = BuildupCmd_0xFA_C_D(0xFD, &s32Err);
		if (pJsonObj == NULL)
		{
			return MY_ERR(_Err_JSON);
		}

		sprintf(c8Char, "Body:%s", json_object_to_json_string(pJsonObj));
		json_object_put(pJsonObj);
		pJsonObj = NULL;
		PRINT("%s\n", c8Char);
#if TEST_CLOUD
		{
		StSendInfo stSendInfo = {false, AUTHENTICATION_ADDR, NULL, -1};
		StMMap stMap = {NULL};

		stSendInfo.pSendBody = c8Char;
		s32Err = CloudSendAndGetReturn(pStat, &stSendInfo, &stMap);
		if (s32Err != 0)
		{
			return s32Err;
		}
		/* analysis the return text */
		pJsonObj = GetHttpJsonBody(&stMap, &s32Err);
		CloudMapRelease(&stMap);
		if (pJsonObj == NULL)
		{
			return s32Err;
		}
		json_object_put(pJsonObj);
		pJsonObj = NULL;
		}
#endif
	}
#if TEST_CLOUD
end:
#endif
	if (pJsonObj != NULL)
	{
		json_object_put(pJsonObj);
	}
	return s32Err;
}


/* buildup the command of 0xF5 for client */
json_object *BuildupCmd_0xF5(const char c8ID[PRODUCT_ID_CNT], int32_t *pErr)
{
	json_object *pJsonObj = json_object_new_object();
	if (pJsonObj == NULL)
	{
		PRINT("json_object_new_object");
		*pErr = MY_ERR(_Err_JSON);
		return NULL;
	}

	json_object_object_add(pJsonObj, "sc", json_object_new_string(KEEPALIVE_SC));
	json_object_object_add(pJsonObj, "sv", json_object_new_string(KEEPALIVE_SV));
	json_object_object_add(pJsonObj, "Command", json_object_new_string("0xF5"));
	json_object_object_add(pJsonObj, "ProductID", json_object_new_string_len(c8ID, PRODUCT_ID_CNT));

	return pJsonObj;
}

/*
 * 函数名      : CloudKeepAlive
 * 功能        : 云保活
 * 参数        : pStat [in] (StCloudStat * 类型): 云状态，详见定义
 *             : c8ID [in] (const char * 类型): 产品ID
 * 返回        : 正确返回0, 错误返回错误码
 * 作者        : 许龙杰
 */
int32_t CloudKeepAlive(StCloudStat *pStat, const char c8ID[PRODUCT_ID_CNT])
{
	json_object *pJsonObj;
	char c8Char[PAGE_SIZE];
	int32_t s32Err = 0;

	if ((pStat == NULL) || (c8ID == NULL))
	{
		return MY_ERR(_Err_InvalidParam);
	}

	pJsonObj = BuildupCmd_0xF5(c8ID, &s32Err);
	if (pJsonObj == NULL)
	{
		return MY_ERR(_Err_JSON);
	}

	sprintf(c8Char, "Body:%s", json_object_to_json_string(pJsonObj));
	json_object_put(pJsonObj);
	pJsonObj = NULL;
	PRINT("%s\n", c8Char);
#if TEST_CLOUD
	{
	StSendInfo stSendInfo = {false, AUTHENTICATION_ADDR, NULL, -1};
	StMMap stMap = {NULL};
	json_object *pReturnValue;
	stSendInfo.pSendBody = c8Char;
	s32Err = CloudSendAndGetReturn(pStat, &stSendInfo, &stMap);
	CloudMapRelease(&stMap);
	}
#endif
	return s32Err;
}


