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
	if (pHandle->pDBHandle != NULL)
	{
		DBClose(pHandle->pDBHandle);
	}
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
		goto shm_ok;
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

shm_ok:
	LockUnlock(pHandle->s32LockHandle);
	pHandle->pDBHandle = DBOpen(DB_NAME,  O_RDWR | O_CREAT | O_TRUNC, MODE_RW);
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
 * 函数名      : CloudGetStat
 * 功能        : 通过句柄得到云状态
 * 参数        : s32Handle [in] (int32_t): CloudInit返回的句柄
 * 返回值      : bool 标识云是否在线
 * 作者        : 许龙杰
 */
bool CloudIsOnline(int32_t s32Handle)
{
	StCloudHandle *pHandle = (StCloudHandle *)s32Handle;

	if (pHandle == NULL)
	{
		return false;
	}

	if (pHandle->pCloud->stStat.emStat == _Cloud_IsOnline)
	{
		return true;
	}

	return false;
}

/*
 * 函数名      : CloudSetStat
 * 功能        : 通过句柄设置云状态
 * 参数        : s32Handle [in] (int32_t): CloudInit返回的句柄
 * 			   : pStat[out] (StCloudStat *): 保存云状态，详见定义
 * 返回值      : int32_t 型数据, 0成功, 否则表示错误码
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

const char *c_pRegionPrefix[_Region_Reserved] =
{
	"Region_HTTPS",
	"Region_UDP",
};

/*
 * 函数名      : CloudSaveDomainViaRegion
 * 功能        : 保存区域对应的域名[:端口]
 * 参数        : s32Handle [in] (int32_t): CloudInit返回的句柄
 * 			   : emType[in] (EmRegionType): 要查找域名的类别
 * 			   : pRegion[in] (const char *): 域名的关键值, 区域编号, 例如: "CN"、"EN"、"US"......,
 * 			     必须是以\0为结尾的字符串
 * 			   : pDomain[in] (const char *): 域名的内容[:端口], 例如:"wehealth.com[:443]"("[]"代表可有可无，
 * 			     没有的话为默认443), 必须是以\0为结尾的字符串
 * 返回值      : int32_t 型数据, 0成功, 否则表示错误码
 * 作者        : 许龙杰
 */
int32_t CloudSaveDomainViaRegion(int s32Handle,  EmRegionType emType,
		const char *pRegion, const char *pDomain)
{

	StCloudHandle *pHandle = (StCloudHandle *)s32Handle;
	if ((pHandle == NULL) || (emType >= _Region_Reserved) ||
			(pRegion == NULL) || (pDomain == NULL))
	{
		return MY_ERR(_Err_InvalidParam);
	}
	{
	char c8Key[64];
	snprintf(c8Key, 64, "%s_%s", c_pRegionPrefix[emType], pRegion);
	if (DBStore(pHandle->pDBHandle, c8Key, pDomain, DB_STORE) == -1)
	{
		return MY_ERR(_Err_Cloud_Save_Domain);
	}
	}

	return 0;
}

/*
 * 函数名      : CloudGetDomainFromRegion
 * 功能        : 得到区域对应的域名[:端口]
 * 参数        : s32Handle [in] (int32_t): CloudInit返回的句柄
 * 			   : emType[in] (EmRegionType): 要查找域名的类别
 * 			   : pRegion[in] (const char *): 域名的关键值, 区域编号, 例如: "CN"、"EN"、"US"......,
 * 			     必须是以\0为结尾的字符串
 * 			   : pDomain[out] (char *): 成功保存域名的内容[:端口], 例如:"wehealth.com[:443]"("[]"代表可有可无，
 * 			     没有的话为默认443), 必须是以\0为结尾的字符串
 * 			     u32Size[out] (uint32_t): 指明pDomain指向字符串的大小
 * 返回值      : int32_t 型数据, 0成功, 否则表示错误码
 * 作者        : 许龙杰
 */
int32_t CloudGetDomainFromRegion(int32_t s32Handle, EmRegionType emType,
		const char *pRegion, char *pDomain, uint32_t u32Size)
{

	StCloudHandle *pHandle = (StCloudHandle *)s32Handle;
	if ((pHandle == NULL) || (emType >= _Region_Reserved) ||
			(pRegion == NULL) || (pDomain == NULL))
	{
		return MY_ERR(_Err_InvalidParam);
	}
	{
	char *pBuf;
	char c8Key[64];
	snprintf(c8Key, 64, "%s_%s", c_pRegionPrefix[emType], pRegion);
	pBuf = DBFetch(pHandle->pDBHandle, c8Key);
	if (pBuf == NULL)
	{
		PRINT("cannot find domain form database via key: %s\n", pRegion);
		return MY_ERR(_Err_Cloud_Get_Domain);
	}
	else
	{
		strncpy(pDomain, pBuf, u32Size);
	}
	}
	return 0;
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
 * 参数        : pStat [in] (StCloudDomain * 类型): 云状态，详见定义
 *             : pSendInfo [in] (StSendInfo 类型): 发送信息，详见定义
 *             : pMap [out] (StMMap *): 保存返回内容，详见定义，使用过之后必须使用CloudMapRelease销毁
 * 返回        : 正确返回0, 错误返回错误码
 * 作者        : 许龙杰
 */
int32_t CloudSendAndGetReturn(StCloudDomain *pStat, StSendInfo *pSendInfo, StMMap *pMap)
{
	if ((pStat == NULL) || (pSendInfo == NULL) || (pMap == NULL))
	{
		return MY_ERR(_Err_InvalidParam);
	}

	if (pStat->stStat.emStat != _Cloud_IsOnline)
	{
		return MY_ERR(_Err_Cloud_IsNotOnline);
	}

	{
	int32_t s32Err = 0;
	int32_t s32Socket = -1;
	struct sockaddr_in stServerAddr, stClientAddr;

	SSL *pSSL;
	SSL_CTX *pSSLCTX;
	int32_t s32SSL = -1;
	char c8Domain[64];
	c8Domain[0] = 0;
	if (pSendInfo->pSecondDomain != NULL)
	{
		sprintf(c8Domain, "%s.", pSendInfo->pSecondDomain);
	}
	strcat(c8Domain, pStat->c8Domain);
	PRINT("server domain: %s\n", c8Domain);

	bzero(&stServerAddr, sizeof(stServerAddr));
	stServerAddr.sin_family = AF_INET;
	s32Err = GetHostIPV4Addr(c8Domain, NULL, &(stServerAddr.sin_addr));
	if (s32Err != 0)
	{
		/* get the IP address of the server */
		PRINT("GetHostIPV4Addr error\n");
		return s32Err;
	}
	stServerAddr.sin_port = htons(pStat->s32Port);	/* https */

	bzero(&stClientAddr, sizeof(stClientAddr));
	stClientAddr.sin_family = AF_INET;    /* Internet protocol */
	if (inet_aton(pStat->stStat.c8ClientIPV4, &stClientAddr.sin_addr) == 0)
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
		ERR_print_errors_fp(stderr);
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
		ERR_print_errors_fp(stderr);
		PRINT("SSL_connect error %d\n", s32Err);
		s32Err = MY_ERR(_Err_SSL + s32Err);
		goto err2;
	}

	{
	char c8Request[1024];
	c8Request[0] = 0;
	if (pSendInfo->s32BodySize == -1)
	{
		pSendInfo->s32BodySize = strlen(pSendInfo->pSendBody);
	}
	sprintf(c8Request,
			"%s /%s HTTP/1.1\r\n"
			"Accept: */*\r\n"
			/* "Accept-Language: zh-cn\r\n"
			"User-Agent: Mozilla/4.0 (compatible; MSIE 5.01; Windows NT 5.0)\r\n" maybe not useful */
			"Host: %s:%d\r\n"
			"Content-Length: %d\r\n"
			"Connection: Close\r\n\r\n",
			pSendInfo->boIsGet ? "GET" : "POST",
			pSendInfo->pFile, c8Domain, pStat->s32Port,
			pSendInfo->s32BodySize);
	PRINT("\n%s\n", c8Request);
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
 * 函数名      : CloudSendAndGetReturnNoSSL
 * 功能        : 向云发送数据并得到返回(不通过SSL)，保存在pMap中
 * 参数        : pStat [in] (StCloudDomain * 类型): 云状态，详见定义
 *             : pSendInfo [in] (StSendInfo 类型): 发送信息，详见定义
 *             : pMap [out] (StMMap *): 保存返回内容，详见定义，使用过之后必须使用CloudMapRelease销毁
 * 返回        : 正确返回0, 错误返回错误码
 * 作者        : 许龙杰
 */
int32_t CloudSendAndGetReturnNoSSL(StCloudDomain *pStat, StSendInfo *pSendInfo, StMMap *pMap)
{
	if ((pStat == NULL) || (pSendInfo == NULL) || (pMap == NULL))
	{
		return MY_ERR(_Err_InvalidParam);
	}

	if (pStat->stStat.emStat != _Cloud_IsOnline)
	{
		return MY_ERR(_Err_Cloud_IsNotOnline);
	}

	{
	int32_t s32Err = 0;
	int32_t s32Socket = -1;
	struct sockaddr_in stServerAddr, stClientAddr;

	char c8Domain[64];
	c8Domain[0] = 0;
	if (pSendInfo->pSecondDomain != NULL)
	{
		sprintf(c8Domain, "%s.", pSendInfo->pSecondDomain);
	}
	strcat(c8Domain, pStat->c8Domain);
	PRINT("server domain: %s\n", c8Domain);

	bzero(&stServerAddr, sizeof(stServerAddr));
	stServerAddr.sin_family = AF_INET;
	s32Err = GetHostIPV4Addr(c8Domain, NULL, &(stServerAddr.sin_addr));
	if (s32Err != 0)
	{
		/* get the IP address of the server */
		PRINT("GetHostIPV4Addr error\n");
		return s32Err;
	}
	stServerAddr.sin_port = htons(pStat->s32Port);	/* https */

	bzero(&stClientAddr, sizeof(stClientAddr));
	stClientAddr.sin_family = AF_INET;    /* Internet protocol */
	if (inet_aton(pStat->stStat.c8ClientIPV4, &stClientAddr.sin_addr) == 0)
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
	/* 设置套接字选项,接收和发送超时时间 */
	{
	struct timeval stTimeout = {30};
	if(setsockopt(s32Socket, SOL_SOCKET, SO_RCVTIMEO, &stTimeout, sizeof(struct timeval)) < 0)
	{
		close(s32Socket);
		return MY_ERR(_Err_SYS + errno);
	}

	if(setsockopt(s32Socket, SOL_SOCKET, SO_SNDTIMEO, &stTimeout, sizeof(struct timeval)) < 0)
	{
		close(s32Socket);
		return MY_ERR(_Err_SYS + errno);
	}
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
			pSendInfo->pFile, c8Domain, pStat->s32Port,
			pSendInfo->s32BodySize);
	PRINT("%s", c8Request);
	/* send */
	{
	int32_t s32Totalsend = 0;
	int32_t s32Size = strlen(c8Request);
	while (s32Totalsend < s32Size)
	{
		int32_t s32Send = send(s32Socket, c8Request + s32Totalsend, s32Size - s32Totalsend, MSG_NOSIGNAL);
		if (s32Send < 0)
		{
			PRINT("send error %s\n", strerror(errno));
			s32Err = MY_ERR(_Err_SYS + errno);
			goto err;
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
			int32_t s32Send = send(s32Socket, pBody + s32Totalsend, s32Size - s32Totalsend, MSG_NOSIGNAL);
			if (s32Send <= 0)
			{
				PRINT("send error %s\n", strerror(errno));
				s32Err = MY_ERR(_Err_SYS + errno);
				goto err;
			}
			s32Totalsend += s32Send;
		}
	}
	}
	/* receive */
	{
	FILE *pFile = tmpfile();
	int32_t s32Fd = fileno(pFile);
	int32_t s32RecvTotal = 0;
	void *pMapAddr = NULL;
	if (pFile == NULL)
	{
		PRINT("fopen error:%s\n", strerror(errno));
		s32Err = MY_ERR(_Err_SYS + errno);
		goto err;
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
			s32RecvTmp = recv(s32Socket, (void *)((uint32_t)pMapAddr + s32Recv), PAGE_SIZE - s32Recv, 0);
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
	goto err;
err4:
	if (pMapAddr != NULL)
	{
		munmap(pMapAddr, PAGE_SIZE);
	}

	fclose(pFile);
	}
	}

err:
	close(s32Socket);
	return s32Err;
	}
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


/*
 * 函数名      : GetRegionOfGateway
 * 功能        : 得到Gateway的区域信息
 * 参数        : pRegion [out] (char * 类型): 成功保存区域字符串
 *             : uint32_t [in] (uint32_t 类型): pRegion指向字符串的长度
 * 返回        : 正确返回0, 错误返回错误码
 * 作者        : 许龙杰
 */
int32_t GetRegionOfGateway(char *pRegion, uint32_t u32Size)
{
	if ((pRegion == NULL) || (u32Size == 0))
	{
		return MY_ERR(_Err_InvalidParam);
	}
	else
	{
		json_object *pInfo, *pRegionObj;
		pInfo = json_object_from_file(GATEWAY_INFO_FILE);
		if (pInfo == NULL)
		{
			PRINT("json_object_from_file error\n");
			return MY_ERR(_Err_JSON);
		}
		pRegionObj = json_object_object_get(pInfo, "Ptr");
		if (pRegionObj == NULL)
		{
			json_object_put(pInfo);
			return MY_ERR(_Err_JSON);
		}

		pRegionObj = json_object_object_get(pRegionObj, "Region");
		if (pRegionObj == NULL)
		{
			json_object_put(pInfo);
			return MY_ERR(_Err_JSON);
		}
		strncpy(pRegion, json_object_get_string(pRegionObj), u32Size);
		json_object_put(pInfo);
		return 0;
	}
}

/*
 * 函数名      : GetDomainPortFromString
 * 功能        : 得到Gateway的区域信息
 * 参数        : pStr [in] (const char * 类型): 要解析的符串(以'\0'结尾), 例如"www.jiuan.com[:443]"
 * 			   : pDomain [out] (char * 类型): 成功保存域名
 *             : uint32_t [in] (uint32_t 类型): pDomain指向字符串的长度
 *             : pPort [out] (uint32_t * 类型): 成功保存段口号
 * 返回        : 正确返回0, 错误返回错误码
 * 作者        : 许龙杰
 */
int32_t GetDomainPortFromString(const char *pStr, char *pDomain, uint32_t u32Size, int32_t *pPort)
{
	if (pStr == NULL || pDomain == NULL || pPort == NULL || u32Size == 0)
	{
		return MY_ERR(_Err_InvalidParam);
	}
	else
	{
		uint32_t i = 0;
		bool boFindPort = false;
		while (pStr[i] != 0)
		{
			if (pStr[i] == ':')
			{
				*pPort = atoi(pStr + i + 1);
				pDomain[i] = 0;
				boFindPort = true;
				break;
			}

			if (i < u32Size)
			{
				pDomain[i] = pStr[i];
			}
			i++;
		}
		pDomain[u32Size - 1] = 0;
		if (!boFindPort)
		{
			*pPort = 443;
		}
	}

	return 0;
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
			PRINT("HTTP return: %d\n", s32ReturnCode);
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
	json_object *pReturn = NULL, *pSon;
	int32_t s32Err;
	const char *pBody;
	PRINT("server return: \n%s\n", (const char *)pMap->pMap);
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
		PRINT("cannot find char \'{\'\n");
		goto end;
	}
	pReturn = json_tokener_parse(pBody);
	if (pReturn == NULL)
	{
		s32Err = MY_ERR(_Err_Cloud_JSON);
		PRINT("json_tokener_parse error\n");
		goto end;
	}
	pSon = json_object_object_get(pReturn, "Result");
	if (pSon == NULL)
	{
		s32Err = MY_ERR(_Err_Cloud_JSON);
		json_object_put(pReturn);
		PRINT("json cannot get key Result\n");
		pReturn = NULL;
		goto end;
	}
	/* result is ok */
	if (json_object_is_type(pSon, json_type_int))
	{
		s32Err = json_object_get_int(pSon);
		if (s32Err != 1)
		{
			pSon = json_object_object_get(pReturn, "ResultMessage");
			PRINT("ResultMessage: %s\n", json_object_to_json_string(pSon));
			s32Err = MY_ERR(_Err_Cloud_Result + s32Err);
			json_object_put(pReturn);
			pReturn = NULL;
			goto end;
		}
	}
	else
	{
		PRINT("key Result's type is not int\n");

		s32Err = MY_ERR(_Err_Cloud_JSON);
		json_object_put(pReturn);
		pReturn = NULL;
		goto end;
	}
	pSon = json_object_object_get(pReturn, "ReturnValue");
	if (pSon == NULL)
	{
		s32Err = MY_ERR(_Err_Cloud_JSON);
		json_object_put(pReturn);
		pReturn = NULL;
		goto end;
	}
	else
	{
		s32Err = 0;
		json_object_get(pSon);
		json_object_put(pReturn);
		return pSon;
	}

end:
	*pErr = s32Err;
	return pReturn;
}
/*
 * resolve the command of 0xF5 from the cloud
 * 1:
 */
int32_t ResolveCmd_0xF5(json_object *pJsonObj)
{
	char c8Buf[16];
	int s32Rslt = 0xFD;
	char c8Tmp = 0;
	sprintf(c8Buf, "%s", json_object_get_string(pJsonObj));
	sscanf(c8Buf, "%02hhX", &c8Tmp);
	s32Rslt = c8Tmp;
	PRINT("F5 return %s\n", c8Buf);
	return (s32Rslt & 0xFF);
}

/* resolve the command of 0xFA from the cloud, the c8R will save the cloud R */
int32_t ResolveCmd_0xFA(json_object *pJsonObj, char c8R[RAND_NUM_CNT])
{
	/* command data */
	{
	int32_t s32Len;
	s32Len = json_object_get_string_len(pJsonObj);
	if (s32Len != (RAND_NUM_CNT * 2))
	{
		PRINT("command FA R NULL\n");
		return MY_ERR(_Err_Cloud_Data);
	}
	}

	{
	int32_t i;
	char c8RServer[36];
	char c8Buf[4] = {0};
	snprintf(c8RServer, 36,"%s", json_object_to_json_string(pJsonObj));
	for (i = 0; i < RAND_NUM_CNT; i++)
	{
		c8Buf[0] = c8RServer[i * 2 + 1];		/* \" */
		c8Buf[1] = c8RServer[i * 2 + 2];
		sscanf(c8Buf, "%02hhX", c8R + i);
	}
	}
	return 0;
}

/* resolve the command of 0xFB from the cloud, the c8ClientR is R1 of client */
int32_t ResolveCmd_0xFB(json_object *pJsonObj, char c8ClientR[RAND_NUM_CNT], const char c8Key[XXTEA_KEY_CNT_CHAR])
{
	/* command data */
	{
	int32_t s32Len;
	s32Len = json_object_get_string_len(pJsonObj);
	if (s32Len != (RAND_NUM_CNT * 2))
	{
		PRINT("command FB RN1's coding NULL\n");
		return MY_ERR(_Err_Cloud_Authentication);
	}
	}

	{
	int32_t i;
	char c8RServer[36];
	char c8Buf[4] = {0};
	snprintf(c8RServer, 36,"%s", json_object_to_json_string(pJsonObj));

	btea((int32_t *)c8ClientR, RAND_NUM_CNT / sizeof(int32_t), (int32_t *)c8Key);

	for (i = 0; i < RAND_NUM_CNT; i++)
	{
		char c8R = 0;
		c8Buf[0] = c8RServer[i * 2 + 1];		/* \" */
		c8Buf[1] = c8RServer[i * 2 + 2];
		sscanf(c8Buf, "%02hhX", &c8R);
		if (c8R != c8ClientR[i])
		{
			PRINT("command FB RN1's coding error\n");
			return MY_ERR(_Err_Cloud_Authentication);
		}
	}
	}

	return 0;
}

json_object *GetIDPS(int32_t *pErr)
{
	json_object *pInfo, *pIDPS;
	pInfo = json_object_from_file(GATEWAY_INFO_FILE);
	if (pInfo == NULL)
	{
		PRINT("json_object_from_file error\n");
		*pErr = MY_ERR(_Err_JSON);
		return NULL;
	}
	pIDPS = json_object_object_get(pInfo, "IDPS");
	if (pIDPS == NULL)
	{
		*pErr = MY_ERR(_Err_JSON);
		json_object_put(pInfo);
		return NULL;
	}
	json_object_get(pIDPS);
	json_object_put(pInfo);
	*pErr = 0;
	return pIDPS;
}
static uint32_t s_u32QueueNum = 0;
inline int32_t AuthCmdAddBaseInfo(json_object *pJsonObj)
{
	int32_t s32Err = 0;
	json_object_object_add(pJsonObj, "Sc", json_object_new_string(AUTHENTICATION_SC));
	json_object_object_add(pJsonObj, "Sv", json_object_new_string(AUTHENTICATION_SV));
	json_object_object_add(pJsonObj, "QueueNum", json_object_new_int(s_u32QueueNum++));
	json_object *pIDPS = GetIDPS(&s32Err);
	if (s32Err == 0)
	{
		json_object_object_add(pJsonObj, "IDPS", pIDPS);
	}
	return s32Err;
}

json_object *BuildupCmd_0xF5_A(char c8Cmd, int32_t *pErr)
{
	json_object * pJsonObj = json_object_new_object();
	if (pJsonObj == NULL)
	{
		*pErr = MY_ERR(_Err_JSON);
		return NULL;
	}

	*pErr = AuthCmdAddBaseInfo(pJsonObj);
	if (*pErr != 0)
	{
		json_object_put(pJsonObj);
		return NULL;
	}

	{
	char c8Tmp[8];
	sprintf(c8Tmp, "%02hhX", c8Cmd);
	json_object_object_add(pJsonObj, "Command", json_object_new_string(c8Tmp));
	}

	return pJsonObj;
}

#if 0
/* buildup the command of 0xFA, 0xFC, 0xFD for client */
json_object *BuildupCmd_0xFA_C_D(char c8Cmd, int32_t *pErr)
{
	json_object * pJsonObj = json_object_new_object();
	if (pJsonObj == NULL)
	{
		*pErr = MY_ERR(_Err_JSON);
		return NULL;
	}

	*pErr = AuthCmdAddBaseInfo(pJsonObj);
	if (*pErr != 0)
	{
		json_object_put(pJsonObj);
		return NULL;
	}

	{
	char c8Tmp[8];
	sprintf(c8Tmp, "0x%02hhX", c8Cmd);
	json_object_object_add(pJsonObj, "Command", json_object_new_string(c8Tmp));
	}

	return pJsonObj;
}
#endif

/* buildup the command of 0xFB for client */
json_object *BuildupCmd_0xFB(char c8CloudR[RAND_NUM_CNT], char c8ClientR[RAND_NUM_CNT],
		const char c8ID[PRODUCT_ID_CNT], const char c8Key[XXTEA_KEY_CNT_CHAR],
		int32_t *pErr)
{
	int32_t s32Err = 0;
	json_object *pJsonObj = json_object_new_object();
	if (pJsonObj == NULL)
	{
		PRINT("json_object_new_object");
		s32Err = MY_ERR(_Err_JSON);
		goto err;
	}

	*pErr = AuthCmdAddBaseInfo(pJsonObj);
	if (*pErr != 0)
	{
		json_object_put(pJsonObj);
		return NULL;
	}

	json_object_object_add(pJsonObj, "Command", json_object_new_string("FB"));

	btea((int32_t *)c8CloudR, RAND_NUM_CNT / sizeof(int32_t), (int32_t *)c8Key);
	{
	char c8CloudBuf[36], c8ClientBuf[36];
	int32_t i;
	srand(time(NULL));
	for (i = 0; i < RAND_NUM_CNT; i++)
	{
		sprintf(c8CloudBuf + (i * 2), "%02hhX", c8CloudR[i]);
		c8ClientR[i] = rand();
		sprintf(c8ClientBuf + (i * 2), "%02hhX", c8ClientR[i]);
	}
	json_object_object_add(pJsonObj, "GatewayID", json_object_new_string_len(c8ID, PRODUCT_ID_CNT));
	json_object_object_add(pJsonObj, "RNCoded", json_object_new_string(c8CloudBuf));
	json_object_object_add(pJsonObj, "RN1", json_object_new_string(c8ClientBuf));
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

#define TEST_CLOUD		1

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
 * 			   : boIsCoordination [in] (bool 类型): 是否通过协调服务器
 *             : c8ID [in] (const char * 类型): 产品ID
 *             : c8Key [in] (const char *): 产品KEY
 * 返回        : 正确返回0, 错误返回错误码
 * 作者        : 许龙杰
 */
int32_t CloudAuthentication(StCloudDomain *pStat, bool boIsCoordination,
		const char c8ID[PRODUCT_ID_CNT], const char c8Key[XXTEA_KEY_CNT_CHAR])
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
	pJsonObj = BuildupCmd_0xF5_A(0xF5, &s32Err);
	if (pJsonObj == NULL)
	{
		return MY_ERR(_Err_JSON);
	}

	sprintf(c8Char, "content=%s", json_object_to_json_string(pJsonObj));
	json_object_put(pJsonObj);
	pJsonObj = NULL;
	PRINT("%s\n", c8Char);

	{
#if TEST_CLOUD
	StSendInfo stSendInfo = {false, NULL, AUTHENTICATION_ADDR, NULL, -1};

	if (!boIsCoordination)
	{
		stSendInfo.pSecondDomain = AUTHENTICATION_SECOND_DOMAIN;
	}

	{
	StMMap stMap = {NULL};
	stSendInfo.pSendBody = c8Char;
	stSendInfo.s32BodySize = -1;
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
	s32Err = ResolveCmd_0xF5(pJsonObj);
	if (s32Err == 0xF0)
	{
		s32Err = 0;
		goto end;
	}
	else if (s32Err == 0xFD)
	{
		s32Err = MY_ERR(_Err_IDPS);
		goto end;
	}
	else if (s32Err != 0xF5)
	{
		s32Err = MY_ERR(_Err_Cloud_Data);
		goto end;
	}
	json_object_put(pJsonObj);
	pJsonObj = NULL;

	}
#endif

	pJsonObj = BuildupCmd_0xF5_A(0xFA, &s32Err);
	if (pJsonObj == NULL)
	{
		return MY_ERR(_Err_JSON);
	}

	sprintf(c8Char, "content=%s", json_object_to_json_string(pJsonObj));
	json_object_put(pJsonObj);
	pJsonObj = NULL;
	PRINT("%s\n", c8Char);
#if TEST_CLOUD
	{
	StMMap stMap = {NULL};

	stSendInfo.pSendBody = c8Char;
	stSendInfo.s32BodySize = -1;
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
	s32Err = ResolveCmd_0xFA(pJsonObj, c8CloudR);
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
	sprintf(c8Char, "content=%s", json_object_to_json_string(pJsonObj));
	json_object_put(pJsonObj);
	pJsonObj = NULL;
	PRINT("%s\n", c8Char);

#if TEST_CLOUD
	{
	StMMap stMap = {NULL};
	stSendInfo.pSendBody = c8Char;
	stSendInfo.s32BodySize = -1;
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

	s32Err = ResolveCmd_0xFB(pJsonObj, c8ClientR, c8Key);
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

inline int32_t GetSelfRegionCmdAddBaseInfo(json_object *pJsonObj)
{
	int32_t s32Err = 0;
	json_object_object_add(pJsonObj, "Sc", json_object_new_string(GET_SELF_REGION_SC));
	json_object_object_add(pJsonObj, "Sv", json_object_new_string(GET_SELF_REGION_SV));
	json_object_object_add(pJsonObj, "QueueNum", json_object_new_int(s_u32QueueNum++));
	json_object *pIDPS = GetIDPS(&s32Err);
	if (s32Err == 0)
	{
		json_object_object_add(pJsonObj, "IDPS", pIDPS);
	}
	return s32Err;
}


json_object *BuildupCmdGetSelfRegion(int32_t *pErr)
{
	json_object * pJsonObj = json_object_new_object();
	if (pJsonObj == NULL)
	{
		*pErr = MY_ERR(_Err_JSON);
		return NULL;
	}

	*pErr = GetSelfRegionCmdAddBaseInfo(pJsonObj);
	if (*pErr != 0)
	{
		json_object_put(pJsonObj);
		return NULL;
	}
	return pJsonObj;
}

int32_t ResolveCmdGetSelfRegion(json_object *pJsonObj, char *pRegion, uint32_t u32Size)
{
	const char *pTmp = json_object_get_string(pJsonObj);
	if (pTmp == NULL)
	{
		return MY_ERR(_Err_Cloud_JSON);
	}
	pRegion[u32Size - 1] = 0;
	strncpy(pRegion, pTmp + 1, u32Size - 1);	/* \" */
	pRegion[strlen(pRegion) - 1] = 0;	/* \" */
	return 0;
}
/*
 * 函数名      : CloudGetSelfRegion
 * 功能        : 通过云得到自身的区域
 * 参数        : pStat [in] (StCloudStat * 类型): 云状态，详见定义
 *             : pRegion [in/out] (char * 类型): 指向输出buffer, 正确时, 保存区域
 *             : u32Size [in] (uint32_t): pRegion buffer 的大小
 * 返回        : 正确返回0, 错误返回错误码
 * 作者        : 许龙杰
 */
int32_t CloudGetSelfRegion(StCloudDomain *pStat, char *pRegion, uint32_t u32Size)
{
	int32_t s32Err = 0;
	json_object *pJsonObj = NULL;
	char c8Char[PAGE_SIZE];

	if ((pStat == NULL) || (pRegion == NULL) || (u32Size == 0))
	{
		return MY_ERR(_Err_InvalidParam);
	}
	pJsonObj = BuildupCmdGetSelfRegion(&s32Err);
	if (pJsonObj == NULL)
	{
		return MY_ERR(_Err_JSON);
	}

	sprintf(c8Char, "content=%s", json_object_to_json_string(pJsonObj));
	json_object_put(pJsonObj);
	pJsonObj = NULL;
	PRINT("%s\n", c8Char);
#if TEST_CLOUD
	{
	StMMap stMap = {NULL};
	StSendInfo stSendInfo = {false, NULL, AUTHENTICATION_ADDR, NULL, -1};

	stSendInfo.pSendBody = c8Char;
	stSendInfo.s32BodySize = -1;
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
	s32Err = ResolveCmdGetSelfRegion(pJsonObj, pRegion, u32Size);
	json_object_put(pJsonObj);
	pJsonObj = NULL;

	}
#endif
	return s32Err;
}


inline int32_t GetRegionMappingCmdAddBaseInfo(json_object *pJsonObj)
{
	int32_t s32Err = 0;
	json_object_object_add(pJsonObj, "Sc", json_object_new_string(GET_REGION_MAPPING_SC));
	json_object_object_add(pJsonObj, "Sv", json_object_new_string(GET_REGION_MAPPING_SV));
	json_object_object_add(pJsonObj, "QueueNum", json_object_new_int(s_u32QueueNum++));
	json_object *pIDPS = GetIDPS(&s32Err);
	if (s32Err == 0)
	{
		json_object_object_add(pJsonObj, "IDPS", pIDPS);
	}
	return s32Err;
}


json_object *BuildupCmdGetRegionMapping(int32_t *pErr)
{
	json_object * pJsonObj = json_object_new_object();
	if (pJsonObj == NULL)
	{
		*pErr = MY_ERR(_Err_JSON);
		return NULL;
	}

	*pErr = GetRegionMappingCmdAddBaseInfo(pJsonObj);
	if (*pErr != 0)
	{
		json_object_put(pJsonObj);
		return NULL;
	}
	return pJsonObj;
}

int32_t ResolveCmdGetRegionMapping(json_object *pJsonObj, StRegionMapping **p2Mapping, uint32_t *pCnt)
{
	int32_t s32Err = 0;
	uint32_t i;
	uint32_t u32Cnt = 0;
	StRegionMapping *pMap = NULL;
	const char *pKey[3] =
	{
		"Region",
		"CloudUrlVar",
		"HBUrl",
	};
	const uint8_t u8DestSize[3] =
	{
		16 - 1,
		64 - 1,
		64 - 1,
	};
	if (!json_object_is_type(pJsonObj, json_type_array))
	{
		return MY_ERR(_Err_Cloud_JSON);
	}

	u32Cnt = json_object_array_length(pJsonObj);
	if (u32Cnt == 0)
	{
		return MY_ERR(_Err_Cloud_JSON);
	}

	pMap = malloc(u32Cnt * sizeof(StRegionMapping));
	if (pMap == NULL)
	{
		return MY_ERR(_Err_Mem);
	}
	*p2Mapping = pMap;

	for (i = 0; i < u32Cnt; i++)
	{
		json_object *pObj = json_object_array_get_idx(pJsonObj, i);
		char *pDest[3] = {pMap->c8Region, pMap->c8Cloud, pMap->c8Heartbeat};
		int32_t j;
		for (j = 0; j < 3; j++)
		{
			json_object *pValue = json_object_object_get(pObj, pKey[j]);
			const char *pTmp = json_object_get_string(pValue);
			if (pTmp == NULL)
			{
				s32Err = MY_ERR(_Err_Cloud_JSON);
				break;
			}
			pDest[j][u8DestSize[j]] = 0;
			strncpy(pDest[j], pTmp + 1, u8DestSize[j]);
			pDest[j][strlen(pDest[j]) - 1] = 0;
		}
		if (s32Err != 0)
		{
			break;
		}
		pMap++;
	}
	if (s32Err != 0)
	{
		free(*p2Mapping);
		*p2Mapping = NULL;
		*pCnt = 0;
	}
	else
	{
		*pCnt = u32Cnt;
	}

	return s32Err;
}

/*
 * 函数名      : CloudGetRegionMapping
 * 功能        : 通过云得到区域的对应的domain
 * 参数        : pStat [in] (StCloudStat * 类型): 云状态，详见定义
 * 			   : p2Mapping [in/out] (StRegionMapping ** 类型): 成功返回申请的数组
 *             : pCnt [in/out] (uint32_t * 类型): 成功返回*p2Mapping申请的数组的大小
 * 返回        : 正确返回0, 错误返回错误码
 * 作者        : 许龙杰
 */
int32_t CloudGetRegionMapping(StCloudDomain *pStat, StRegionMapping **p2Mapping, uint32_t *pCnt)
{
	int32_t s32Err = 0;
	json_object *pJsonObj = NULL;
	char c8Char[PAGE_SIZE];

	if ((pStat == NULL) || (p2Mapping == NULL) || (pCnt == NULL))
	{
		return MY_ERR(_Err_InvalidParam);
	}
	pJsonObj = BuildupCmdGetRegionMapping(&s32Err);
	if (pJsonObj == NULL)
	{
		return MY_ERR(_Err_JSON);
	}

	sprintf(c8Char, "content=%s", json_object_to_json_string(pJsonObj));
	json_object_put(pJsonObj);
	pJsonObj = NULL;
	PRINT("%s\n", c8Char);
#if TEST_CLOUD
	{
	StMMap stMap = {NULL};
	StSendInfo stSendInfo = {false, NULL, AUTHENTICATION_ADDR, NULL, -1};

	stSendInfo.pSendBody = c8Char;
	stSendInfo.s32BodySize = -1;
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
	s32Err = ResolveCmdGetRegionMapping(pJsonObj, p2Mapping, pCnt);
	json_object_put(pJsonObj);
	pJsonObj = NULL;
	}
#endif
	return s32Err;
}
/*
 * 函数名      : CloudKeepAlive
 * 功能        : 云保活
 * 参数        : pStat [in] (StCloudDomain * 类型): 云状态，详见定义
 *             : c8ID [in] (const char * 类型): 产品ID
 * 返回        : 正确返回0, 错误返回错误码
 * 作者        : 许龙杰
 */
int32_t CloudKeepAlive(StCloudDomain *pStat, const char c8ID[PRODUCT_ID_CNT])
{
#if 0
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
#else
	return 0;
#endif
}


/*
 * 函数名      : UDPKAInit
 * 功能        : UDP保活超时结构体初始化, 与UDPKADestroy成对使用
 * 参数        : 无
 * 返回        : StUDPKeepalive *类型, 错误返回指针NULL, 其余正确
 * 作者        : 许龙杰
 */
StUDPKeepalive *UDPKAInit(void)
{
	int32_t i;
	StUDPKeepalive *pUDP = (StUDPKeepalive *)calloc(1, sizeof(StUDPKeepalive));

	if (pUDP == NULL)
	{
		return NULL;
	}
	for (i = 0; i < CMD_CNT - 1; i++)
	{
		pUDP->stUDPInfo[i].pNext = pUDP->stUDPInfo + i + 1;
	}
	pUDP->stUDPInfo[i].pNext = pUDP->stUDPInfo;
	pUDP->stUDPInfo[0].pPrev = pUDP->stUDPInfo + i;
	for (i = 1; i < CMD_CNT; i++)
	{
		pUDP->stUDPInfo[i].pPrev = pUDP->stUDPInfo + i - 1;
	}
	return pUDP;
}

/*
 * 函数名      : UDPKAAddASendTime
 * 功能        : 当发送一个心跳包后, 将报序号和发送的时间记录
 * 参数        : pUDP [in] (StUDPKeepalive *) UDPKAInit返回的结构体指针
 *             : u16SendNum [in] (uint16_t) 包序号
 *             : u64SendTime[in] (uint64_t) 发送的时间
 * 返回        : int32_t类型, 正确返回0, 否则返回错误码
 * 作者        : 许龙杰
 */
int32_t UDPKAAddASendTime(StUDPKeepalive *pUDP, uint16_t u16SendNum,
	uint64_t u64SendTime)
{
	if (pUDP == NULL)
	{
		return MY_ERR(_Err_InvalidParam);
	}

	pUDP->u16LatestSendNum = u16SendNum;

	if (pUDP->u16SendCnt == 0)
	{
		pUDP->u16OldestSendNum = u16SendNum;
		pUDP->pCur = pUDP->pOldest = pUDP->stUDPInfo;
	}
	pUDP->pCur->u16QueueNum = u16SendNum;
	pUDP->pCur->u64SendTime = u64SendTime;
	pUDP->pCur->u64ReceivedTime = 0;

	pUDP->pCur = pUDP->pCur->pNext;

	if (pUDP->u16SendCnt == CMD_CNT)
	{
		pUDP->pOldest = pUDP->pOldest->pNext;
	}
	else
	{
		pUDP->u16SendCnt++;
	}
	return 0;
}

/*
 * 函数名      : UDPKAAddAReceivedTime
 * 功能        : 当受到一个心跳包后, 将报序号, 接收的时间和服务器发送记录时的时间记录
 * 参数        : pUDP [in] (StUDPKeepalive *) UDPKAInit返回的结构体指针
 *             : Received [in] (uint16_t) 包序号
 *             : u64SendTime[in] (uint64_t) 接收的时间
 *             : u64ServerTime[in] (uint64_t) 服务器发送记录时的时间
 * 返回        : int32_t类型, 正确返回0, 否则返回错误码
 * 作者        : 许龙杰
 */
int32_t UDPKAAddAReceivedTime(StUDPKeepalive *pUDP, uint16_t u16ReceivedNum,
	uint64_t u64ReceivedTime, uint64_t u64ServerTime)
{

	if (pUDP == NULL)
	{
		return MY_ERR(_Err_InvalidParam);
	}
	if (pUDP->u16SendCnt == 0)
	{
		return MY_ERR(_Err_Common);
	}
	{
	int32_t i;
	StUDPInfo *pInfo = pUDP->pCur->pPrev;
	for (i = pUDP->u16SendCnt; i > 0; i++)
	{
		if (pInfo->u16QueueNum == u16ReceivedNum)
		{
			pUDP->u16LatestReceivedNum = u16ReceivedNum;
			pInfo->u64ReceivedTime = u64ReceivedTime;
			pInfo->u64ServerTime = u64ServerTime;
			return 0;
		}
		pInfo = pInfo->pPrev;
	}
	}
	return MY_ERR(_Err_Common);
}

/*
 * 函数名      : UDPKAAddAReceivedTime
 * 功能        : 判断当前记录的数据中, UDP保活是否已经超时
 * 参数        : pUDP [in] (StUDPKeepalive *) UDPKAInit返回的结构体指针
 * 返回        : bool类型, 超时返回true, 否则返回false
 * 作者        : 许龙杰
 */
bool UPDKAIsTimeOut(StUDPKeepalive *pUDP)
{
	if (pUDP == NULL)
	{
		return true;
	}

	if ((pUDP->u16LatestSendNum - pUDP->u16LatestReceivedNum) > TIMEOUT_CNT)
	{
		return true;
	}
	return false;
}

/*
 * 函数名      : UDPKAGetTimeDiff
 * 功能        : 得到本地时间与服务器之间的时间差
 * 参数        : pUDP [in] (StUDPKeepalive *) UDPKAInit返回的结构体指针
 *             : pTimeDiff [in/out] (int64_t *) 用于保存时间差
 * 返回        : int32_t类型, 正确返回0, 否则返回错误码
 * 作者        : 许龙杰
 */
int32_t UDPKAGetTimeDiff(StUDPKeepalive *pUDP, int64_t *pTimeDiff)
{
	int32_t i;
	int64_t s64TimeDiff = 0;

	if (pUDP == NULL)
	{
		return  MY_ERR(_Err_Common);
	}
	if (pUDP->u16SendCnt != CMD_CNT)
	{
		return  MY_ERR(_Err_Common);
	}
	for (i = 0; i < CMD_CNT; i++)
	{
		StUDPInfo *pInfo = pUDP->stUDPInfo + i;
		if (pInfo->u64ReceivedTime != 0)
		{
			uint64_t u64EchoTimeDiff = pInfo->u64ReceivedTime - pInfo->u64SendTime;
			u64EchoTimeDiff = pInfo->u64SendTime + u64EchoTimeDiff / 2;
			s64TimeDiff += (pInfo->u64ServerTime - u64EchoTimeDiff);
		}
		else
		{
			return  MY_ERR(_Err_Common);
		}
	}

	s64TimeDiff /= CMD_CNT;
	if (pTimeDiff != NULL)
	{
		*pTimeDiff = s64TimeDiff;
	}

	return 0;
}

/*
 * 函数名      : UDPKAClearTimeDiff
 * 功能        : 清除时间差统计结果
 * 参数        : pUDP [in] (StUDPKeepalive *) UDPKAInit返回的结构体指针
 * 返回        : int32_t类型, 正确返回0, 否则返回错误码
 * 作者        : 许龙杰
 */
int32_t UDPKAClearTimeDiff(StUDPKeepalive *pUDP)
{
	if (pUDP == NULL)
	{
		return  MY_ERR(_Err_Common);
	}

	pUDP->stUDPInfo[CMD_CNT - 1].u64ReceivedTime = 0;
	return 0;
}

/*
 * 函数名      : UDPKAReset
 * 功能        : 将记录数据复位
 * 参数        : pUDP [in] (StUDPKeepalive *) UDPKAInit返回的结构体指针
 * 返回        : 无
 * 作者        : 许龙杰
 */
void UDPKAReset(StUDPKeepalive *pUDP)
{
	if(pUDP != NULL)
	{
		pUDP->u16SendCnt = 0;
	}
}

/*
 * 函数名      : UDPKADestroy
 * 功能        : 销毁资源, 与UDPKAInit成对使用
 * 参数        : pUDP [in] (StUDPKeepalive *) UDPKAInit返回的结构体指针
 * 返回        : 无
 * 作者        : 许龙杰
 */
void UDPKADestroy(StUDPKeepalive *pUDP)
{
	if (pUDP != NULL)
	{
		free(pUDP);
	}
}

#define SDCARD_DIR		"/home/lyndon/workspace/SDCard/"
#define UPDATE_DIR		SDCARD_DIR"Update/"
#define RUN_DIR			SDCARD_DIR"Run/"
#define BACKUP_DIR		SDCARD_DIR"Backup/"
#define LIST_FILE		"FileList.json"

#define UPDATE_FILENAME_LENGTH			64
typedef struct _tagStUpdateFileInfo
{
	char c8Name[UPDATE_FILENAME_LENGTH];
	uint32_t u32FileType;
	uint32_t u32Version;
	uint32_t u32OldVersion;
	uint32_t u32CRC32;
}StUpdateFileInfo;


StUpdateFileInfo *GetUpdateFileArray(json_object *pListNew, json_object *pListOld)
{
	if ((pListNew == NULL) || (pListOld == NULL))
	{
		PRINT("error arg\n");
		return NULL;
	}
	else
	{
		int32_t i, s32UpdateCnt;
		int32_t s32NewLength = json_object_array_length(pListNew);
		int32_t s32OldLength = json_object_array_length(pListOld);
		StUpdateFileInfo *pNew, *pOld;
		char c8Buf[16];


		pNew = malloc(sizeof(StUpdateFileInfo) * (s32NewLength + 1));
		pOld = malloc(sizeof(StUpdateFileInfo) * s32OldLength);

		if ((pNew == NULL) || (pOld == NULL))
		{
			goto end;
		}

#define JSON_TRANSLATE(src, dest, length)\
for(i = 0; i < length; i++)\
{\
	json_object *pObj;\
	json_object *pObjNew = json_object_array_get_idx(src, i);\
	char *pTmp;\
	StUpdateFileInfo *pInfo = dest + i;\
	PRINT("\t[%d]=%s\n", i, json_object_to_json_string(pObjNew));\
\
	pObj = json_object_object_get(pObjNew, "FileName");\
	strncpy(pInfo->c8Name, json_object_to_json_string(pObj) + 1, UPDATE_FILENAME_LENGTH);\
	pTmp = strrchr(pInfo->c8Name, '_');\
	if (pTmp != NULL)\
	{\
		pTmp[0] = 0;	/* truncation */ \
	}\
	pObj = json_object_object_get(pObjNew, "FileType");\
	pInfo->u32FileType = json_object_get_int(pObj);\
\
	pObj = json_object_object_get(pObjNew, "Version");\
	strncpy(c8Buf, json_object_to_json_string(pObj), 16);\
	PRINT("version: %s\n", c8Buf);\
	pTmp = (char *)(&(pInfo->u32Version));\
	pInfo->u32Version = 0;\
	sscanf(c8Buf, "\"%hhd.%hhd.%hhd\"", pTmp + 2, pTmp + 1, pTmp);\
\
	pObj = json_object_object_get(pObjNew, "CRC32");\
	strncpy(c8Buf, json_object_to_json_string(pObj), 16);\
	sscanf(c8Buf, "%08X", &(pInfo->u32CRC32));\
}

		JSON_TRANSLATE(pListNew, pNew, s32NewLength);
		JSON_TRANSLATE(pListOld, pOld, s32OldLength);

		s32UpdateCnt = 0;
		for (i = 0; i < s32NewLength; i++)
		{
			int32_t j;
			for (j = 0; j < s32OldLength; j++)
			{
				if (strcmp(pNew[i].c8Name, pOld[j].c8Name) == 0)
				{
					PRINT("name: %s\n", pNew[i].c8Name);
					PRINT("version: %06X, old version: %06X\n",
							pNew[s32UpdateCnt].u32Version, pOld[j].u32Version);
					if (pNew[i].u32Version > pOld[j].u32Version)
					{
						pNew[s32UpdateCnt] = pNew[i];
						pNew[s32UpdateCnt].u32OldVersion = pOld[j].u32Version;
						s32UpdateCnt++;
					}
					break;
				}
			}
			if (j == s32OldLength)	/* new file */
			{
				pNew[s32UpdateCnt] = pNew[i];
				pNew[s32UpdateCnt].u32OldVersion = 0;
				s32UpdateCnt++;
			}
		}
		pNew[s32UpdateCnt].c8Name[0] = 0;
end:
		if (pOld != NULL)
		{
			free(pOld);
		}
		return pNew;
	}
}

int32_t UpdateFileCopyToRun()
{
	json_object *pListNew = NULL;
	json_object *pListOld = NULL;
	int32_t s32Err;
	StUpdateFileInfo *pInfo = NULL;

	char c8FileName[_POSIX_PATH_MAX];

	sprintf(c8FileName, "%s%s", UPDATE_DIR, LIST_FILE);
	PRINT("file name %s\n", c8FileName);
	pListNew = json_object_from_file(c8FileName);
	if (pListNew == NULL)
	{
		PRINT("json_object_from_file error\n");
		s32Err = MY_ERR(_Err_JSON);
		goto end;
	}

	sprintf(c8FileName, "%s%s", RUN_DIR, LIST_FILE);
	PRINT("file name %s\n", c8FileName);
	pListOld = json_object_from_file(c8FileName);
	if (pListOld == NULL)
	{
		PRINT("json_object_from_file error\n");
		s32Err = MY_ERR(_Err_JSON);
		goto end;
	}
	pInfo = GetUpdateFileArray(json_object_object_get(pListNew, "FileList"),
			json_object_object_get(pListOld, "FileList"));
	if (pInfo != NULL)
	{
		int32_t s32Cnt = 0;

		char c8Buf[1024];

		/* copy the list file from the run directory to backup directoy */
		sprintf(c8Buf, "cp -f %s%s %s%s", RUN_DIR, LIST_FILE, BACKUP_DIR, LIST_FILE);
		system(c8Buf);

		while(pInfo[s32Cnt].c8Name[0] != 0)
		{
			StUpdateFileInfo *pInfoTmp = pInfo + s32Cnt;
			PRINT("FileName: %s\n, type: %d, new version: %06X, old version: %06X\n",
					pInfoTmp->c8Name, pInfoTmp->u32FileType,
					pInfoTmp->u32Version, pInfoTmp->u32OldVersion);

			/* remove the backup file from the backup directory */
			sprintf(c8Buf, "rm -f %s%s_*", BACKUP_DIR, pInfoTmp->c8Name);
			system(c8Buf);

			/* copy the run file to the backup directory */
			sprintf(c8Buf, "cp -f %s%s_* %s", RUN_DIR, pInfoTmp->c8Name, BACKUP_DIR);
			system(c8Buf);

			/* remove the run file from the run directory */
			sprintf(c8Buf, "rm -f %s%s_*", RUN_DIR, pInfoTmp->c8Name);
			system(c8Buf);

			/* copy the update file to the run directory */
			sprintf(c8Buf, "cp -f %s%s_* %s", UPDATE_DIR, pInfoTmp->c8Name, RUN_DIR);
			system(c8Buf);

			s32Cnt++;
		}
		/* copy the list file from the update directory to run directoy */
		sprintf(c8Buf, "cp -f %s%s %s%s", UPDATE_DIR, LIST_FILE, RUN_DIR, LIST_FILE);
		system(c8Buf);

	}
end:
	if (pListNew != NULL)
	{
		json_object_put(pListNew);
	}
	if (pListOld != NULL)
	{
		json_object_put(pListOld);
	}
	if (pInfo != NULL)
	{
		free(pInfo);
	}
	return s32Err;
}


#if 0
void FunctionTest(void)
{
	struct addrinfo stFilter = {0};
	struct addrinfo *pRslt = NULL;
	int32_t s32Err = 0;
	stFilter.ai_family = AF_INET;
	stFilter.ai_socktype = SOCK_STREAM;
	s32Err = getaddrinfo("www.google.com.hk", NULL, &stFilter, &pRslt);
	if (s32Err == 0)
	{
		struct addrinfo *pTmp = pRslt;
		while (pTmp != NULL)
		{
			PRINT("after inet_ntoa: %s\n", inet_ntoa(((struct sockaddr_in*)(pTmp->ai_addr))->sin_addr));
			pTmp = pTmp->ai_next;
		}
	}
	else
	{
		PRINT("getaddrinfo s32Err = %d, error: %s\n", s32Err, gai_strerror(s32Err));
	}

}
#endif

#if 0

void FunctionTest(void)
{
	struct hostent *pHost;
	struct sockaddr_in stAddr;
	char c8Addr[64];

	pHost = gethostbyname("www.baidu.com");
	PRINT("www.baidu.com's host(name) is: %s\n", pHost->h_name);
	PRINT("www.baidu.com's host(AF_INET) is: %hhu.%hhu.%hhu.%hhu\n",
			pHost->h_addr[0], pHost->h_addr[1], pHost->h_addr[2], pHost->h_addr[3]);
	PRINT("after inet_ntoa: %s\n", inet_ntoa(*((struct in_addr *)pHost->h_addr)));

	pHost = gethostbyname("74.125.128.199");
	PRINT("74.125.128.199's host is: %s\n", pHost->h_addr);
	PRINT("after inet_ntoa: %s\n", inet_ntoa(*((struct in_addr *)pHost->h_addr)));


}
#endif

#if 0
/*
 * {IDPS:{"Ptl":"com.jiuan.GW01","Name":" Health Gateway","FVer":"1.0.2","HVer":"1.1.1","MFR":"iHealth","Model":"GW 001","SN":"0000000001"},Ptr:{"Region"="CN"}}
 */
void JSONTest()
{
	json_object *new_obj, *pSon_obj, *pJson;
	new_obj = json_object_from_file("/home/lyndon/workspace/nfsboot/GatewayIDPS.idps");
	printf("new_obj.to_string()=%s\n", json_object_to_json_string_ext(new_obj, JSON_C_TO_STRING_PRETTY));

	pJson = json_object_new_object();

	printf("\n\n");
	json_object_object_get_ex(new_obj, "IDPS", &pSon_obj);
	printf("pSon_obj(IDPS).to_string()=%s\n", json_object_to_json_string_ext(pSon_obj, JSON_C_TO_STRING_PRETTY));

	//json_object_get(pSon_obj);
	json_object_object_add(pJson, "IDPS", pSon_obj);

	printf("\n\n");
	json_object_object_get_ex(new_obj, "Ptr", &pSon_obj);
	printf("pSon_obj(Ptr).to_string()=%s\n", json_object_to_json_string(pSon_obj));

	json_object_put(new_obj);

	printf("\n\n");
	printf("pJson.to_string()=%s\n", json_object_to_json_string(pJson));
	json_object_put(pJson);

}
#endif

#if 1
void JSONTest()
{
	json_object *new_obj, *pSon_obj;
	new_obj = json_tokener_parse("{\"Result\": \"1\",\n\t\t \"TS\": 19923928382000, "\
			"\"ResultMessage\": \"100\", \"ReturnValue\": null }DADADsdf}");
	json_object_to_file("/home/lyndon/workspace/nfsboot/GatewayIDPS.idps", new_obj);
	printf("new_obj.to_string()=%s\n", json_object_to_json_string(new_obj));

	printf("\n\n");
	json_object_object_get_ex(new_obj, "Result", &pSon_obj);
	printf("pSon_obj(Result).to_string()=%s\n", json_object_to_json_string(pSon_obj));

	printf("\n\n");
	json_object_object_get_ex(new_obj, "TS", &pSon_obj);
	printf("pSon_obj(TS).to_string()=%s\n", json_object_to_json_string(pSon_obj));
	printf("pSon_obj(TS).type=%d\n", json_object_get_type(pSon_obj));

	printf("\n\n");
	json_object_object_get_ex(new_obj, "ResultMessage", &pSon_obj);
	printf("pSon_obj(ResultMessage).to_string()=%s\n", json_object_to_json_string(pSon_obj));

	printf("\n\n");
	json_object_object_get_ex(new_obj, "ReturnValue", &pSon_obj);
	printf("pSon_obj(ReturnValue).to_string()=%s\n", json_object_to_json_string(pSon_obj));
	printf("pSon_obj(ReturnValue).type=%d\n", json_object_get_type(pSon_obj));

	json_object_put(new_obj);
}
#endif
