/****************************************************************************
 * Copyright(c), 2001-2060, ******************************* 版权所有
 ****************************************************************************
 * 文件名称             : cloud_thread.c
 * 版本                 : 0.0.1
 * 作者                 : 许龙杰
 * 创建日期             : 2014年5月16日
 * 描述                 : 
 ****************************************************************************/
#include "common_define.h"
#include "upgrade.h"

#define CLOUD_KEEPALIVE_TIME		30
#define CLOUD_UPDATE_TIME			(24 * 3600)

const char c_c8LanName[] = "p8p1"; /* <TODO> */
const char c_c8WifiName[] = "wlp2s0"; /* <TODO> */


void *ThreadCloud(void *pArg)
{
	int32_t s32Err = 0;
	int32_t s32CloudHandle = 0;

	int32_t s32CloudServer = -1;

	uint32_t u32KeepAliveTime = time(NULL);
	uint32_t u32UpDateTime = 0;
	uint32_t u32SysTime = u32KeepAliveTime;

	bool boIsCloudOnline = false;

	/* <TODO> I think I should get the key, ID and URL from other process or server */
	char c8ServerURL[256] = "www.jiuan.com";
	char c8ID[PRODUCT_ID_CNT] = "0123456789012345";
	char c8Key[XXTEA_KEY_CNT_CHAR];


	StCloudStat stStat = {_Cloud_IsOnline, };
	strcpy(stStat.c8ServerURL, c8ServerURL);


	s32CloudServer = ServerListen(CLOUD_SOCKET);
	if (s32CloudServer < 0)
	{
		PRINT("ServerListen error: 0x%08x", s32CloudServer);
		return NULL;
	}

	s32CloudHandle = CloudInit(&s32Err);
	if (s32CloudHandle == 0)
	{
		PRINT("CloudInit error: 0x%08x", s32Err);
		ServerRemove(s32CloudServer, CLOUD_SOCKET);
		return NULL;
	}

	{
	StCloudStat stStatTmp = {_Cloud_IsNotOnline};
	CloudGetStat(s32CloudHandle, &stStatTmp);
	if (stStatTmp.emStat == _Cloud_IsOnline)
	{
		stStat = stStatTmp;
		PRINT("cloud is on line IP %s, URL %s\n", stStat.c8ClientIPV4, stStat.c8ServerURL);
		boIsCloudOnline = true;
	}
	}

	while (!g_boIsExit)
	{
		fd_set stSet;
		struct timeval stTimeout;

		stTimeout.tv_sec = 1;
		stTimeout.tv_usec = 0;
		FD_ZERO(&stSet);
		FD_SET(s32CloudServer, &stSet);

		if (select(s32CloudServer + 1, &stSet, NULL, NULL, &stTimeout) > 0)
		{
			int32_t s32Client = -1;
			if (!FD_ISSET(s32CloudServer, &stSet))
			{
				goto next;
			}
			s32Client = ServerAccept(s32CloudServer);
			if (!boIsCloudOnline)
			{
				close(s32Client);
				goto next;
			}
		}
        /* time out */
next:
		if (!boIsCloudOnline) /* cloud in not online, authenticate */
		{
			StIPV4Addr stIPV4Addr[4];
			uint32_t u32Cnt = 4;
			int32_t s32LanAddr = -1, s32WifiAddr = -1;
			uint32_t i;

			/* list the network */
			s32Err = GetIPV4Addr(stIPV4Addr, &u32Cnt);
			for (i = 0; i < u32Cnt; i++)
			{
				if (strcmp(stIPV4Addr[i].c8Name, c_c8LanName) == 0)
				{
					s32LanAddr = i; /* well, the lan is connected */
				}
				else if (strcmp(stIPV4Addr[i].c8Name, c_c8WifiName) == 0)
				{
					s32WifiAddr = i; /* WIFI is connected */
				}
			}

			if (s32LanAddr != -1)
			{
				memcpy(stStat.c8ClientIPV4, stIPV4Addr[s32LanAddr].c8IPAddr, 16);
				s32Err = CloudAuthentication(&stStat, c8ID, c8Key);
				if (s32Err != 0)
				{
					/* oh no, I can't through the authentication with the cloud */
					PRINT("CloudAuthentication error: 0x%08x\n", s32Err);
				}
				else
				{
					CloudSetStat(s32CloudHandle, &stStat);
					PrintLog("IP %s through the authentication of the cloud %s\n", stStat.c8ClientIPV4, stStat.c8ServerURL);
					boIsCloudOnline = true;
					u32KeepAliveTime = u32SysTime;
				}
			}
			if ((!boIsCloudOnline) && (s32WifiAddr != -1))
			{
				memcpy(stStat.c8ClientIPV4, stIPV4Addr[s32WifiAddr].c8IPAddr, 16);
				s32Err = CloudAuthentication(&stStat, c8ID, c8Key);
				if (s32Err != 0)
				{
					/* oh no, I can't through the authentication with the cloud */
					PRINT("CloudAuthentication error: 0x%08x\n", s32Err);
				}
				else
				{
					CloudSetStat(s32CloudHandle, &stStat);
					PrintLog("IP %s through the authentication of the cloud %s\n", stStat.c8ClientIPV4, stStat.c8ServerURL);
					boIsCloudOnline = true;
					u32KeepAliveTime = u32SysTime;
				}
			}
		}

		/* keep alive <TODO> I don't know whether other normal communication should be treated as a keep alive */
		if ((boIsCloudOnline) && ((u32SysTime - u32KeepAliveTime) > CLOUD_KEEPALIVE_TIME))
		{
			s32Err = CloudKeepAlive(&stStat, c8ID);
			if (s32Err != 0)
			{
				PRINT("CloudKeepAlive error: 0x%08x\n", s32Err);
				PrintLog("cloud keep alive error: 0x%08x\n", s32Err);

				stStat.emStat = _Cloud_IsNotOnline;
				CloudSetStat(s32CloudHandle, &stStat);
				stStat.emStat = _Cloud_IsOnline;
				boIsCloudOnline = false;
			}
			else
			{
				PRINT("CloudKeepAlive OK\n");
				u32KeepAliveTime = u32SysTime;
			}
		}

		/* update */
		if ((boIsCloudOnline) && ((u32SysTime - u32UpDateTime) > CLOUD_UPDATE_TIME))
		{
			/* <TODO> ...... */
		}

		if (boIsCloudOnline)
		{
			/* <TODO> check whether there is that a new MCU module has been added ...... */
		}

		u32SysTime = time(NULL);
	}

	ServerRemove(s32CloudServer, CLOUD_SOCKET);
	CloudDestroy(s32CloudHandle);
	return NULL;
}

