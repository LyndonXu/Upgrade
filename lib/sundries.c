/****************************************************************************
 * Copyright(c), 2001-2060, ******************************* 版权所有
 ****************************************************************************
 * 文件名称             : sundries.c
 * 版本                 : 0.0.1
 * 作者                 : 许龙杰
 * 创建日期             : 2014年5月7日
 * 描述                 : 
 ****************************************************************************/
#include "common_define.h"

#include <net/if.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <linux/sockios.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "upgrade.h"

static int32_t IsADir(const char *pPathName)
{
	int32_t s32Tmp1 = -1, s32Tmp2 = -1, s32Tmp3 = -1, s32Tmp4 = -1;

	struct stat stStat =
	{ 0 };

	(void) s32Tmp1;
	(void) s32Tmp2;
	(void) s32Tmp3;
	(void) s32Tmp4;

	if (NULL == pPathName)
	{
		return MY_ERR(_Err_NULLPtr);
	}

	if (lstat(pPathName, &stStat) < 0)
	{
		return MY_ERR(_Err_SYS + errno);
	}

	if (0 == S_ISDIR(stStat.st_mode))
	{
		PRINT("the path is: %s", pPathName);
		return MY_ERR(_Err_NotADir);
	}
	return 0;
}

/*
 * 函数名      : TranversaDir
 * 功能        : 遍历目录
 * 参数        : pPathName [in] (const char * 类型): 要查询的目录
 *             : boIsRecursion [in] (bool类型): 是否递归子目录
 *             : pFunCallback [in] (PFUN_TranversaDir_Callback类型): 详见类型定义
 *             : pContext [in] (void *类型): 回调函数的上下文指针
 * 返回        : 正确返回0, 错误返回错误码
 * 作者        : 许龙杰
 */
int32_t TranversaDir(const char *pPathName, bool boIsRecursion,
		PFUN_TranversaDir_Callback pFunCallback, void *pContext)
{
	DIR *pDirHandle = NULL;
	struct dirent stDirent = { 0 }, *pDirent = NULL;
	int32_t s32Err = 0;

	if (NULL == pPathName)
	{
		return MY_ERR(_Err_NULLPtr);
	}

	s32Err = IsADir(pPathName);
	if (s32Err != 0)
	{
		return s32Err;
	}

	pDirHandle = opendir(pPathName);
	if (NULL == pDirHandle)
	{
		return MY_ERR(_Err_SYS + errno);
	}
	while (NULL != (pDirent = readdir(pDirHandle)))
	{
		stDirent = *pDirent;
		s32Err = 0;
		if ((strcmp(stDirent.d_name, ".")) == 0)
		{
			continue;
		}
		if ((strcmp(stDirent.d_name, "..")) == 0)
		{
			continue;
		}
		if (pFunCallback != NULL)
		{
			if ((s32Err = pFunCallback(pPathName, &stDirent, pContext)) != 0)
			{
				return s32Err;
			}
		}
		if (((stDirent.d_type & DT_DIR) != 0) && boIsRecursion)
		{
			char *pPathInner = (char *) malloc(_POSIX_PATH_MAX + 1);
			if (NULL == pPathInner)
			{
				return MY_ERR(_Err_Mem);
			}
			strcpy(pPathInner, pPathName);
			if (pPathInner[strlen(pPathInner) - 1] != '/')
			{
				PRINT("%s\n", pPathInner);
				strcat(pPathInner, "/");
			}
			strcat(pPathInner, stDirent.d_name);
			TranversaDir(pPathInner, boIsRecursion, pFunCallback, pContext);
			free(pPathInner);
		}
	}
	closedir(pDirHandle);
	return 0;
}



#define	 MX (((z >> 5) ^ (y << 2)) + (((y >> 3) ^ (z << 4)) ^ (sum ^ y)) + (k[(p & 3) ^ e] ^ z))


/*
 * 函数名      : btea
 * 功能        : XXTEA加密/解密
 * 参数        : v [in/out] (int32_t * 类型): 需要加密/解密的数据指针
 *             : n [in] (int32_t 类型): 需要加密/解密的数据长度，正值表示加密，负值表示解密
 *             : k [in] (int32_t *): 128位的Key值
 * 返回        : 正确返回0, 错误返回错误码
 * 作者        : 许龙杰
 */
int32_t btea(int32_t *v, int32_t n, int32_t *k)
{
	uint32_t z = v[n - 1], y = v[0], sum = 0, e, DELTA = 0x9e3779b9;
	int32_t p, q;
	if (n > 1)
	{ /* Coding Part */
		q = 6 + 52 / n;
		while (q-- > 0)
		{
			sum += DELTA;
			e = (sum >> 2) & 3;
			for (p = 0; p < n - 1; p++)
			{
				y = v[p + 1];
				v[p] += MX;
				z = v[p];
			}
			y = v[0];
			v[n - 1] += MX;
			z = v[n - 1];
		}
		return 0;
	}
	else if (n < -1)
	{ /* Decoding Part */
		n = -n;
		q = 6 + 52 / n;
		sum = q * DELTA;
		while (sum != 0)
		{
			e = (sum >> 2) & 3;
			for (p = n - 1; p > 0; p--)
			{
				z = v[p - 1];
				v[p] -= MX;
				y = v[p];
			}
			z = v[n - 1];
			v[0] -= MX;
			y = v[0];
			sum -= DELTA;
		}
		return 0;
	}
	return 1;
}


/* get the check sum */
int32_t CheckSum(int32_t *pData, uint32_t u32Cnt)
{
	uint32_t i;
	int32_t s32CheckSum = 0;
	for (i = 0; i < u32Cnt; i++)
	{
		s32CheckSum += pData[i];
	}
	s32CheckSum = ~s32CheckSum;
	s32CheckSum -= 1;
	return s32CheckSum;
}

/*
 * 函数名      : GetIPV4Addr
 * 功能        : 得到支持IPV4的已经连接的网卡的信息
 * 参数        : pAddrOut [out] (StIPV4Addr * 类型): 详见定义
 *             : pCnt [in/out] (uint32_t * 类型): 作为输入指明pAddrOut数组的数量，
 *               作为输出指明查询到的可用网卡的数量
 * 返回        : 正确返回0, 错误返回错误码
 * 作者        : 许龙杰
 */
int32_t GetIPV4Addr(StIPV4Addr *pAddrOut, uint32_t *pCnt)
{
#if 1
	if ((pAddrOut == NULL) || (pCnt == NULL))
	{
		return MY_ERR(_Err_InvalidParam);
	}
	{
	uint32_t u32Cnt = 0, i, u32NetworkCnt = 0;
	char c8NewWork[16][16];
	{
	FILE *pFile;
	char c8Buf[256];
	c8Buf[0] = 0;

	pFile = fopen("/proc/net/dev", "rb");
	if (pFile == NULL)
	{
		return MY_ERR(_Err_SYS + errno);
	}
	fgets(c8Buf, 256, pFile);
	fgets(c8Buf, 256, pFile);
	while (fgets(c8Buf, 256, pFile) != NULL)
	{
		char *pTmp = strchr(c8Buf, ':');
		if (pTmp == NULL)
		{
			continue;
		}
		*pTmp = 0;
		sscanf(c8Buf, "%s", c8NewWork[u32Cnt]);
		PRINT("network: %s\n", c8NewWork[u32Cnt]);
		u32Cnt++;
		if (u32Cnt >= 16)
		{
			break;
		}
	}
	fclose(pFile);
	}
	for (i = 0; i < u32Cnt; i++)
	{
		struct ifreq stIfreq;
		int32_t s32FD;
		s32FD = socket(AF_INET, SOCK_DGRAM, 0);
		if (s32FD < 0)
		{
			continue;
		}

		strncpy(stIfreq.ifr_name, c8NewWork[i], sizeof(stIfreq.ifr_name));
		stIfreq.ifr_addr.sa_family = AF_INET;
		if (ioctl(s32FD, SIOCGIFADDR, &stIfreq) == -1)
		{
			close(s32FD);
			continue;
		}
		close(s32FD);
		{
		int32_t s32Rslt = getnameinfo(&stIfreq.ifr_addr, sizeof(struct sockaddr),
				pAddrOut[u32NetworkCnt].c8IPAddr, 16, NULL,	0, NI_NUMERICHOST);
		if (s32Rslt != 0)
		{
			PRINT("getnameinfo error: %s\n", strerror(errno));
			continue;
		}
		}
		strncpy(pAddrOut[u32NetworkCnt].c8Name, c8NewWork[i], 16);
		PRINT("network :%s address: %s\n", c8NewWork[i], pAddrOut[u32NetworkCnt].c8IPAddr);
		u32NetworkCnt++;
		if (u32NetworkCnt >= (*pCnt))
		{
			return 0;
		}
	}
	*pCnt = u32NetworkCnt;
	return 0;
	}


#else	/* mips dos't supported */
#include <ifaddrs.h>
	struct ifaddrs *pInterfaceAddr, *pI;
	int32_t s32Family;
	uint32_t u32NetworkCnt = 0;
	char c8Host[16];

	if ((pAddrOut == NULL) || (pCnt == NULL))
	{
		return MY_ERR(_Err_InvalidParam);
	}
	if (getifaddrs(&pInterfaceAddr) == -1)
	{
		PRINT("getifaddrs error: %s\n", strerror(errno));
		return MY_ERR(_Err_SYS + errno);
	}

	/* Walk through linked list, maintaining head pointer so we
	 can free list later */

	for (pI = pInterfaceAddr; pI != NULL; pI = pI->ifa_next)
	{
		if (pI->ifa_addr == NULL)
			continue;

		s32Family = pI->ifa_addr->sa_family;

		if (s32Family == AF_INET )
		{
			int32_t s32Rslt = getnameinfo(pI->ifa_addr, sizeof(struct sockaddr_in), c8Host, 16, NULL,
					0, NI_NUMERICHOST);
			if (s32Rslt != 0)
			{
				PRINT("getnameinfo error: %s\n", strerror(errno));
				freeifaddrs(pInterfaceAddr);
				return MY_ERR(_Err_SYS + errno);
			}
			PRINT("network :%s address: %s\n", pI->ifa_name, c8Host);
			strncpy(pAddrOut[u32NetworkCnt].c8Name, pI->ifa_name, 16);
			strncpy(pAddrOut[u32NetworkCnt].c8IPAddr, c8Host, 16);
			u32NetworkCnt++;
			if (u32NetworkCnt >= (*pCnt))
			{
				freeifaddrs(pInterfaceAddr);
				return 0;
			}
		}
	}
	*pCnt = u32NetworkCnt;
	freeifaddrs(pInterfaceAddr);
#endif
	return 0;
}

