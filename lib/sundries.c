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
 * 函数名      : TraversalDir
 * 功能        : 遍历目录
 * 参数        : pPathName [in] (const char * 类型): 要查询的目录
 *             : boIsRecursion [in] (bool类型): 是否递归子目录
 *             : pFunCallback [in] (PFUN_TraversalDir_Callback类型): 详见类型定义
 *             : pContext [in] (void *类型): 回调函数的上下文指针
 * 返回        : 正确返回0, 错误返回错误码
 * 作者        : 许龙杰
 */
int32_t TraversalDir(const char *pPathName, bool boIsRecursion,
		PFUN_TraversalDir_Callback pFunCallback, void *pContext)
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
			TraversalDir(pPathInner, boIsRecursion, pFunCallback, pContext);
			free(pPathInner);
		}
	}
	closedir(pDirHandle);
	return 0;
}



#define	 MX ((((z >> 5) ^ (y << 2)) + ((y >> 3) ^ (z << 4))) ^ ((sum ^ y) + (k[(p & 3) ^ e] ^ z)))
//#define MX		(((z >> 5 ^ y << 2) + (y >> 3 ^ z << 4)) ^ ((sum ^ y) + (k[(p & 3) ^ e] ^ z)))

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
		strncpy(pAddrOut[u32NetworkCnt].c8Name, c8NewWork[i], IPV4_ADDR_LENGTH);
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
			strncpy(pAddrOut[u32NetworkCnt].c8IPAddr, c8Host, IPV4_ADDR_LENGTH);
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

/*
 * 函数名      : GetHostIPV4Addr
 * 功能        : 解析域名或者IPV4的IPV4地址
 * 参数        : pHost [in] (const char * 类型): 详见定义
 *             : c8IPV4Addr [in/out] (char [IPV4_ADDR_LENGTH] 类型): 用于保存IP地址
 *             : pInternetAddr [in/out] (struct in_addr * 类型): 用户保存Internet类型地址
 * 返回        : 正确返回0, 错误返回错误码
 * 作者        : 许龙杰
 */
int32_t GetHostIPV4Addr(const char *pHost, char c8IPV4Addr[IPV4_ADDR_LENGTH], struct in_addr *pInternetAddr)
{
	if (pHost == NULL)
	{
		return MY_ERR(_Err_InvalidParam);
	}
	else
	{
		struct addrinfo stFilter = {0};
		struct addrinfo *pRslt = NULL;
		int32_t s32Err = 0;
		stFilter.ai_family = AF_INET;
		stFilter.ai_socktype = SOCK_STREAM;
		s32Err = getaddrinfo(pHost, NULL, &stFilter, &pRslt);
		if (s32Err == 0)
		{
			struct sockaddr_in *pTmp =  (struct sockaddr_in *)(pRslt->ai_addr);
			PRINT("server ip: %s\n", inet_ntoa(pTmp->sin_addr));
			if (pInternetAddr != NULL)
			{
				*pInternetAddr = pTmp->sin_addr;
			}
			if (c8IPV4Addr != NULL)
			{
				inet_ntop(AF_INET, (void *)(&(pTmp->sin_addr)), c8IPV4Addr, IPV4_ADDR_LENGTH);
				PRINT("after inet_ntoa: %s\n", c8IPV4Addr);		/* just get the first address */
			}
			freeaddrinfo(pRslt);
			return 0;
		}
		else
		{
			PRINT("getaddrinfo s32Err = %d, error: %s\n", s32Err, gai_strerror(s32Err));
			s32Err = 0 - s32Err;
			return MY_ERR(_Err_Unkown_Host + s32Err);
		}
	}


}

/*
 * 函数名      : TimeGetTime
 * 功能        : 得到当前系统时间 (MS级)
 * 参数        : 无
 * 返回值      : 当前系统时间ms
 * 作者        : 许龙杰
 */
uint64_t TimeGetTime(void)
{
    struct timeval stTime;

    gettimeofday(&stTime, NULL);

    uint64_t u64Time;
    u64Time = stTime.tv_sec;
    u64Time *= 1000; /* ms */
    u64Time += (stTime.tv_usec / 1000);
    return u64Time;
}


#if 0

void CreateCRC32Table(void)
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
}

#endif

static uint32_t s_u32CRC32Table[] = {
	0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F, 0xE963A535, 0x9E6495A3,
	0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988, 0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91,
	0x1DB71064, 0x6AB020F2, 0xF3B97148, 0x84BE41DE, 0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
	0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC, 0x14015C4F, 0x63066CD9, 0xFA0F3D63, 0x8D080DF5,
	0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172, 0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B,
	0x35B5A8FA, 0x42B2986C, 0xDBBBC9D6, 0xACBCF940, 0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
	0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116, 0x21B4F4B5, 0x56B3C423, 0xCFBA9599, 0xB8BDA50F,
	0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924, 0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D,
	0x76DC4190, 0x01DB7106, 0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
	0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818, 0x7F6A0DBB, 0x086D3D2D, 0x91646C97, 0xE6635C01,
	0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E, 0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457,
	0x65B0D9C6, 0x12B7E950, 0x8BBEB8EA, 0xFCB9887C, 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
	0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2, 0x4ADFA541, 0x3DD895D7, 0xA4D1C46D, 0xD3D6F4FB,
	0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0, 0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9,
	0x5005713C, 0x270241AA, 0xBE0B1010, 0xC90C2086, 0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
	0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4, 0x59B33D17, 0x2EB40D81, 0xB7BD5C3B, 0xC0BA6CAD,
	0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A, 0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683,
	0xE3630B12, 0x94643B84, 0x0D6D6A3E, 0x7A6A5AA8, 0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
	0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE, 0xF762575D, 0x806567CB, 0x196C3671, 0x6E6B06E7,
	0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC, 0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5,
	0xD6D6A3E8, 0xA1D1937E, 0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
	0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60, 0xDF60EFC3, 0xA867DF55, 0x316E8EEF, 0x4669BE79,
	0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236, 0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F,
	0xC5BA3BBE, 0xB2BD0B28, 0x2BB45A92, 0x5CB36A04, 0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
	0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A, 0x9C0906A9, 0xEB0E363F, 0x72076785, 0x05005713,
	0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38, 0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21,
	0x86D3D2D4, 0xF1D4E242, 0x68DDB3F8, 0x1FDA836E, 0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
	0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C, 0x8F659EFF, 0xF862AE69, 0x616BFFD3, 0x166CCF45,
	0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2, 0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB,
	0xAED16A4A, 0xD9D65ADC, 0x40DF0B66, 0x37D83BF0, 0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
	0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6, 0xBAD03605, 0xCDD70693, 0x54DE5729, 0x23D967BF,
	0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94, 0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D,
};

/*
 * 函数名      : CRC32Buf
 * 功能        : 计算一段缓存的CRC32值
 * 参数        : pBuf [in] (uint8_t *)缓存指针
 *             : u32Length [in] (uint32_t)缓存的大小
 * 返回值      : uint32_t 类型, 表示CRC32值
 * 作者        : 许龙杰
 */
uint32_t CRC32Buf(uint8_t *pBuf, uint32_t u32Length)
{
	uint32_t u32CRC32 = ~0, i;

	if ((pBuf == NULL) || (u32Length == 0))
	{
		return u32CRC32;
	}

	for (i = 0; i < u32Length; ++i)
	{
		u32CRC32 = s_u32CRC32Table[((u32CRC32 & 0xFF) ^ pBuf[i] )] ^ (u32CRC32 >> 8);
	}
	u32CRC32 = ~u32CRC32;
	return u32CRC32;
}

int32_t CRC32File(const char *pName, uint32_t *pCRC32)
{
	int32_t s32FD, s32Read;
	uint8_t u8Buf[512];
	uint32_t u32CRC32 = ~0;

	if ((pName == NULL) || (pCRC32 == NULL))
	{
		return MY_ERR(_Err_InvalidParam);
	}

	s32FD = open(pName, O_RDONLY);
	if (s32FD < 0)
	{
		return MY_ERR(_Err_SYS + errno);
	}

	while ((s32Read = read(s32FD, u8Buf, 512)) > 0)
	{
		int32_t i;
		for (i = 0; i < s32Read; i++)
		{
			u32CRC32 = s_u32CRC32Table[((u32CRC32 & 0xFF) ^ u8Buf[i] )] ^ (u32CRC32 >> 8);
		}
	}
	close(s32FD);
	if (s32Read < 0)
	{
		return MY_ERR(_Err_SYS + errno);
	}

	*pCRC32 = ~u32CRC32;

	return 0;
}


