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
#include <stddef.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>


//#define JSON_TEST
//#define STAT_TEST
//#define UPDATE_TEST
//#define CRC32_FILE_TEST
//#define CRC32_TABLE_TEST
//#define KEEPALIVE_TEST
//#define UPDKA_TEST
//#define GET_HOST_IP_TEST
//#define DB_TEST
//#define MMAP_TEST
//#define SEND_FD_TEST
//#define AUTH_TEST
//#define XXTEA_TEST
//#define HTTPS_TEST
//#define TMPFILE_TEST
//#define PROCESS_LIST_TEST
//#define TRAVERSAL_DIR_TEST
//#define LOG_FILE_TEST
//#define USAGE_TEST
//#define LOG_SERVER_TEST
//#define SLO_TEST
//#define LOCK_TEST
#define COMMON_TEST

#if defined COMMON_TEST
int main()
{
	char c8Str[8];
	c8Str[8 - 1] = 0;
	strncpy(c8Str, "01234567879", 8 - 1);
	printf("%s\n", c8Str);
	return 0;
}

#elif defined JSON_TEST
#include "upgrade.h"

int main()
{
	void JSONTest();
	JSONTest();
	return 0;
}

#elif defined STAT_TEST
#include "upgrade.h"
int main(int argc, char *argv[])
{
	struct stat stStat;
	if (argc != 2)
	{
		printf("usage: %s file name\n", argv[0]);
		return -1;
	}

	if( stat(argv[1], &stStat) != 0)
	{
		printf("stat error: %s\n", strerror(errno));
	}
	else
	{
		printf("stat ok\n");
	}

	return 0;
}

#elif defined UPDATE_TEST
#include "upgrade.h"
int main()
{
	int32_t UpdateFileCopyToRun();
	UpdateFileCopyToRun();
	return 0;
}



#elif defined CRC32_FILE_TEST
#include "upgrade.h"

int main(int argc, char *argv[])
{
	uint32_t u32CRC32 = 0;
	if (argc != 2)
	{
		printf("usage: %s file name", argv[0]);
		return -1;
	}

	CRC32File(argv[1], &u32CRC32);

	printf("the CRC32 of file(%s) is: 0x%08X\n", argv[1], u32CRC32);

	return 0;
}

#elif defined CRC32_TABLE_TEST
#include "upgrade.h"

int main()
{
	FILE *pFile = fopen("CRC32_table", "wb+");
	int32_t i, j;
	uint32_t u32CRC32;
	fprintf(pFile, "static uint32_t s_u32CRC32Table[] = {\n");

	for (i = 0; i < 256; i++)
	{
		u32CRC32 = i;
		for (j = 0; j < 8; j++)
		{
			if ((u32CRC32 & 0x01) != 0)
			{
				u32CRC32 = (u32CRC32 >> 1) ^ 0xEDB88320;
			}
			else
			{
				u32CRC32 >>= 1;
			}
		}
		if ((i & 0x07) == 0)
		{
			fprintf(pFile, "\t");
		}
		fprintf(pFile, "0x%08X, ", u32CRC32);
		if ((i & 0x07) == 0x07)
		{
			fprintf(pFile, "\n");
		}
	}

	fprintf(pFile, "};\n");
	fclose(pFile);
	return 0;
}


#elif defined KEEPALIVE_TEST
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <sys/time.h>

#include "upgrade.h"

#ifndef PRINT
#define PRINT(x, ...) printf("[%s:%d]: "x, __FILE__, __LINE__, ##__VA_ARGS__)
#endif

#define CLOUD_KEEPALIVE_TIME		30
#define UDP_MSG_HEAD_NAME			"ANGW"
#define GW_TEST_SN					"0123456789012345"
#define UDP_MSG_HEAD_NAME_LENGTH	4


const char c_c8Domain[] = "www.jiuan.com:8000";



typedef struct _tagStUDPMsgHead
{
	char c8HeadName[UDP_MSG_HEAD_NAME_LENGTH];
	char c8Var[3];
	char c8Reserved;
	uint64_t u64TimeStamp;
	uint16_t u16Cmd;
	uint16_t u16CmdLength;
}StUDPMsgHead;
typedef struct _tagStUDPHeartBeat
{
	uint16_t u16QueueNum;
	char c8SN[16];
}StUDPHeartBeat;
typedef union _tagUnUDPMsg
{
	char c8Buf[64];
	struct
	{
		StUDPMsgHead stHead;
		void *pData;
	};
}UnUDPMsg;


#if HAS_CROSS
const char c_c8LanName[] = "eth2.2"; /* <TODO> */
const char c_c8WifiName[] = "wlp2s0"; /* <TODO> */
#else
const char c_c8LanName[] = "p8p1";
const char c_c8WifiName[] = "wlp2s0";
#endif

/* ./test_upgrade 203.195.202.228 6000 */
int main(int argc, char *argv[])
{
	int32_t s32Socket = -1;

	UnUDPMsg unUDPMsg;
	uint16_t u16QueueNum = 0;
	struct sockaddr stBindAddr = {0};
	struct sockaddr_in stServerAddr = {0};

	StIPV4Addr stIPV4Addr[4];
	int32_t s32LanAddr = -1;

	struct sockaddr_in *pTmpAddr = (void *)(&stBindAddr);

	uint64_t u64SendTime = TimeGetTime();

	StUDPKeepalive *pUDPKA = UDPKAInit();

	if (pUDPKA == NULL)
	{
		//PrintLog("error memory\n");
		PRINT("error memory\n");
		goto end;
	}


	if (argc != 3)
	{
		PRINT("usage: %s <server> <port>\n", argv[0]);
	}

	{
	uint32_t u32Cnt = 4;
	uint32_t i;
	while (1)
	{
		/* list the network */
		GetIPV4Addr(stIPV4Addr, &u32Cnt);
		for (i = 0; i < u32Cnt; i++)
		{
			if (strcmp(stIPV4Addr[i].c8Name, c_c8LanName) == 0)
			{
				s32LanAddr = i; /* well, the lan is connected */
				break;
			}
			else
			{
				PRINT("name is: %s\n", stIPV4Addr[i].c8Name);
			}
		}
		if (i != u32Cnt)
		{
			break;
		}
		sleep(1);
	}
	}
	s32Socket = socket(AF_INET, SOCK_DGRAM, 0);
	if (s32Socket < 0)
	{
		//PrintLog("socket error: %s\n", strerror(errno));
		PRINT("socket error: %s\n", strerror(errno));
	}
	pTmpAddr->sin_family = AF_INET;
	pTmpAddr->sin_port = htons(0);
	if (inet_aton(stIPV4Addr[s32LanAddr].c8IPAddr, &(pTmpAddr->sin_addr)) == 0)
	{
		PRINT("local IP Address Error!\n");
		goto end;
	}

	if (bind(s32Socket, &stBindAddr, sizeof(struct sockaddr)))
	{
		PRINT("bind error: %s\n", strerror(errno));
		goto end;
	}


	stServerAddr.sin_family = AF_INET;
	stServerAddr.sin_port = htons(atoi(argv[2]));
	if (GetHostIPV4Addr(argv[1], NULL, &(stServerAddr.sin_addr)) != 0)
	{
		goto end;
	}

	while (1)
	{
		fd_set stSet;
		struct timeval stTimeout;

		stTimeout.tv_sec = 1;
		stTimeout.tv_usec = 0;
		FD_ZERO(&stSet);
		FD_SET(s32Socket, &stSet);

		if (select(s32Socket + 1, &stSet, NULL, NULL, &stTimeout) <= 0)
		{
			goto next;
		}
		else if (FD_ISSET(s32Socket, &stSet))
		{
			/* we need read some info from the server */
			struct sockaddr_in stRecvAddr = {0};
			int32_t s32RecvLen = 0;
			{
			/* I don't think we should check the return value */
			socklen_t s32Len = sizeof(struct sockaddr_in);
			s32RecvLen = recvfrom(s32Socket, &unUDPMsg, sizeof(UnUDPMsg), 0, (struct sockaddr *)(&stRecvAddr), &s32Len);
			if (s32RecvLen <= 0 || (s32Len != sizeof (struct sockaddr_in)))
			{
				goto next;
			}
			}
			if(memcmp(&stServerAddr, &stRecvAddr, sizeof(struct sockaddr)) == 0)
			{
				if (memcmp(unUDPMsg.stHead.c8HeadName, UDP_MSG_HEAD_NAME, UDP_MSG_HEAD_NAME_LENGTH) != 0)
				{
					unUDPMsg.stHead.c8Var[0] = 0;
					PRINT("head name error: %s\n", unUDPMsg.stHead.c8HeadName);
					goto next;
				}
				/* my message */
				switch (unUDPMsg.stHead.u16Cmd)
				{
				case 1:		/* heartbeat */
				{
					uint64_t u64GetTime = TimeGetTime();
					StUDPHeartBeat *pData = (void *)(&(unUDPMsg.pData));

					UDPKAAddAReceivedTime(pUDPKA, pData->u16QueueNum,u64GetTime, unUDPMsg.stHead.u64TimeStamp);

					PRINT("get echo: %d\n", pData->u16QueueNum);
					PRINT("server %s:%d, and the length is: %d\n",
							inet_ntoa(stRecvAddr.sin_addr), htons(stRecvAddr.sin_port),
							s32RecvLen);


					{
						int32_t i;
						for (i = 0; i < s32RecvLen; i++)
						{
							printf("0x%02hhx ", unUDPMsg.c8Buf[i]);
							if ((i & 0x07) == 0x07)
							{
								printf("\n");
							}
						}
						printf("\n");
					}
					break;
				}
				case 3 ... 4:		/* config message */
				{
					break;
				}
				default:
					break;
				}
			}
			else
			{
				PRINT("Unknown server %s:%d(%d), and the length is: %d\n",
						inet_ntoa(stRecvAddr.sin_addr), htons(stRecvAddr.sin_port), stRecvAddr.sin_family,
						s32RecvLen);
				{
					int32_t i;
					for (i = 0; i < s32RecvLen; i++)
					{
						printf("0x%02hhx ", unUDPMsg.c8Buf[i]);
						if ((i & 0x07) == 0x07)
						{
							printf("\n");
						}
					}
					printf("\n");
				}
				goto next;
			}
		}
next:
		if ((TimeGetTime() - u64SendTime) < 1000)
		{
			int64_t s64TimeDiff = 0;
			if (UDPKAGetTimeDiff(pUDPKA, &s64TimeDiff) == 0)
			{
				PRINT("\n\ntime diff is: %lld\n", s64TimeDiff);
#if HAS_CROSS
				if ((s64TimeDiff < -1000) || (s64TimeDiff > 1000))
				{
					struct timeval stTime;
					uint64_t u64Time = TimeGetTime();
					u64Time += s64TimeDiff;
					stTime.tv_sec = u64Time / 1000;
					stTime.tv_usec = (u64Time % 1000) * 1000;
					settimeofday(&stTime, NULL);
					u64Time /= 1000;
					PRINT("adjust time to: %s\n\n", ctime((void *)(&u64Time)));
					/* after we adjust the time difference, we clear this statistics */
					UDPKAClearTimeDiff(pUDPKA);
				}
#endif
			}
			continue;
		}
		/* I don't think we should check the return value */
		memset(&unUDPMsg, 0, sizeof(UnUDPMsg));
		memcpy(unUDPMsg.stHead.c8HeadName, UDP_MSG_HEAD_NAME, UDP_MSG_HEAD_NAME_LENGTH);
		u64SendTime = unUDPMsg.stHead.u64TimeStamp = TimeGetTime();
		unUDPMsg.stHead.u16Cmd = 1;
		unUDPMsg.stHead.u16CmdLength = sizeof(StUDPHeartBeat);
		{
			StUDPHeartBeat *pData = (void *)(&(unUDPMsg.pData));
			pData->u16QueueNum = u16QueueNum;
			memcpy(pData->c8SN, GW_TEST_SN, sizeof(pData->c8SN));
		}
		sendto(s32Socket, &unUDPMsg, sizeof(StUDPMsgHead) + sizeof(StUDPHeartBeat), MSG_NOSIGNAL,
				(struct sockaddr *)(&stServerAddr), sizeof(struct sockaddr));

		UDPKAAddASendTime(pUDPKA, u16QueueNum, unUDPMsg.stHead.u64TimeStamp);

		PRINT("send a heart beat: %d at %lld\n", u16QueueNum, unUDPMsg.stHead.u64TimeStamp);
		{

			int32_t i;
			for (i = 0; i < sizeof(StUDPMsgHead) + sizeof(StUDPHeartBeat); i++)
			{
				printf("0x%02hhx ", unUDPMsg.c8Buf[i]);
				if ((i & 0x07) == 0x07)
				{
					printf("\n");
				}
			}
			printf("\n\n");

		}

		u16QueueNum++;

	}
end:
	UDPKADestroy(pUDPKA);
	if (s32Socket >= 0)
	{
		close(s32Socket);
	}
	return 0;
}

#elif defined UPDKA_TEST
int main()
{
	StUDPKeepalive *pUDP = UDPKAInit();
	int32_t i = 0;

	for (i = 0; i < 65536; i += 2)
	{
		UDPKAAddASendTime(pUDP, i >> 1, i);
		UDPKAAddAReceivedTime(pUDP, i >> 1, i, i);
	}
	UDPKADestroy(pUDP);

	return 0;
}

#elif defined GET_HOST_IP_TEST
#include "upgrade.h"
int main()
{
	char c8IPV4[IPV4_ADDR_LENGTH];
	GetHostIPV4Addr("www.baidu.com", NULL, NULL);
	return 0;
}


#elif defined DB_TEST
#include "upgrade.h"

#define		FILE_MODE	(S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
#define		TEST_CNT	500
int main(void)
{
	void *db;
	int i;
	char c8Buf[TEST_CNT][32], c8Data[64];
	uint64_t u64Time;
	srand((int)time(NULL));
	if ((db = DBOpen("/tmp/db", O_RDWR | O_CREAT | O_TRUNC, FILE_MODE)) == NULL)
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
		if (DBStore(db, c8Buf[i], c8Data, DB_STORE) == -1)
		{
			printf("db_store error for %s, %s\n", c8Buf[i], c8Data);
			exit(1);
		}
	}
	u64Time = TimeGetTime();
	for (i = 0; i < TEST_CNT; i++)
	{
		if (DBFetch(db, c8Buf[i]) == NULL)
		{
			printf("db_fetch error for %d----%s\n", i, c8Buf[i]);
			exit(1);
		}
	}
	u64Time = TimeGetTime() - u64Time;
	printf("find %d entry using time: %lldms\n", TEST_CNT, u64Time);

	DBClose(db);
	exit(0);
}

#elif defined MMAP_TEST
#include "upgrade.h"

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


#elif defined SEND_FD_TEST
#include "upgrade.h"
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


#elif defined AUTH_TEST
#ifndef PRINT
#define PRINT(x, ...) printf("[%s:%d]: "x, __FILE__, __LINE__, ##__VA_ARGS__)
#endif


#if HAS_CROSS
const char c_c8LanName[] = "eth2.2"; /* <TODO> */
const char c_c8WifiName[] = "wlp2s0"; /* <TODO> */
#else
const char c_c8LanName[] = "p8p1";
const char c_c8WifiName[] = "wlp2s0";
#endif

#include "upgrade.h"
/* ./test_upgrade 203.195.202.228 443 */
int main(int argc, char *argv[])
{
	StCloudDomain stStat = {{_Cloud_IsOnline}};
	StIPV4Addr stIPV4Addr[4];
	int32_t s32LanAddr = 0;
#if 1
	const char c8ID[PRODUCT_ID_CNT] = {"0123456789012345"};
	const char c8Key[XXTEA_KEY_CNT_CHAR] =
	{
		0xC5, 0x14, 0x68, 0x31, 0xB3, 0x9F, 0xF0, 0xCC,
		0x23, 0xA5, 0x9C, 0x30, 0x19, 0x6D, 0x38, 0x73
	};
#else
	const char c8ID[PRODUCT_ID_CNT] =
	{
		0x31, 0x34, 0x30, 0x36, 0x31, 0x38, 0x30, 0x30,
		0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x32, 0x30
	};
	const char c8Key[XXTEA_KEY_CNT_CHAR] =
	{
		0x6e, 0x2e, 0xcb, 0x96, 0x30, 0xb2, 0x98, 0x64,
		0x8c, 0xf9, 0x42, 0x12, 0x50, 0x95, 0x83, 0x36
	};

#endif
	if (argc != 3)
	{
		PRINT("usage: %s <server> <port>\n", argv[0]);
		return -1;
	}
	SSL_Init();
	{
	uint32_t u32Cnt = 4;
	uint32_t i;
	while (1)
	{
		/* list the network */
		GetIPV4Addr(stIPV4Addr, &u32Cnt);
		for (i = 0; i < u32Cnt; i++)
		{
			if (strcmp(stIPV4Addr[i].c8Name, c_c8LanName) == 0)
			{
				s32LanAddr = i; /* well, the lan is connected */
				break;
			}
			else
			{
				PRINT("name is: %s\n", stIPV4Addr[i].c8Name);
			}
		}
		if (i != u32Cnt)
		{
			break;
		}
		sleep(1);
	}
	}
	memcpy(stStat.stStat.c8ClientIPV4, stIPV4Addr[s32LanAddr].c8IPAddr, 16);
	memcpy(stStat.c8Domain, argv[1], strlen(argv[1]));
	stStat.s32Port = atoi(argv[2]);
	if (CloudAuthentication(&stStat, true, c8ID, c8Key) == 0)
	{
		PRINT("Auth OK!\n");
	}
	CloudKeepAlive(&stStat, c8ID);

	SSL_Destory();
	return 0;
}


#elif defined XXTEA_TEST
#include "upgrade.h"

int main()
{
#if 0
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
#else


	int32_t i;
	char c8RServer[36] = {"\"919C7F4CD2F361098D6F52AF835FE4A4\""}, c8R[16];
#if 1
	const char c8Key[16] =
	{
		0xC5, 0x14, 0x68, 0x31,
		0xB3, 0x9F, 0xF0, 0xCC,
		0x23, 0xA5, 0x9C, 0x30,
		0x19, 0x6D, 0x38, 0x73
	};
#else
	const char c8Key[16] =
	{
		0x31, 0x68, 0x14, 0xC5,
		0xCC, 0xF0, 0x9F, 0xB3,
		0x30, 0x9C, 0xA5, 0x23,
		0x73, 0x38, 0x6D, 0x19,
	};
#endif
	char c8Buf[4] = {0};
	for (i = 0; i < 16; i++)
	{
		uint32_t u32Tmp = i;
		uint32_t u32Tmp1 = i;
#if 0
		u32Tmp1 /= 4;
		u32Tmp1 += 1;
		u32Tmp1 *= 4;
		u32Tmp1 -= u32Tmp;
		u32Tmp1--;
#endif
		c8Buf[0] = c8RServer[u32Tmp1 * 2 + 1];		/* \" */
		c8Buf[1] = c8RServer[u32Tmp1 * 2 + 2];
		sscanf(c8Buf, "%02hhX", c8R + i);
		printf("%02hhX", c8R[i]);
	}
	printf("\n");
	btea((int32_t *)c8R, 4, (int32_t *)c8Key);
	for (i = 0; i < 16; i++)
	{
		printf("%02hhX", c8R[i]);
	}
	printf("\n");
#endif
	return 0;
}



#elif defined HTTPS_TEST
#include "upgrade.h"
//ebsnew.boc.cn/boc15/login.html
//pbnj.ebank.cmbchina.com/CmbBank_GenShell/UI/GenShellPC/Login/Login.aspx
//ebank.spdb.com.cn/per/gb/otplogin.jsp
/*
 * 1、api1--网关认证询问测试地址：
http://203.195.202.228:8008/API/auth.ashx

2、post内容：
content={"Sc":"001cfe2fe7044aa691d4e6eff9bfb56c",
"Sv":"56435ce5601f40c59b1db14405578f60","QueueNum":123,
"IDPS":{"Ptl":"com.jiuan.BPV20","SN":"00000001","FVer":"1.0.2",
"HVer":"1.0.1","MFR":"iHealth","Model":"BP3 11070","Name":"BP Monitor"},"Command":"F5"}

因为服务器redis服务还没有更新，所以目前返回结果为：
{"Result":3,"QueueNum":123,"ResultMessage":"5000","TS":1402907636419,"ReturnValue":"通用缓存异常"}
更新之后，正确结果:{"Result":1,"QueueNum":123,"ResultMessage":"1000","TS":1402906148304,"ReturnValue":"F0"}

如果仅仅为了测试通道发送和接收，目前已经具备条件，可以测试！
 *
 */
#define LOACAL_IP				"10.0.0.108"
#define HTTPS					1
#if 1
#define SERVER_DOMAIN			"203.195.202.228"
#if HTTPS
#define SERVER_PORT				443
#else
#define SERVER_PORT				8008
#endif
#define SERVER_SECOND_DOMAIN	NULL
#define SERVER_FILE				"API/auth.ashx"

#define SEND_BODY				"content={\"Sc\":\"001cfe2fe7044aa691d4e6eff9bfb56c\","\
								"\"Sv\":\"56435ce5601f40c59b1db14405578f60\",\"QueueNum\":123,"\
								"\"IDPS\":{\"Ptl\":\"com.jiuan.BPV20\",\"SN\":\"00000001\",\"FVer\":\"1.0.2\","\
								"\"HVer\":\"1.0.1\",\"MFR\":\"iHealth\",\"Model\":\"BP3 11070\","\
								"\"Name\":\"BP Monitor\"},\"Command\":\"F5\"}"
#define BODY_LENGTH				(sizeof(SEND_BODY) - 1)
#else
#define SERVER_DOMAIN			"www.baidu.com"
#define SERVER_PORT				80
#define SERVER_SECOND_DOMAIN	NULL
#define SERVER_FILE				"per/gb/otplogin.jsp"

#define SEND_BODY				NULL
#define BODY_LENGTH				0
#endif
int main()
{
	StCloudDomain stStat = { {_Cloud_IsOnline, LOACAL_IP}, SERVER_DOMAIN, SERVER_PORT};
	StSendInfo  stInfo = { false, SERVER_SECOND_DOMAIN, SERVER_FILE, SEND_BODY, BODY_LENGTH};
	StMMap stMMap = {NULL,};
	int32_t s32Err;

	StIPV4Addr stAddr[8];
	uint32_t i, u32AddrSize = 8;



	s32Err = GetIPV4Addr(stAddr, &u32AddrSize);
	if (s32Err != 0)
	{
		return -1;
	}
	SSL_Init();

	for (i = 0; i < u32AddrSize; i++)
	{
		if (strcmp(stAddr[i].c8Name, "lo") == 0)
		{
			continue;
		}
		printf("network: %s, IP address: %s begin to try\n", stAddr[i].c8Name, stAddr[i].c8IPAddr);
		strncpy(stStat.stStat.c8ClientIPV4, stAddr[i].c8IPAddr, 16);
#if HTTPS
		s32Err = CloudSendAndGetReturn(&stStat, &stInfo, &stMMap);
#else
		s32Err = CloudSendAndGetReturnNoSSL(&stStat, &stInfo, &stMMap);
#endif
		if (s32Err == 0)
		{
			FILE *pFile = fopen("login.jsp", "wb+");
			fwrite(stMMap.pMap, stMMap.u32MapSize, 1, pFile);
			fclose(pFile);
			CloudMapRelease(&stMMap);
		}
	}

	SSL_Destory();
	return 0;
}



#elif defined TMPFILE_TEST
#include "upgrade.h"
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



#elif defined PROCESS_LIST_TEST
#include "upgrade.h"

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



#elif defined TRAVERSAL_DIR_TEST
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

	TraversalDir(argv[1], true, Callback, NULL);

	return 0;
}


#elif defined LOG_FILE_TEST
#include "upgrade.h"
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


#elif defined USAGE_TEST
#include <common_define.h>
#include "upgrade.h"
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


#elif defined LOG_SERVER_TEST
#include "common_define.h"
#include "upgrade.h"
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


#elif defined SLO_TEST

#include "common_define.h"
#include "upgrade.h"
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

#elif defined LOCK_TEST
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
