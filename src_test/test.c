/****************************************************************************
 * Copyright(c), 2001-2060, ******************************* 版权所有
 ****************************************************************************
 * 文件名称             : test.c
 * 版本                 : 0.0.1
 * 作者                 : 许龙杰
 * 创建日期             : 2014年5月7日
 * 描述                 : 
 ****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "upgrade.h"

#if 1
int main()
{
	return 0;
}

#endif

#if 0
int main()
{
	char c8IPV4[IPV4_ADDR_LENGTH];
	GetHostIPV4Addr("www.baidu.com", NULL, NULL);
	return 0;
}

#endif

#if 0
int main()
{
	void JSONTest();
	JSONTest();
	return 0;
}

#endif

#if 0
#define		FILE_MODE	(S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
#define		TEST_CNT	500
int main(void)
{
	DBHANDLE	db;
	int i;
	char c8Buf[TEST_CNT][32], c8Data[64];
	uint64_t u64Time;
	srand((int)time(NULL));
	if ((db = db_open("/tmp/db", O_RDWR | O_CREAT | O_TRUNC, FILE_MODE)) == NULL)
	{
		printf("db_open error\n");
		exit(1);
	}

	for (i = 0; i < TEST_CNT; i++)
	{
		int s32Cnt = (rand() % 20) + 5, j;
		sprintf(c8Buf[i], "SN%05d%05d", rand() % 100000, rand() % 100000);
		for(j = 0; j < s32Cnt; j++)
		{
			c8Data[j] = (rand() % 10) + '0';
		}
		c8Data[j] = 0;
		if (db_store(db, c8Buf[i], c8Data, DB_STORE) == -1)
		{
			printf("db_store error for %s, %s\n", c8Buf[i], c8Data);
			exit(1);
		}
	}
	u64Time = TimeGetTime();
	for (i = 0; i < TEST_CNT; i++)
	{
		if (db_fetch(db, c8Buf[i]) == NULL)
		{
			printf("db_fetch error for %d----%s\n", i, c8Buf[i]);
			exit(1);
		}
	}
	u64Time = TimeGetTime() - u64Time;
	printf("find %d entry using time: %lldms\n", TEST_CNT, u64Time);

	db_close(db);
	exit(0);
}
#endif

#if 0
#define TEST_CNT	4096
#if 1 /* mmap memory file */
#include <sys/mman.h>
#include <unistd.h>
int main()
{
	char c8Buf[4096] = {0};
	int32_t i, s32Cnt;
	FILE *pFile;
	int32_t s32Fd;
	void *pMapAddr = NULL;
	uint64_t u64Tmp;
	uint64_t u64Time;

	for (i = 0; i < 4095; i++)
	{
		c8Buf[i] = rand() % 94 + ' ';
	}
	c8Buf[i] = 0;

	pFile = tmpfile();
	s32Fd = fileno(pFile);

	ftruncate(s32Fd, 4096);

	pMapAddr = mmap(0, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, s32Fd, 0);

	u64Time = TimeGetTime();
	s32Cnt = 0;
	while (s32Cnt < TEST_CNT)
	{
		memcpy(pMapAddr, c8Buf, 4096);
		memcpy(c8Buf, pMapAddr, 4096);
		s32Cnt++;
	}
	u64Time = TimeGetTime() - u64Time;
	printf("mmap: %d, time using %lldms\n", TEST_CNT, u64Time);
	munmap(pMapAddr, 4096);
	fclose(pFile);
	u64Tmp = (uint64_t)4096 * 2 * TEST_CNT * 1000 * 100;
	u64Tmp /= u64Time;
	u64Tmp /= 1024;
	u64Tmp /= 1024;
	printf("copy speed %lld.%02lldM/s\n", u64Tmp / 100, u64Tmp % 100);

	return 0;
}
#else
int main()
{
	char c8Buf[4096] = {0};
	int32_t i, s32Cnt;
	void *pMapAddr = NULL;
	uint64_t u64Tmp;
	uint64_t u64Time;

	for (i = 0; i < 4095; i++)
	{
		c8Buf[i] = rand() % 94 + ' ';
	}
	pMapAddr = malloc(4096);
	u64Time = TimeGetTime();
	s32Cnt = 0;
	while (s32Cnt < TEST_CNT)
	{
		memcpy(pMapAddr, c8Buf, 4096);
		memcpy(c8Buf, pMapAddr, 4096);
		s32Cnt++;
	}
	u64Time = TimeGetTime() - u64Time;
	printf("memory: %d, time using %lldms\n", TEST_CNT, u64Time);
	u64Tmp = (uint64_t)4096 * 2 * TEST_CNT * 1000 * 100;
	u64Tmp /= u64Time;
	u64Tmp /= 1024;
	u64Tmp /= 1024;
	printf("copy speed %lld.%02lldM/s\n", u64Tmp / 100, u64Tmp % 100);
	free(pMapAddr);
	return 0;
}
#endif
#endif

#if 0
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#define TEST_SOCKET		"/tmp/test.sock"
#define TEST_FD			0

int main(int argc, char *argv[])
{
	char c8Buf[4096] = {0};
	int32_t i;
	for (i = 0; i < 4095; i++)
	{
		c8Buf[i] = rand() % 94 + ' ';
	}
	c8Buf[i] = 0;
	if (argc > 1)	/* client */
	{
		int32_t s32Client;
#if TEST_FD
		int32_t s32FD;
		FILE *pFile = NULL;
#endif
		int32_t s32Cnt = 0;
		uint64_t u64TimeEnd;
		uint64_t u64Time = TimeGetTime();
		printf("client in %d\n", getpid());
#if TEST_FD
		pFile = tmpfile();
		fwrite(c8Buf, 4096, 1, pFile);
#endif
		while (s32Cnt < 4096)
		{
#if TEST_FD


			s32FD = fileno(pFile);
			s32Client = ClientConnect(TEST_SOCKET);
			SendFd(s32Client, s32FD);

#else
			s32Client = ClientConnect(TEST_SOCKET);
			MCSSyncSendData(s32Client, 1000, 4096, c8Buf);
			close(s32Client);
#endif
			close(s32Client);
			s32Cnt++;
		}
#if 0
		fclose(pFile);
#endif
		u64TimeEnd = TimeGetTime();
		printf("time using %lldms\n", u64TimeEnd - u64Time);
	}
	else			/* server */
	{
		int32_t s32Server = ServerListen(TEST_SOCKET);
		int32_t s32Client;
#if TEST_FD
		int32_t s32FD;
#endif
		printf("server in %d\n", getpid());
		while (1)
		{
			s32Client = ServerAccept(s32Server);

#if TEST_FD
			s32FD = ReceiveFd(s32Client);

			//printf("get fd %d\n", s32FD);
			//printf("press enter key to continue\n");
			//getchar();
			//lseek(s32FD, 0, SEEK_SET);
			//c8Buf[0] = 0;
			read(s32FD, c8Buf, 4096);
			//printf("get message: %s\n", c8Buf);
			close(s32FD);
#else
			uint32_t u32Size;
			void *pBuf = MCSSyncReceive(s32Client,false, 1000, &u32Size, NULL);
			MCSSyncFree(pBuf);
#endif
			close(s32Client);
		}
		printf("press enter key to exit\n");
		getchar();
		ServerRemove(s32Server, TEST_SOCKET);
	}

	return 0;
}

#endif

#if 0
int main()
{
	StCloudDomain stStat = {{_Cloud_IsOnline}};
	const char c8ID[PRODUCT_ID_CNT] = {"01234567/9012345"};
	const char c8Key[XXTEA_KEY_CNT_CHAR] = {"0123456789012345"};
	CloudAuthentication(&stStat, c8ID, c8Key);
	CloudKeepAlive(&stStat, c8ID);
	return 0;
}
#endif
#if 0

int main()
{
	int32_t s32Key[4] =
	{
		0x01234567, 0x89ABCDEF, 0xFEDCBA98, 0x76543210
	};
	char c8ID[16];
	int32_t i, j;
	srand(time(NULL));
	for (j = 0; j < 36; j++)
	{
		printf("org ID is: ");
		for (i = 0; i < 16; i++)
		{
			c8ID[i] = (rand() % 10) + '0';
			printf("%c", c8ID[i]);
		}
		printf("\n coding is: ");
		btea((int32_t *)c8ID, 4, s32Key);
		for (i = 0; i < 16; i++)
		{
			printf("%02hhx ", c8ID[i]);
		}
		btea((int32_t *)c8ID, -4, s32Key);
		printf("\n decoding is: ");
		for (i = 0; i < 16; i++)
		{
			printf("%c", c8ID[i]);
		}
		printf("\n\n\n");
	}

	return 0;
}


#endif

#if 0
//ebsnew.boc.cn/boc15/login.html
//pbnj.ebank.cmbchina.com/CmbBank_GenShell/UI/GenShellPC/Login/Login.aspx
//ebank.spdb.com.cn/per/gb/otplogin.jsp
int main()
{
	StCloudDomain stStat = { {_Cloud_IsOnline, "10.0.0.110"}, "spdb.com.cn", 443};
	StSendInfo  stInfo = { false, "ebank", "per/gb/otplogin.jsp"};
	StMMap stMMap = {NULL,};
	int32_t s32Err;
#if 0
	SSL_Init();
	printf("network: %s, IP address: %s begin to try\n", "br0", stStat.c8ClientIPV4);
	s32Err = CloudSendAndGetReturn(&stStat, &stInfo, &stMMap);
	if (s32Err == 0)
	{
		FILE *pFile = fopen("login.jsp", "wb+");
		fwrite(stMMap.pMap, stMMap.u32MapSize, 1, pFile);
		fclose(pFile);
		CloudMapRelease(&stMMap);
	}
	SSL_Destory();
#else
	StIPV4Addr stAddr[8];
	uint32_t i, u32AddrSize = 8;



	s32Err = GetIPV4Addr(stAddr, &u32AddrSize);
	if (s32Err != 0)
	{
		return -1;
	}
#if 1
	SSL_Init();

	for (i = 0; i < u32AddrSize; i++)
	{
		if (strcmp(stAddr[i].c8Name, "lo") == 0)
		{
			continue;
		}
		printf("network: %s, IP address: %s begin to try\n", stAddr[i].c8Name, stAddr[i].c8IPAddr);
		strncpy(stStat.stStat.c8ClientIPV4, stAddr[i].c8IPAddr, 16);
		s32Err = CloudSendAndGetReturn(&stStat, &stInfo, &stMMap);
		if (s32Err == 0)
		{
			FILE *pFile = fopen("login.jsp", "wb+");
			fwrite(stMMap.pMap, stMMap.u32MapSize, 1, pFile);
			fclose(pFile);
			CloudMapRelease(&stMMap);
		}
	}

	SSL_Destory();
#endif
#endif
	return 0;
}
#endif


#if 0
#include <sys/types.h>
#include <unistd.h>
int main()
{
	FILE *pFile = tmpfile();
	char c8Buf[16] = {0};

	fwrite("test ok", sizeof("test ok"), 1, pFile);
	fflush(pFile);
	printf("PID is %d, fileno is %d\n", getpid(), fileno(pFile));
	printf("press enter key to continue\n");
	getchar();

	fseek(pFile, 0, SEEK_SET);
	fread(c8Buf, 16, 1, pFile);

	printf("file: %s", c8Buf);

	printf("press enter key to continue\n");
	getchar();

	fclose(pFile);
	return 0;
}
#endif
#if 0


#include <signal.h>
#include <unistd.h>


bool g_boIsExit = false;

static void ProcessStop(int32_t s32Signal)
{
    printf("Signal is %d\n", s32Signal);
    g_boIsExit = true;
}


int main(int argc, char *argv[])
{
	int32_t s32Handle, s32Err;

	signal(SIGINT, ProcessStop);
	signal(SIGTERM, ProcessStop);

	s32Handle = ProcessListInit(&s32Err);
	if (s32Handle == 0)
	{
		printf("ProcessListInit error 0x%08x\n", s32Err);
		PrintLog("ProcessListInit error 0x%08x\n", s32Err);
	}
	while (!g_boIsExit)
	{
		ProcessListUpdate(s32Handle);
		sleep(1);
	}
	ProcessListDestroy(s32Handle);
	return 0;
}


#endif




#if 0
#include "common_define.h"
#include "upgrade.h"

int32_t Callback(const char *pCurPath, struct dirent *pInfo, void *pContext)
{
	printf("%s%s\n", pCurPath, pInfo->d_name);
	return 0;
}

int main(int argc, char *argv[])
{
	if (argc < 2)
	{
		printf("usage: %s directory\n", argv[0]);
	}

	TranversaDir(argv[1], true, Callback, NULL);

	return 0;
}
#endif

#if 0
int main(int argc, char *argv[])
{
	int i;
	int32_t s32Day = 15;
	int32_t s32Handle = LogFileCtrlInit(NULL, NULL);
	(void)i;
	(void)s32Day;
#if 1
#if 1
#if 0
	for (i = 0; i < 5; i++)
	{
		int32_t j;
		for (j = 0; j < 24; j++)
		{
			int32_t k;
			for (k = 0; k < 60; k++)
			{
				int32_t m;
				for (m = 0; m < 6; m++)
				{
					LogFileCtrlDelAOldestBlockLog(s32Handle);
				}
			}

		}
	}
#endif
	for (i = 0; i < 146; i++)
	{
		LogFileCtrlDelAOldestBlockLog(s32Handle);
	}

#else
	for (i = 0; i < 10; i++)
	{
		int32_t j;
		for (j = 0; j < 24; j++)
		{
			int32_t k;
			for (k = 0; k < 60; k++)
			{
				int32_t m;
				for (m = 0; m < 6; m++)
				{
					/* 10s, a message */
					char c8Buf[128];
					c8Buf[0] = 0;
					sprintf(c8Buf, LOG_DATE_FORMAT"%s(%d)\n",
						2014, 5, i + s32Day,
						j, k, m * 10, rand() % 1000,
						"Test OOOOOOOOOOOOOOOOOOOOOOOOOOOOK!",
						m);
					LogFileCtrlWriteLog(s32Handle, c8Buf, -1);
				}
			}

		}
	}
#endif
#endif

	LogFileCtrlDestroy(s32Handle);

	PRINT("ptess enter key to exit\n");
	getchar();	return 0;
}

#endif


#if 0
int main(int argc, char *argv[])
{
	int32_t s32Pid = 0;
	StSYSInfo stSYSInfo = {{0}, {0}};
	StProcessInfo stProcessInfo = {0};
	if (argc == 2)
	{
		s32Pid = atoi(argv[1]);
	}

	if (s32Pid == 0)
	{
		s32Pid = getpid();
	}


	while (1)
	{
		StProcessStat stProcessStat = {0};
		int32_t s32Err;
		CpuInfo(&(stSYSInfo.stCPU));
		MemInfo(&(stSYSInfo.stMem));


		s32Err = GetProcessStat(s32Pid, &stProcessStat);
		if (s32Err == 0)
		{
			uint64_t u64Tmp2 = stProcessInfo.u32SysTime + stProcessInfo.u32UserTime;
			uint64_t u64Tmp;
			stProcessInfo.u32SysTime = stProcessStat.u32Stime;
			stProcessInfo.u32UserTime = stProcessStat.u32Utime;
			u64Tmp = stProcessInfo.u32SysTime + stProcessInfo.u32UserTime;

			u64Tmp = (u64Tmp - u64Tmp2) * 10000 /
					(stSYSInfo.stCPU.u64Total - stSYSInfo.stCPU.u64PrevTotal);
			u64Tmp *= sysconf(_SC_NPROCESSORS_ONLN);
			stProcessInfo.u16CPUUsage = u64Tmp;
			u64Tmp2 = stProcessInfo.u16CPUAverageUsage;
			u64Tmp = u64Tmp2 * (100 - AVERAGE_WEIGHT) + u64Tmp * AVERAGE_WEIGHT;
			u64Tmp /= 100;
			stProcessInfo.u16CPUAverageUsage = u64Tmp;

			u64Tmp = stProcessInfo.u32RSS = stProcessStat.s32Rss;
			u64Tmp = ((u64Tmp / 1024 ) * 10000) / stSYSInfo.stMem.u32MemTotal;
			stProcessInfo.u16MemUsage = u64Tmp;

			u64Tmp2 = stProcessInfo.u16MemAverageUsage;
			u64Tmp = u64Tmp2 * (100 - AVERAGE_WEIGHT) + u64Tmp * AVERAGE_WEIGHT;
			u64Tmp /= 100;
			stProcessInfo.u16MemAverageUsage = u64Tmp;

			printf("Process CPU: %d.%02d, %d.%02d\n",
					stProcessInfo.u16CPUUsage / 100, stProcessInfo.u16CPUUsage % 100,
					stProcessInfo.u16CPUAverageUsage / 100, stProcessInfo.u16CPUAverageUsage % 100);

			printf("Process Mem: %d.%02d, %d.%02d\n",
					stProcessInfo.u16MemUsage / 100, stProcessInfo.u16MemUsage % 100,
					stProcessInfo.u16MemAverageUsage / 100, stProcessInfo.u16MemAverageUsage % 100);
		}
		sleep(1);
	}

	return 0;
}
#endif


#if 0

static bool s_boIsExit = false;
static void ProcessStop(int32_t s32Signal)
{
    PRINT("Signal is %d\n", s32Signal);
    s_boIsExit = true;
}

static int32_t MCSCallBack(uint32_t u32CmdNum, uint32_t u32CmdCnt, uint32_t u32CmdSize,
        const char *pCmdData, void *pContext)
{
    FILE *pLogFile = (FILE *)pContext;

    if ((u32CmdNum == 0x00006001) || (u32CmdNum == 0x00006002))
	{
		fwrite(pCmdData, u32CmdSize, u32CmdCnt, pLogFile);
	}
    else
	{
		fprintf(pLogFile, "error command: 0x%08x\n", u32CmdNum);
	}
    fflush(pLogFile);
    return 0;
}

void *ThreadLogServer(void *pArg)
{
	FILE *pLogFile;
	int32_t s32LogSocket = -1;


	pLogFile = fopen(LOG_FILE, "wb+");

	if (pLogFile == NULL)
	{
		PRINT("fopen %s error: %s\n", LOG_FILE, strerror(errno));
		return NULL;
	}
	s32LogSocket = ServerListen(LOG_SOCKET);
	if (s32LogSocket < 0)
	{

		PRINT("ServerListen error: 0x%08x\n", s32LogSocket);
		fclose(pLogFile);
		return NULL;
	}

	while (!s_boIsExit)
	{
        fd_set stSet;
        struct timeval stTimeout;

        int32_t s32Client = -1;
        stTimeout.tv_sec = 2;
        stTimeout.tv_usec = 0;
        FD_ZERO(&stSet);
        FD_SET(s32LogSocket, &stSet);

        if (select(s32LogSocket + 1, &stSet, NULL, NULL, &stTimeout) <= 0)
        {
            continue;
        }
        s32Client = ServerAccept(s32LogSocket);
        if (s32Client < 0)
        {
        	PRINT("ServerAccept error: 0x%08x\n", s32Client);
        	break;
        }
        else
        {
            uint32_t u32Size = 0;
            void *pMCSStream;
            pMCSStream = MCSSyncReceive(s32Client, 1000, &u32Size, NULL);
            if (pMCSStream != NULL)
            {
                MCSResolve((const char *)pMCSStream, u32Size, MCSCallBack, pLogFile);
                MCSSyncFree(pMCSStream);
            }
            close(s32Client);
        }
	}
	ServerRemove(s32LogSocket, LOG_SOCKET);
	fclose(pLogFile);
	return NULL;
}

int main(int argc, char *argv[])
{
	pthread_t s32LogServerPid = 0;
	int32_t s32Err, s32Cnt;

	system("mkdir -p "LOG_DIR);
	signal(SIGINT, ProcessStop);
	signal(SIGTERM, ProcessStop);

	s32Err = MakeThread(ThreadLogServer, NULL, false, &s32LogServerPid, false);

	if (s32Err != 0)
	{
		PRINT("MakeThread error: 0x%08x\n", s32Err);
		return -1;
	}
	s32Cnt = 0;
	while (!s_boIsExit)
	{
		s32Err = PrintLog("Log test %d\n", s32Cnt++);
		PRINT("PrintLog error code: 0x%08x\n", s32Err);
		sleep(1);
	}

	pthread_join(s32LogServerPid, NULL);

	return 0;
}

#endif



#if 0

#if 1
int main(int argc, char *argv[])
{
	int32_t s32Handle;
	if (argc > 1)
	{
		s32Handle = ProcessListCreate(NULL);
	}
	else
	{
		int32_t i = 0;
		s32Handle = ProcessListInit(NULL);
		printf("ProcessListUpdate\n");
		for (i = 0; i < 5; i++)
		{
			int32_t s32Err = ProcessListUpdate(s32Handle);
			printf("ProcessListUpdate error: 0x%08x\n", s32Err);
		}
	}

	printf("Hello World!\nPress enter key to exit!\n");
	getchar();

	if (argc > 1)
	{
		ProcessListTombDestroy(s32Handle);
	}
	else
	{
		ProcessListDestroy(s32Handle);
	}

	return 0;
}


#else

static int32_t Compare(void *pLeft, void *pAdditionData, void *pRight)
{
	if (((StProcessInfo *)pLeft)->u32Pid == ((StProcessInfo *)pRight)->u32Pid)
	{
		return 0;
	}
	else
	{
		return -1;
	}
}

static int32_t CompareError(void *pLeft, void *pAdditionData, void *pRight)
{
	if (((StProcessInfo *)pLeft)->u32Pid == ((StProcessInfo *)pRight)->u32Pid)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

int main(int argc, char *argv[])
{
	StProcessInfo stInfo = { getpid(), "TestProcess", 0 };
	int32_t s32Handle, s32BaseNumber;
	int32_t i;

	if (argc != 2)
	{
		printf("usage: %s [basenumber]\n", argv[0]);
		exit(0);
	}
	s32BaseNumber = atoi(argv[1]);

	s32Handle = SLOInit("ProcessList", 256, sizeof(StProcessInfo), 256, NULL);

	for (i = 0; i < 6; i++)
	{
		int32_t s32Index = 0;
		stInfo.u32Pid = s32BaseNumber + i;
		if (i == 5)
		{
			s32Index = SLOInsertAnEntity(s32Handle, &stInfo, NULL);
		}
		else
		{
			s32Index = SLOInsertAnEntity(s32Handle, &stInfo, NULL);
		}
		PRINT("the entity index is: %d\n", s32Index);
	}
#if 1
	stInfo.u32Pid = 1 + s32BaseNumber;
	for (i = 0; i < 7; i++)
	{
		PRINT("\n\nUpdate %dth\n", i);
		SLOUpdateAnEntity(s32Handle, &stInfo, Compare);
		SLOTraversal(s32Handle, NULL, NULL);
	}

	stInfo.u32Pid = 1 + s32BaseNumber;
	PRINT("\n\nTraversal error\n");
	SLOTraversal(s32Handle, &stInfo, CompareError);
#if 1
	PRINT("\n\nTraversal normally\n");
	SLOTraversal(s32Handle, NULL, NULL);
#endif

#if 0
	for (i = 5; i >=0; i--)
	{
		if (i == 0)
		{
			//SLODeleteAnEntity(s32Handle, stInfo + i, Compare);
			SLODeleteAnEntityUsingIndex(s32Handle, i);
		}
		else
		{
			//SLODeleteAnEntity(s32Handle, stInfo + i, Compare);
			SLODeleteAnEntityUsingIndex(s32Handle, i);
		}
	}
#endif
#endif
	printf("Hello World!\nPress enter key to exit!\n");
	getchar();

	SLOTombDestroy(s32Handle);

	return 0;
}

#endif
#endif
#if 0
int main(void)
{
	int32_t s32Handle = 0, s32Err = 0, s32Cnt = 0;
	s32Handle = LockOpen(NULL, &s32Err);
	while(s32Cnt < 20)
	{
		PRINT("the %dth test\n", s32Cnt++);
		LockLock(s32Handle);
		sleep(2);
		LockUnlock(s32Handle);
	}
	LockClose(s32Handle);

	puts("!!!Hello World!!!"); /* prints !!!Hello World!!! */
	return 0;
}
#endif
