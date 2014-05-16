/****************************************************************************
 * Copyright(c), 2001-2060, ******************************* 版权所有
 ****************************************************************************
 * 文件名称             : stat_thread.c
 * 版本                 : 0.0.1
 * 作者                 : 许龙杰
 * 创建日期             : 2014年5月8日
 * 描述                 : 
 ****************************************************************************/
#include "common_define.h"
#include "upgrade.h"


static int32_t SLOTraversalUpdateInfoCallback(void *pLeft, void *pUnused, void *pRight)
{
	StProcessStat stProcessStat = {0};

	StProcessInfo *pProcessInfo = (StProcessInfo *)pLeft;
	StSYSInfo *pSYSInfo = (StSYSInfo *)pRight;
	int32_t s32Err;

	s32Err = GetProcessStat(pProcessInfo->u32Pid, &stProcessStat);
	if (s32Err != 0)
	{
		PrintLog("Process %s(%d) is exited for unknown reason\n");
		return s32Err;
	}
	if (s32Err == 0)
	{
		uint64_t u64Tmp2 = pProcessInfo->u32SysTime + pProcessInfo->u32UserTime;
		uint64_t u64Tmp;
		pProcessInfo->u32SysTime = stProcessStat.u32Stime;
		pProcessInfo->u32UserTime = stProcessStat.u32Utime;
		u64Tmp = pProcessInfo->u32SysTime + pProcessInfo->u32UserTime;

		u64Tmp = (u64Tmp - u64Tmp2) * 10000 /
				(pSYSInfo->stCPU.u64Total - pSYSInfo->stCPU.u64PrevTotal);
		u64Tmp *= sysconf(_SC_NPROCESSORS_ONLN);
		pProcessInfo->u16CPUUsage = u64Tmp;
		u64Tmp2 = pProcessInfo->u16CPUAverageUsage;
		u64Tmp = u64Tmp2 * (100 - AVERAGE_WEIGHT) + u64Tmp * AVERAGE_WEIGHT;
		u64Tmp /= 100;
		pProcessInfo->u16CPUAverageUsage = u64Tmp;

		u64Tmp = pProcessInfo->u32RSS = stProcessStat.s32Rss;
		u64Tmp = ((u64Tmp / 1024 ) * 10000) / pSYSInfo->stMem.u32MemTotal;
		pProcessInfo->u16MemUsage = u64Tmp;

		u64Tmp2 = pProcessInfo->u16MemAverageUsage;
		u64Tmp = u64Tmp2 * (100 - AVERAGE_WEIGHT) + u64Tmp * AVERAGE_WEIGHT;
		u64Tmp /= 100;
		pProcessInfo->u16MemAverageUsage = u64Tmp;

		PRINT("Process(%d) CPU: %d.%02d, %d.%02d \tMem: %d.%02d, %d.%02d\n",
				pProcessInfo->u32Pid,
				pProcessInfo->u16CPUUsage / 100, pProcessInfo->u16CPUUsage % 100,
				pProcessInfo->u16CPUAverageUsage / 100, pProcessInfo->u16CPUAverageUsage % 100,
				pProcessInfo->u16MemUsage / 100, pProcessInfo->u16MemUsage % 100,
				pProcessInfo->u16MemAverageUsage / 100, pProcessInfo->u16MemAverageUsage % 100);
	}
	return 0;
}


void *ThreadStat(void *pArg)
{
	int32_t s32SLOHandle, s32Err = 0, s32Cnt = 0;
	StSYSInfo stSYSInfo = {{0}, {0}};
	s32SLOHandle = SLOInit(PEOCESS_LIST_NAME, PEOCESS_LIST_CNT, sizeof(StProcessInfo), 0, &s32Err);
	if (s32SLOHandle == 0)
	{
		PrintLog("SLOInit error 0x%08x", s32Err);
		PRINT("SLOInit error 0x%08x", s32Err);
	}
	while (!g_boIsExit)
	{
		CpuInfo(&(stSYSInfo.stCPU));
		MemInfo(&(stSYSInfo.stMem));

		s32Err = SLOTraversal(s32SLOHandle, &stSYSInfo, SLOTraversalUpdateInfoCallback);
		s32Err = PrintLog("Log test %d\n", s32Cnt++);
		PRINT("PrintLog error code: 0x%08x\n", s32Err);
		sleep(1);
	}
	SLODestroy(s32SLOHandle);

	return NULL;
}
