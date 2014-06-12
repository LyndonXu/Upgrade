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

#if HAS_CROSS
const char c_c8LanName[] = "br0"; /* <TODO> */
const char c_c8WifiName[] = "wlp2s0"; /* <TODO> */
#else
const char c_c8LanName[] = "p8p1";
const char c_c8WifiName[] = "wlp2s0";
#endif

static int32_t s_s32CloudHandle = 0;
static uint32_t s_u32KeepAliveTime = 0;
static uint32_t s_u32UpDateTime = 0;
static uint32_t s_u32SysTime = 0;

static bool s_boIsCloudOnline = false;

/* <TODO> I think I should get the key, ID and URL from other process or server */
static char s_c8ServerDomain [64] = COORDINATION_DOMAINA;
static char s_c8GatewayRegion [32] = {0};
static char s_c8ID [PRODUCT_ID_CNT] = "0123456789012345";
static char s_c8Key [XXTEA_KEY_CNT_CHAR];

#if 1

#define CUE_THREAD_CNT		3

void *ThreadError(void *pArg)
{
	int32_t s32CloudServer = ServerListen(CLOUD_SOCKET);
	if (s32CloudServer < 0)
	{
		PRINT("ServerListen error: 0x%08x", s32CloudServer);
		return NULL;
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
				continue;
			}
			s32Client = ServerAccept(s32CloudServer);
			if (!s_boIsCloudOnline)
			{
				close(s32Client);
			}
			else
			{
				/* create detach process thread */
			}
		}
		s_u32SysTime = time(NULL);
	}
	ServerRemove(s32CloudServer, CLOUD_SOCKET);
	return NULL;
}

/*
 * why we should open a socket reading and writing it abidingly? think that, we send some message via a socket
 * and then close it. normally, we bind a port randomly(allocated by the system), after that we send a message
 * to a server, the server will get our IP address and port, and the server will echo a message via this
 * address:port, but unfortunately, we have closed the channel, how we can get the echo?
 *
 *
 */
void *ThreadKeepAlive(void *pArg)
{
	StCloudDomain stStat = {{_Cloud_IsOnline}};
	char c8Domain[64];
	int32_t s32Socket = -1;

	s32Socket = socket(AF_INET, SOCK_DGRAM, 0);
	if (s32Socket < 0)
	{
		PrintLog("socket error: %s\n", strerror(errno));
		PRINT("socket error: %s\n", strerror(errno));
		return NULL;
	}
	/*
	 * why we don't bind the socket to the local IP address?
	 * because we don't know whether the gateway has passed the authentication
	 */

	while (!g_boIsExit)
	{
		if (s_boIsCloudOnline)
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
				socklen_t s32Len = 0;
				s32RecvLen = recvfrom(s32Socket, NULL, -1, 0, (struct sockaddr *)(&stRecvAddr), &s32Len);
				if (s32RecvLen <= 0 || (s32Len != sizeof (struct sockaddr_in)))
				{
					goto next;
				}
				}
				/* compare stRecvAddr with stServerAddr to check whether the message is mine */
				/* get the domain and port from the database */
				if (CloudGetDomainFromRegion(s_s32CloudHandle, _Region_UDP,
						s_c8GatewayRegion , c8Domain, sizeof(c8Domain)) != 0)
				{
					goto next;
				}
				/* translate the string */
				GetDomainPortFromString(c8Domain, stStat.c8Domain, 64, &(stStat.s32Port));
				{
				struct sockaddr_in stServerAddr = {0};
				stServerAddr.sin_family = AF_INET;
				stServerAddr.sin_port = htons(stStat.s32Port);
				/* get the IPV4 address of host via DNS(in this application, the host just has one IP address)  */
				if (GetHostIPV4Addr(stStat.c8Domain, NULL, &(stServerAddr.sin_addr)) != 0)
				{
					goto next;
				}
				else if(memcmp(&stServerAddr, &stRecvAddr, sizeof(struct sockaddr)) == 0)
				{
					/* well, now, I get a right message */
				}
				}
			}
next:
			if ((s_u32SysTime - s_u32KeepAliveTime) > CLOUD_KEEPALIVE_TIME)
			{
				int32_t s32Err = CloudGetDomainFromRegion(s_s32CloudHandle, _Region_UDP,
						s_c8GatewayRegion , c8Domain, sizeof(c8Domain));
				if (s32Err != 0)
				{
					continue;
				}
				GetDomainPortFromString(c8Domain, stStat.c8Domain, 64, &(stStat.s32Port));

				{
				struct sockaddr_in stServerAddr = {0};
				stServerAddr.sin_family = AF_INET;
				stServerAddr.sin_port = htons(stStat.s32Port);
				s32Err = GetHostIPV4Addr(stStat.c8Domain, NULL, &(stServerAddr.sin_addr));
				if (s32Err != 0)
				{
					continue;
				}
				else
				{
					/* every time we send, we should check whether the local IP address is changed */
					s32Err = CloudGetStat(s_s32CloudHandle, &(stStat.stStat));
					if (s32Err != 0)
					{
						continue;
					}
					else
					{
						struct sockaddr stBindAddr = {0};
						struct sockaddr_in *pTmpAddr = (void *)(&stBindAddr);
						socklen_t s32Len = sizeof(struct sockaddr);
						/* get the current address to which the socket socket is bound */
						s32Err = getsockname(s32Socket, &stBindAddr, &s32Len);
						if (s32Err != 0)
						{
							/* what happened */
							continue;
						}
						else
						{
							in_addr_t u32LocalInternetAddr;
							u32LocalInternetAddr = htonl(INADDR_ANY);
							if (u32LocalInternetAddr != pTmpAddr->sin_addr.s_addr)
							{
								/*
								 * we find that the address has changed, we should re-bind the socket, but very sorry,
								 * there is no right system API can finish this directly, so, we close the socket
								 * and open a new socket, then, bind it
								 */
								close(s32Socket);
								s32Socket = socket(AF_INET, SOCK_DGRAM, 0);
								if (s32Socket < 0)
								{
									PrintLog("socket error: %s\n", strerror(errno));
									PRINT("socket error: %s\n", strerror(errno));
									return NULL;
								}
								pTmpAddr->sin_family = AF_INET;
								pTmpAddr->sin_port = htons(0);
								pTmpAddr->sin_addr.s_addr = u32LocalInternetAddr;
								if (bind(s32Socket, &stBindAddr, sizeof(struct sockaddr)))
								{
									PRINT("bind error: %s\n", strerror(errno));
									PrintLog("bind error: %s\n", strerror(errno));
									continue;
								}
							}
						}
					}
					/* I don't think we should check the return value */
					sendto(s32Socket, NULL, -1, MSG_NOSIGNAL, (struct sockaddr *)(&stServerAddr), sizeof(struct sockaddr));
					PRINT("CloudKeepAlive OK\n");
					s_u32KeepAliveTime = s_u32SysTime;
				}
				}
			}
		}
		else
		{
			sleep(1);
		}
#if 0
		sleep(1);
		if ((s_boIsCloudOnline) && ((s_u32SysTime - s_u32KeepAliveTime) > CLOUD_KEEPALIVE_TIME))
		{
			int32_t s32Err = CloudGetDomainFromRegion(s_s32CloudHandle, Region_HTTPS,
					s_c8GatewayRegion , c8Domain, sizeof(c8Domain));
			if (s32Err != 0)
			{
				continue;
			}
			GetDomainPortFromString(c8Domain, stStat.c8Domain, 64, &(stStat.s32Port));
			s32Err = CloudKeepAlive(&stStat, s_c8ID );
			if (s32Err != 0)
			{
				PRINT("CloudKeepAlive error: 0x%08x\n", s32Err);
				PrintLog("cloud keep alive error: 0x%08x\n", s32Err);

				stStat.stStat.emStat = _Cloud_IsNotOnline;
				CloudSetStat(s_s32CloudHandle, &stStat.stStat);
				stStat.stStat.emStat = _Cloud_IsOnline;
				s_boIsCloudOnline = false;
			}
			else
			{
				PRINT("CloudKeepAlive OK\n");
				s_u32KeepAliveTime = s_u32SysTime;
			}
		}
#endif
	}

	close(s32Socket);
	return NULL;
}

void *ThreadUpdate(void *pArg)
{
	//StCloudDomain stStat = {{_Cloud_IsOnline}};
	//char c8Domain[64];


	while (!g_boIsExit)
	{
		sleep(1);
		if ((s_boIsCloudOnline) && ((s_u32SysTime - s_u32UpDateTime) > CLOUD_UPDATE_TIME))
		{
			/* <TODO> ...... */
		}

		if (s_boIsCloudOnline)
		{
			/* <TODO> check whether there is that a new MCU module has been added ...... */
		}
	}
	return NULL;
}

const PFUN_THREAD c_pFunThreadCue[CUE_THREAD_CNT] =
{
	ThreadError, ThreadKeepAlive, ThreadUpdate,
};

void *ThreadCloud(void *pArg)
{
	int32_t s32Err = 0;

	pthread_t s32TidCloud[CUE_THREAD_CNT] = {0};

	bool boIsCoordination = false;

	s_u32SysTime = time(NULL);

	StCloudDomain stStat = {{_Cloud_IsOnline}};
	s_c8GatewayRegion[0] = 0;
	GetRegionOfGateway(s_c8GatewayRegion , 32);
	PRINT("region of gateway is: %s\n", s_c8GatewayRegion );

	s_s32CloudHandle = CloudInit(&s32Err);
	if (s_s32CloudHandle == 0)
	{
		PRINT("CloudInit error: 0x%08x", s32Err);
		return NULL;
	}

	{
	StCloudDomain stStatTmp = {{_Cloud_IsNotOnline}};
	CloudGetStat(s_s32CloudHandle, &stStatTmp.stStat);
	if (stStatTmp.stStat.emStat == _Cloud_IsOnline)
	{
		stStat = stStatTmp;
		PRINT("cloud is on line IP %s\n", stStat.stStat.c8ClientIPV4);
		s_boIsCloudOnline = true;
	}
	s32Err = CloudGetDomainFromRegion(s_s32CloudHandle, _Region_HTTPS,
			s_c8GatewayRegion , s_c8ServerDomain , sizeof(s_c8ServerDomain ));
	if (s32Err == 0)
	{
		boIsCoordination = true;
	}

	}

	GetDomainPortFromString(s_c8ServerDomain , stStat.c8Domain, 64, &(stStat.s32Port));
	PRINT("Domain: %s, Port: %d\n", stStat.c8Domain, stStat.s32Port);

	{
	int32_t i;
	for (i = 0; i < CUE_THREAD_CNT; i++)
	{
		MakeThread(c_pFunThreadCue[i], NULL, false, s32TidCloud + i, false);
	}
	}

	while (!g_boIsExit)
	{
		sleep(1);
		if (!s_boIsCloudOnline) /* cloud in not online, authenticate */
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
				memcpy(stStat.stStat.c8ClientIPV4, stIPV4Addr[s32LanAddr].c8IPAddr, IPV4_ADDR_LENGTH);
				s32Err = CloudAuthentication(&stStat, boIsCoordination, s_c8ID , s_c8Key );
				if (s32Err != 0)
				{
					/* oh no, I can't through the authentication with the cloud */
					PRINT("CloudAuthentication error: 0x%08x\n", s32Err);
				}
				else
				{
					CloudSetStat(s_s32CloudHandle, &stStat.stStat);
					PrintLog("IP %s through the authentication of the cloud %s\n", stStat.stStat.c8ClientIPV4, stStat.c8Domain);
					s_boIsCloudOnline = true;
					s_u32KeepAliveTime = s_u32SysTime;
				}
			}
			if ((!s_boIsCloudOnline) && (s32WifiAddr != -1))
			{
				memcpy(stStat.stStat.c8ClientIPV4, stIPV4Addr[s32WifiAddr].c8IPAddr, IPV4_ADDR_LENGTH);
				s32Err = CloudAuthentication(&stStat, boIsCoordination, s_c8ID , s_c8Key );
				if (s32Err != 0)
				{
					/* oh no, I can't through the authentication with the cloud */
					PRINT("CloudAuthentication error: 0x%08x\n", s32Err);
				}
				else
				{
					CloudSetStat(s_s32CloudHandle, &stStat.stStat);
					PrintLog("IP %s through the authentication of the cloud %s\n", stStat.stStat.c8ClientIPV4, stStat.c8Domain);
					s_boIsCloudOnline = true;
					s_u32KeepAliveTime = s_u32SysTime;
				}
			}
		}
	}

	{
	int32_t i;
	for (i = 0; i < CUE_THREAD_CNT; i++)
	{
		pthread_join(s32TidCloud[i], NULL);
	}
	}

	CloudDestroy(s_s32CloudHandle);
	return NULL;
}
#else

void *ThreadCloud(void *pArg)
{
	int32_t s32Err = 0;

	int32_t s32CloudServer = -1;
	bool boIsCoordination = false;

	s_u32SysTime = time(NULL);

	StCloudDomain stStat = {{_Cloud_IsOnline}};
	s_c8GatewayRegion [0] = 0;
	GetRegionOfGateway(s_c8GatewayRegion , 32);
	PRINT("region of gateway is: %s\n", s_c8GatewayRegion );

	s32CloudServer = ServerListen(CLOUD_SOCKET);
	if (s32CloudServer < 0)
	{
		PRINT("ServerListen error: 0x%08x", s32CloudServer);
		return NULL;
	}

	s_s32CloudHandle = CloudInit(&s32Err);
	if (s_s32CloudHandle == 0)
	{
		PRINT("CloudInit error: 0x%08x", s32Err);
		ServerRemove(s32CloudServer, CLOUD_SOCKET);
		return NULL;
	}

	{
	StCloudDomain stStatTmp = {{_Cloud_IsNotOnline}};
	CloudGetStat(s_s32CloudHandle, &stStatTmp.stStat);
	if (stStatTmp.stStat.emStat == _Cloud_IsOnline)
	{
		stStat = stStatTmp;
		PRINT("cloud is on line IP %s\n", stStat.stStat.c8ClientIPV4);
		s_boIsCloudOnline = true;
	}
	s32Err = CloudGetDomainFromRegion(s_s32CloudHandle, Region_HTTPS,
			s_c8GatewayRegion , s_c8ServerDomain , sizeof(s_c8ServerDomain ));
	if (s32Err == 0)
	{
		boIsCoordination = true;
	}

	}

	GetDomainPortFromString(s_c8ServerDomain , stStat.c8Domain, 64, &(stStat.s32Port));
	PRINT("Domain: %s, Port: %d\n", stStat.c8Domain, stStat.s32Port);

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
			if (!s_boIsCloudOnline)
			{
				close(s32Client);
				goto next;
			}
		}
        /* time out */
next:
		if (!s_boIsCloudOnline) /* cloud in not online, authenticate */
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
				memcpy(stStat.stStat.c8ClientIPV4, stIPV4Addr[s32LanAddr].c8IPAddr, IPV4_ADDR_LENGTH);
				s32Err = CloudAuthentication(&stStat, boIsCoordination, s_c8ID , s_c8Key );
				if (s32Err != 0)
				{
					/* oh no, I can't through the authentication with the cloud */
					PRINT("CloudAuthentication error: 0x%08x\n", s32Err);
				}
				else
				{
					CloudSetStat(s_s32CloudHandle, &stStat.stStat);
					PrintLog("IP %s through the authentication of the cloud %s\n", stStat.stStat.c8ClientIPV4, stStat.c8Domain);
					s_boIsCloudOnline = true;
					s_u32KeepAliveTime = s_u32SysTime;
				}
			}
			if ((!s_boIsCloudOnline) && (s32WifiAddr != -1))
			{
				memcpy(stStat.stStat.c8ClientIPV4, stIPV4Addr[s32WifiAddr].c8IPAddr, IPV4_ADDR_LENGTH);
				s32Err = CloudAuthentication(&stStat, boIsCoordination, s_c8ID , s_c8Key );
				if (s32Err != 0)
				{
					/* oh no, I can't through the authentication with the cloud */
					PRINT("CloudAuthentication error: 0x%08x\n", s32Err);
				}
				else
				{
					CloudSetStat(s_s32CloudHandle, &stStat.stStat);
					PrintLog("IP %s through the authentication of the cloud %s\n", stStat.stStat.c8ClientIPV4, stStat.c8Domain);
					s_boIsCloudOnline = true;
					s_u32KeepAliveTime = s_u32SysTime;
				}
			}
		}
		{
		char c8Domain[64];
		if ((s_boIsCloudOnline) && ((s_u32SysTime - s_u32KeepAliveTime) > CLOUD_KEEPALIVE_TIME))
		{
			int32_t s32Err = CloudGetDomainFromRegion(s_s32CloudHandle, Region_HTTPS,
					s_c8GatewayRegion , c8Domain, sizeof(c8Domain));
			if (s32Err != 0)
			{
				continue;
			}
			GetDomainPortFromString(c8Domain, stStat.c8Domain, 64, &(stStat.s32Port));
			s32Err = CloudKeepAlive(&stStat, s_c8ID );
			if (s32Err != 0)
			{
				PRINT("CloudKeepAlive error: 0x%08x\n", s32Err);
				PrintLog("cloud keep alive error: 0x%08x\n", s32Err);

				stStat.stStat.emStat = _Cloud_IsNotOnline;
				CloudSetStat(s_s32CloudHandle, &stStat.stStat);
				stStat.stStat.emStat = _Cloud_IsOnline;
				s_boIsCloudOnline = false;
			}
			else
			{
				PRINT("CloudKeepAlive OK\n");
				s_u32KeepAliveTime = s_u32SysTime;
			}
		}
		}

		/* update */
		if ((s_boIsCloudOnline) && ((s_u32SysTime - s_u32UpDateTime) > CLOUD_UPDATE_TIME))
		{
			/* <TODO> ...... */
		}

		if (s_boIsCloudOnline)
		{
			/* <TODO> check whether there is that a new MCU module has been added ...... */
		}

		s_u32SysTime = time(NULL);
	}

	ServerRemove(s32CloudServer, CLOUD_SOCKET);
	CloudDestroy(s_s32CloudHandle);
	return NULL;
}
#endif
