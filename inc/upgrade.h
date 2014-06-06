/****************************************************************************
 * Copyright(c), 2001-2060, ******************************* 版权所有
 ****************************************************************************
 * 文件名称             : upgrade.h
 * 版本                 : 0.0.1
 * 作者                 : 许龙杰
 * 创建日期             : 2014年4月18日
 * 描述                 : 
 ****************************************************************************/
#ifndef UPGRADE_H_
#define UPGRADE_H_

#include <sys/types.h>
#include <dirent.h>
#include <netinet/in.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * 函数名      : LockOpen
 * 功能        : 创建一个新的锁 与 LockClose成对使用
 * 参数        : pName[in] (char * 类型): 记录锁的名字(不包含目录), 为NULL时使用默认名字
 * 返回值      : (int32_t) 成功返回非负数, 否则返回错误码
 * 作者        : 许龙杰
 */
int32_t LockOpen(const char *pName);

/*
 * 函数名      : LockClose
 * 功能        : 销毁一个锁, 并释放相关的资源 与 LockOpen成对使用
 * 参数        : s32Handle[in] (JA_SI32类型): LockOpen返回的值
 * 返回值      : 无
 * 作者        : 许龙杰
 */
void LockClose(int32_t s32Handle);

/*
 * 函数名      : LockLock
 * 功能        : 加锁 与 LockUnlock成对使用
 * 参数        : s32Handle[in] (JA_SI32类型): LockOpen返回的值
 * 返回值      : 成功返回0, 否则返回错误码
 * 作者        : 许龙杰
 */
int32_t LockLock(int32_t s32Handle);

/*
 * 函数名      : LockUnlock
 * 功能        : 解锁 与 LockLock 成对使用
 * 参数        : s32Handle[in] (JA_SI32类型): LockOpen返回的值
 * 返回值      : 成功返回0, 否则返回错误码
 * 作者        : 许龙杰
 */
int32_t LockUnlock(int32_t s32Handle);

/*
 * 函数名      : GetTheShmId
 * 功能        : 得到一个共享内存ID 与 ReleaseAShmId 成对使用
 * 参数        : pName [in] (char * 类型): 名字
 *             : u32Size [in] (JA_UI32类型): 共享内存的大小
 * 返回值      : 正确返回非负ID号, 否则返回错误码
 * 作者        : 许龙杰
 */
int32_t GetTheShmId(const char * pName, uint32_t u32Size);

/*
 * 函数名      : ReleaseAShmId
 * 功能        : 释放一个共享内存ID 与 GetTheShmId成对使用, 所有进程只需要一个进程释放
 * 参数        : s32Id [in] (JA_SI32类型): ID号
 * 返回值      : 无
 * 作者        : 许龙杰
 */
void ReleaseAShmId(int32_t s32Id);



/*
 * 比较pLeft和pRight(通过上下文指针pData传递)数据，操作附加数据(pAdditionData)
 * 返回 0 表示两块数据关键子相同，负数 表示rpLeft > pRight关键字，否则返回 正数
 */
typedef int32_t (*PFUN_SLO_Compare)(void *pLeft, void *pAdditionData, void *pRight);

/*
 * 遍历链表，操作附加数据(pAdditionData)
 * 返回值以具体调用函数而定
 */
typedef PFUN_SLO_Compare PFUN_SLO_Callback;


/*
 * 函数名      : SLOInit
 * 功能        : 初始化共享内存链表，并返回句柄，必须与SLODestroy成对使用，
 * 				 不一定要与SLOTombDestroy成对
 * 参数        : pName[in] (const char *类型): 共享内存的名字
 * 			   : u16Capacity[in] (uint16_t): 期望的容量
 * 			   : u32EntitySize[in] (uint32_t): 实体的大小
 * 			   : u32AdditionDataSize[in] (uint32_t): 附加数据的大小
 * 			   : pErr[out] (int32_t *): 在指针不为NULL的情况下返回错误码
 * 返回值      : int32_t 型数据, 0失败, 否则成功
 * 作者        : 许龙杰
 */
int32_t SLOInit(const char *pName, uint16_t u16Capacity, uint32_t u32EntitySize,
		uint32_t u32AdditionDataSize, int32_t *pErr);


/*
 * 函数名      : SLOInsertAnEntity
 * 功能        : 向共享内存链表插入一个实体(pFunCompare为NULL时直接插入，
 * 				 pFunCompare不为NULL是返回值为0的情况下)
 * 参数        : s32Handle[in] (int32_t): SLOInit的返回值
 * 			   : pData[in] (void *): 实体数据
 * 			   : pFunCompare[in] (PFUN_SLO_Compare): 详见定义(pData将传递到pFunCompare的pRight参数)
 * 返回值      : int32_t 型数据, 正数表示实体的索引, 否则表示错误码
 * 作者        : 许龙杰
 */
int32_t SLOInsertAnEntity(int32_t s32Handle, void *pData, PFUN_SLO_Compare pFunCompare);

/*
 * 函数名      : SLODeleteAnEntity
 * 功能        : 从共享内存链表删除一个实体（pFunCompare返回值为0的情况下）
 * 参数        : s32Handle[in] (int32_t): SLOInit的返回值
 * 			   : pData[in] (void *): 实体数据
 * 			   : pFunCompare[in] (PFUN_SLO_Compare): 详见定义(pData将传递到pFunCompare的pRight参数)
 * 返回值      : int32_t 型数据, 0成功, 否则表示错误码
 * 作者        : 许龙杰
 */
int32_t SLODeleteAnEntity(int32_t s32Handle, void *pData, PFUN_SLO_Compare pFunCompare);

/* 废弃，使用扫描方式和实体的索引删除一个实体 */
int32_t SLODeleteAnEntityUsingIndex(int32_t s32Handle, int32_t s32Index);


/*
 * 函数名      : SLODeleteAnEntityUsingIndexDirectly
 * 功能        : 通过索引从共享内存链表删除一个实体
 * 参数        : s32Handle[in] (int32_t): SLOInit的返回值
 * 			   : s32Index[in] (int32_t): 实体的索引
 * 返回值      : int32_t 型数据, 0成功, 否则表示错误码
 * 作者        : 许龙杰
 */
int32_t SLODeleteAnEntityUsingIndexDirectly(int32_t s32Handle, int32_t s32Index);

/*
 * 函数名      : SLOUpdateAnEntity
 * 功能        : 从共享内存链表更新一个实体（pFunCompare返回值为0的情况下）
 * 参数        : s32Handle[in] (int32_t): SLOInit的返回值
 * 			   : pData[in] (void *): 实体数据
 * 			   : pFunCompare[in] (PFUN_SLO_Compare): 详见定义(pData将传递到pFunCompare的pRight参数)
 * 返回值      : int32_t 型数据, 0成功, 否则表示错误码
 * 作者        : 许龙杰
 */
int32_t SLOUpdateAnEntity(int32_t s32Handle, void *pData, PFUN_SLO_Compare pFunCompare);

/* 废弃，使用扫描方式和实体的索引删除一个实体 */
int32_t SLOUpdateAnEntityUsingIndex(int32_t s32Handle, int32_t s32Index);


/*
 * 函数名      : SLOUpdateAnEntityUsingIndexDirectly
 * 功能        : 通过索引从共享内存链表更新一个实体
 * 参数        : s32Handle[in] (int32_t): SLOInit的返回值
 * 			   : s32Index[in] (int32_t): 实体的索引
 * 返回值      : int32_t 型数据, 0成功, 否则表示错误码
 * 作者        : 许龙杰
 */
int32_t SLOUpdateAnEntityUsingIndexDirectly(int32_t s32Handle, int32_t s32Index);

/*
 * 函数名      : SLOTraversal
 * 功能        : 遍历链表
 * 参数        : s32Handle[in] (int32_t): SLOInit的返回值
 * 			   : pFunCallback[in] (PFUN_SLO_Callback): 实体的索引，该函数返回非0的情况下
 * 			     程序会删除该实体(pData将传递到pFunCompare的pRight参数)
 * 返回值      : int32_t 型数据, 0成功, 否则表示错误码
 * 作者        : 许龙杰
 */
int32_t SLOTraversal(int32_t s32Handle, void *pData, PFUN_SLO_Callback pFunCallback);

/*
 * 函数名      : SLODestroy
 * 功能        : 释放该进程空间中的句柄占用的资源
 * 参数        : s32Handle[in] (int32_t): SLOInit的返回值
 * 返回值      : 无
 * 作者        : 许龙杰
 */
void SLODestroy(int32_t s32Handle);

/*
 * 函数名      : SLODestroy
 * 功能        : 释放该进程空间中的句柄占用的资源，并从该系统中销毁资源
 * 参数        : s32Handle[in] (int32_t): SLOInit的返回值
 * 返回值      : 无
 * 作者        : 许龙杰
 */
void SLOTombDestroy(int32_t s32Handle);




/*
 * 函数指针
 * 参数        : u32CmdNum[in] (uint32_t类型): 命令号
 *             : u32CmdCnt[in] (uint32_t类型): 此命令号命令的数量
 *             : u32CmdSize[in] (uint32_t类型): 每一个命令的大小
 *             : pCmdData[in] (const char *类型): 命令的数据
 *             : pContext[in] (void *类型): 用户上下文
 *
 */

typedef int32_t (*PFUN_MCS_Resolve_CallBack)
        (uint32_t u32CmdNum, uint32_t u32CmdCnt, uint32_t u32CmdSize,
        const char *pCmdData,
        void *pContext);


typedef struct _tagStSendV
{
    uint32_t u32Size;            /* 数据大小 */
    const void *pData;      /* 数据指针 */
}StSendV;

/*
 * 函数名      : MCSOpen
 * 功能        : 得到一个command stream 句柄
 *             必须与MCSClose成对出现
 * 参数        : pErr[out] (int32_t * 类型): 不为NULL时, 错误码, 0正确, 否则返回负数
 * 返回值      : int32_t 型数据, 0失败, 否则成功
 * 作者        : 许龙杰
 */
int32_t MCSOpen(int32_t *pErr);

/*
 * 函数名      : MCSClose
 * 功能        : 销毁MCSOpen返回的句柄中相关资源
 *             必须与MCSOpen成对出现
 * 参数        : s32YSCHandle[in] (int32_t类型): MCSOpen返回的句柄
 * 返回值      : 无
 * 作者        : 许龙杰
 */
void MCSClose(int32_t s32YSCHandle);

/*
 * 函数名      : MCSInsertACmd
 * 功能        : 在句柄中插入一条命令
 * 参数        : s32YSCHandle[in] (int32_t类型): MCSOpen返回的句柄
 *             : u32CmdNum[in] (uint32_t类型): 命令号
 *             : u32CmdCnt[in] (uint32_t类型): 命令对应的命令数量
 *             : u32CmdSize[in] (uint32_t类型): 每一个命令的大小
 *             : pCmdData[in] (void *类型): 命令数据
 *             : boIsRepeatCmd[in] (bool 类型): 是否连接到以前命令
 * 返回值      : int32_t 型数据, 0成功, 否则失败
 * 作者        : 许龙杰
 */
int32_t MCSInsertACmd(int32_t s32YSCHandle,
        uint32_t u32CmdNum, uint32_t u32CmdCnt, uint32_t u32CmdSize,
        const void *pCmdData,
        bool boIsRepeatCmd);

/*
 * 函数名      : MCSInsertAMCS
 * 功能        : 在句柄中插入一条MCS流
 * 参数        : s32MCSHandle[in] (int32_t类型): MCSOpen返回的句柄
 *             : pMCS[in] (const char * 类型): 指向命令流
 *             : u32MCSSize[in] (uint32_t类型): 不为 0 的时候指定命令流的大小
 * 返回值      : int32_t 型数据, 0成功, 否则失败
 * 作者        : 许龙杰
 */
int32_t MCSInsertAMCS(int32_t s32MCSHandle, const char *pMCS, uint32_t u32MCSSize);
/*
 * 函数名      : MCSGetStreamBuf
 * 功能        : 得到句柄中命令流
 * 参数        : s32MCSHandle[in] (int32_t类型): MCSOpen返回的句柄
 *             : pSize[out] (uint32_t类型): 不为 NULL 的时候保存命令流的大小
 *             : pErr[out] (int32_t * 类型): 不为NULL时, 错误码, 0正确, 否则返回负数
 * 返回值      : int32_t 型数据, 0成功, 否则失败
 * 作者        : 许龙杰
 */
const char *MCSGetStreamBuf(int32_t s32MCSHandle, uint32_t *pSize, int32_t *pErr);

/*
 * 函数名      : MCSMalloc
 * 功能        : 得到一些内存
 * 参数        : u32Size[in] (uint32_t类型): 要申请内存的大小
 * 返回值      : void * 型数据, NULL失败，否则成功
 * 作者        : 许龙杰
 */
void *MCSMalloc(uint32_t u32Size);
/*
 * 函数名      : MCSFree
 * 功能        : 释放MCSMalloc得到的内存
 * 参数        : pBuf[in] (void *类型): MCSMalloc的返回值
 * 返回值      : void * 型数据, NULL失败，否则成功
 * 作者        : 许龙杰
 */
void MCSFree(void *pBuf);
/*
 * 函数名      : MCSInsertACmdWithNoCopy
 * 功能        : 在句柄中插入一条命令, 但不拷贝命令载体, 命令载体必须使用MCSMalloc得到，
 *               成功之后一定不能使用MCSFree释放之，否则必须释放之
 * 参数        : s32MCSHandle[in] (int32_t类型): MCSOpen返回的句柄
 *             : u32CmdNum[in] (uint32_t类型): 命令号
 *             : u32CmdCnt[in] (uint32_t类型): 命令对应的命令数量
 *             : u32CmdSize[in] (uint32_t类型): 每一个命令的大小
 *             : pCmdData[in] (void *类型): 命令数据
 *             : boIsRepeatCmd[in] (bool 类型): 是否连接到以前命令
 * 返回值      : int32_t 型数据, 0成功, 否则失败
 * 作者        : 许龙杰
 */
int32_t MCSInsertACmdWithNoCopy(int32_t s32MCSHandle,
        uint32_t u32CmdNum, uint32_t u32CmdCnt, uint32_t u32CmdSize,
        const void *pCmdData,
        bool boIsRepeatCmd);
/*
 * 函数名      : MCSGetCmdLength
 * 功能        : 从MCS流中得到负载的命令大小
 * 参数        : pMCS[in] (const char * 类型): 指向命令流
 *             : pMCSCmdSize[out] (uint32_t *类型): 成功在*pMCSCmdSize中保存大小
 * 返回值      : int32_t 型数据, 0成功, 否则失败
 * 作者        : 许龙杰
 */
int32_t MCSGetCmdLength(const char *pMCS, uint32_t *pMCSCmdSize);
/*
 * 函数名      : MCSGetCmdCnt
 * 功能        : 从MCS流中得到负载的命令数量
 * 参数        : pMCS[in] (const char * 类型): 指向命令流
 *             : pMCSCmdCnt[out] (uint32_t *类型): 成功在*pMCSCmdCnt中保存大小
 * 返回值      : int32_t 型数据, 0成功, 否则失败
 * 作者        : 许龙杰
 */
int32_t MCSGetCmdCnt(const char *pMCS, uint32_t *pMCSCmdCnt);
/*
 * 函数名      : MCSResolve
 * 功能        : 解析命令流
 * 参数        : pMCS[in] (const char * 类型): 指向命令流
 *             : u32MCSSize[in] (uint32_t类型): 不为 0 的时候指定命令流的大小
 *             : pFunCallBack[in] (PFUN_MCS_Resolve_CallBack类型): 命令解析回调函数
 *             : pContext[in] (void *类型): 用户上下文
 * 返回值      : int32_t 型数据, 0成功, 否则失败
 * 作者        : 许龙杰
 */
int32_t MCSResolve(const char *pMCS, uint32_t u32MCSSize, PFUN_MCS_Resolve_CallBack pFunCallBack, void *pContext);


/*
 * 函数名      : MCSSyncReceive
 * 功能        : 以MCS头作为同步头从SOCKET中接收数据, 与 MCSSyncFree成对使用
 * 参数        : s32Socket[in] (int32_t类型): 要接收的SOCKET
 *             : boWantSyncHead [in] (bool类型): 是否希望在数据前端增加同步头
 *             : u32TimeOut[in] (uint32_t类型): 超时时间(ms)
 *             : pSize[out] (uint32_t * 类型): 保存数据的长度
 *             : pErr[out] (int32_t * 类型): 不为NULL时, *pErr中保存错误码
 * 返回值      : int32_t 型数据, 0成功, 否则失败
 * 作者        : 许龙杰
 */
void *MCSSyncReceive(int32_t s32Socket, bool boWantSyncHead, uint32_t u32TimeOut, uint32_t *pSize, int32_t *pErr);

/*
 * 函数名      : MCSSyncFree
 * 功能        : 释放数据, 与 MCSSyncReceive成对使用
 * 参数        : 无
 * 返回值      : 无
 * 作者        : 许龙杰
 */
void MCSSyncFree(void *pData);

/*
 * 函数名      : MCSSyncSend
 * 功能        : 以MCS头作为同步头向SOCKET中发送数据
 * 参数        : s32Socket[in] (int32_t类型): 要放送到的SOCKET
 *             : u32TimeOut[in] (uint32_t类型): 超时(ms)
 *             : u32CommandNum[in] (uint32_t 类型): 数据对应的命令号(根据情况可以添0)
 *             : u32Size[in] (uint32_t 类型): 数据的长度
 *             : pData[in] (const void * 类型): 数据
 * 返回值      : int32_t 型数据, 0成功, 否则失败
 * 作者        : 许龙杰
 */
int32_t MCSSyncSend(int32_t s32Socket,  uint32_t u32TimeOut, uint32_t u32CommandNum, uint32_t u32Size, const void *pData);

/*
 * 函数名      : MCSSyncSendData
 * 功能        : 以MCS头作为同步头向SOCKET中发送数据
 * 参数        : s32Socket[in] (int32_t类型): 要放送到的SOCKET
 *             : u32TimeOut[in] (uint32_t类型): 超时(ms)
 *             : u32Size[in] (uint32_t 类型): 数据的长度
 *             : pData[in] (const void * 类型): 数据
 * 返回值      : int32_t 型数据, 0成功, 否则失败
 * 作者        : 许龙杰
 */
int32_t MCSSyncSendData(int32_t s32Socket,  uint32_t u32TimeOut, uint32_t u32Size, const void *pData);

/*
 * 函数名      : MCSSyncSendV
 * 功能        : 以MCS头作为同步头向SOCKET中发送多个缓冲数据
 * 参数        : s32Socket[in] (int32_t类型): 要放送到的SOCKET
 *             : u32TimeOut[in] (uint32_t类型): 超时(ms)
 *             : u32CommandNum[in] (uint32_t 类型): 数据对应的命令号(根据情况可以添0)
 *             : pSendV[in] (const StSendV * 类型): 多个缓冲数据头指针, 详见其定义
 *             : u32SendVCnt [in] (uint32_t 类型): 有多少个这样的StSendV
 * 返回值      : int32_t 型数据, 0成功, 否则失败
 * 作者        : 许龙杰
 */
int32_t MCSSyncSendV(int32_t s32Socket, uint32_t u32TimeOut, uint32_t u32CommandNum, const StSendV *pSendV, uint32_t u32SendVCnt);
/*
 * 函数名      : MCSSyncSendAMCS
 * 功能        : 以MCS头作为同步头向SOCKET中发送一个MCS数据
 * 参数        : s32Socket[in] (int32_t类型): 要放送到的SOCKET
 *             : u32TimeOut[in] (uint32_t类型): 超时(ms)
 *             : s32MCSHandle[in] (int32_t类型): MCSOpen返回的句柄
 *             : u32Size[in] (uint32_t类型): 不为0时指明MCS的大小
 * 返回值      : int32_t, 0正确, 否则返回错误码
 * 作者        : 许龙杰
 */
int32_t MCSSyncSendAMCS(int32_t s32Socket, uint32_t u32TimeOut, int32_t s32MCSHandle, uint32_t u32Size);


/*
 * 函数名      : AlreadyRunningUsingLockFile
 * 功能        : 使用文件所判断进程是否已经运行
 * 参数        : pLockFile[in] (const char *): 指定的加锁文件
 * 返回值      : bool, 已经运行返回true, 否则false
 * 作者        : 许龙杰
 */
bool AlreadyRunningUsingLockFile(const char *pLockFile);

/*
 * 函数名      : AlreadyRunningUsingName
 * 功能        : 使用进程名字判断进程是否已经运行
 * 参数        : pLockFile[in] (const char *): 进程名
 * 返回值      : bool, 已经运行返回true, 否则false
 * 作者        : 许龙杰
 */
bool AlreadyRunningUsingName(const char *pName);

/*
 * 函数名      : PrintLog
 * 功能        : 保存日志信息, 使用方法和 C99 printf一致
 * 参数        : pFmt [in]: (const char * 类型) 打印的格式
 *             : ...: [in]: 可变参数
 * 返回值      : (int32_t类型) 0表示成功, 否则错误
 * 作者        : 许龙杰
 */
int32_t PrintLog(const char *pFmt, ...);


/* 线程地址形式重定义 */
typedef void *(*PFUN_THREAD)(void *pArg);

/*
 * 函数名      : MakeThread
 * 功能        : 创建线程
 * 参数        : pThread [in] (PFUN_THREAD): 线程地址
 *             : pArg [in] (void *): 传递给线程的参数
 *             : boIsDetach [in] (bool): 是否是分离线程
 *             : pThreadId [in] (pthread_t *): 不为NULL时用于保存线程ID
 *             : boIsFIFO [in] (bool): 是否打开线程FIFO属性(需要root权限)
 * 返回值      : (int32_t类型) 0表示成功, 否则错误
 * 作者        : 许龙杰
 */
int32_t MakeThread(PFUN_THREAD pThread, void *pArg,
        bool boIsDetach, pthread_t *pThreadId, bool boIsFIFO);

/*
 * 函数名      : ServerListen
 * 功能        : 监听一个UNIX域套接字
 * 参数        : pName[in] (const char * 类型): 不为NULL时, 指定关联名字
 * 返回值      : int32_t 型数据, 正数返回socket, 否则返回错误码
 * 作者        : 许龙杰
 */
int32_t ServerListen(const char *pName);

/*
 * 函数名      : ServerAccept
 * 功能        : 获取要连接UNIX域套接字
 * 参数        : s32ListenFd[in] (int32_t类型): ServerListen的返回值
 * 返回值      : int32_t 型数据, 正数失败, 否则返回错误码
 * 作者        : 许龙杰
 */
int32_t ServerAccept(int32_t s32ListenFd);
/*
 * 函数名      : ServerRemove
 * 功能        : 删除UNIX域套接字
 * 参数        : s32Socket[in] (int32_t 类型): ServerListen的返回值
 *             : pName[in] (const char * 类型): 服务器的名字
 * 返回值      : 无
 * 作者        : 许龙杰
 */
void ServerRemove(int32_t s32Socket, const char *pName);

/*
 * 函数名      : ClientConnect
 * 功能        : 连接到log server
 * 参数        : pName[in] (const char * 类型): 不为NULL时, 指定关联名字
 * 返回值      : int32_t 型数据, 正数返回socket, 否则返回错误码
 * 作者        : 许龙杰
 */
int32_t ClientConnect(const char *pName);

int32_t SendFd(int32_t s32Socket, int32_t s32Fd);
int32_t ReceiveFd(int32_t s32Socket);


/*
 * 函数名      : ProcessListInit
 * 功能        : 初始化进程维护表在该进程空间的资源，并将该进程信息插入到表中
 * 参数        : pErr[in] (int32_t * 类型): 不为NULL时, 用于保存错误码
 * 返回值      : int32_t 型数据, 错误返回0, 否则返回控制句柄
 * 作者        : 许龙杰
 */
int32_t ProcessListInit(int32_t *pErr);


/*
 * 函数名      : ProcessListUpdate
 * 功能        : 更新进程表，指明当前进程在仍在有效运行
 * 参数        : s32Handle[in] (int32_t): ProcessListInit的返回值
 * 返回值      : int32_t 型数据, 正数返回0, 否则返回错误码
 * 作者        : 许龙杰
 */
int32_t ProcessListUpdate(int32_t s32Handle);


/*
 * 函数名      : ProcessListDestroy
 * 功能        : 从进程表中删除该进程信息，并销毁句柄在该进程空间的资源
 * 参数        : s32Handle[in] (int32_t): ProcessListInit的返回值
 * 返回值      : 无
 * 作者        : 许龙杰
 */
void ProcessListDestroy(int32_t s32Handle);

/*
 * 函数名      : GetProcessNameFromPID
 * 功能        : 通过进程号得到进程名字
 * 参数        : pNameSave [out]: (char * 类型) 将得到的名字保存到的位置
 *             : u32Size [in]: (uint32_t类型) 指示pNameSave的长度
 *             : s32ID [in]: (int32_t类型) 进程号
 * 返回值      : (int32_t类型) 0表示成功, 否则错误
 * 作者        : 许龙杰
 */
int32_t GetProcessNameFromPID(char *pNameSave, uint32_t u32Size, int32_t s32ID);


/*
 * 函数名      : LogFileCtrlInit
 * 功能        : 初始化日志文件控制，与函数LogFileCtrlDestroy成对使用
 * 参数        : pName [in] (const char *): 日志的名字
 * 			   : pErr [out] (int32_t *): 不为NULL时用于保护错误码
 * 返回值      : int32_t 错误返回0，否则返回控制句柄
 * 作者        : 许龙杰
 */
int32_t LogFileCtrlInit(const char *pName, int32_t *pErr);


/*
 * 函数名      : LogFileCtrlWriteLog
 * 功能        : 写入一条日志
 * 参数        : s32Handle [in] (int32_t) LogFileCtrlInit返回的句柄
 * 			   : pLog [in] (const char *): 日志内容, 字符串
 * 			   : s32Size [out] (int32_t): 正数表示内容的长度，负值，程序将会自动计算长度
 * 返回值      : int32_t 成功返回0，否则返回错误码
 * 作者        : 许龙杰
 */
int32_t LogFileCtrlWriteLog(int32_t s32Handle, const char *pLog, int32_t s32Size);


/*
 * 函数名      : LogFileCtrlGetAOldestBlockLog
 * 功能        : 读取一块最旧日志，成功后并使用过之后应该使用free函数释放*p2Log
 * 参数        : s32Handle [in] (int32_t) LogFileCtrlInit返回的句柄
 * 			   : p2Log [in] (const char **): 一块儿日志内容, 字符串
 * 			   : pSize [out] (int32_t *): 保存日志的长度
 * 返回值      : int32_t 成功返回0，否则返回错误码
 * 作者        : 许龙杰
 */
int32_t LogFileCtrlGetAOldestBlockLog(int32_t s32Handle, char **p2Log, uint32_t *pSize);

/*
 * 函数名      : LogFileCtrlGetAOldestBlockLog
 * 功能        : 删除一块最旧日志
 * 参数        : s32Handle [in] (int32_t) LogFileCtrlInit返回的句柄
 * 返回值      : int32_t 成功返回0，否则返回错误码
 * 作者        : 许龙杰
 */
int32_t LogFileCtrlDelAOldestBlockLog(int32_t s32Handle);

/*
 * 函数名      : LogFileCtrlDestroy
 * 功能        : 销毁日志控制和相关资源
 * 参数        : s32Handle [in] (int32_t) LogFileCtrlInit返回的句柄
 * 返回值      : 无
 * 作者        : 许龙杰
 */
void LogFileCtrlDestroy(int32_t s32Handle);

/*
 * 功能        : 遍历目录的回调函数指针定义
 * 参数        : pCurPath [in] (const char * 类型): 要查询的父目录
 *             : pInfo [in] (struct dirent *类型): 详见定义
 *             : pContext [in] (void *类型): 回调函数的上下文指针
 *             : 返回0为正确，否则为错误码，可导致到用函数退出
 * 作者        : 许龙杰
 */
typedef int32_t (*PFUN_TranversaDir_Callback)(const char *pCurPath, struct dirent *pInfo, void *pContext);

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
		PFUN_TranversaDir_Callback pFunCallback, void *pContext);


/*
 * 函数名      : btea
 * 功能        : XXTEA加密/解密
 * 参数        : v [in/out] (int32_t * 类型): 需要加密/解密的数据指针
 *             : n [in] (int32_t 类型): 需要加密/解密的数据长度，正值表示加密，负值表示解密
 *             : k [in] (int32_t *): 128位的Key值
 * 返回        : 正确返回0, 错误返回错误码
 * 作者        : 许龙杰
 */
int32_t btea(int32_t *v, int32_t n, int32_t *k);


#if !LIB_COMPLINE
typedef enum _tagEmCloudStat
{
	_Cloud_IsNotOnline = 0,
	_Cloud_IsOnline,
}EmCloudStat;

typedef struct _tagStCloudStat
{
	EmCloudStat emStat;					/* 云在线状态 */
	char c8ClientIPV4[16]; 				/* 可用的IP地址 192.168.100.100\0 */
}StCloudStat;							/* 云状态 */


typedef struct _tagStCloudDomain
{
	StCloudStat stStat;
	char c8Domain[64];					/* 服务器一级域名 */
	int32_t s32Port;					/* 端口 */
}StCloudDomain;							/* 通讯信息 */

typedef void *DBHANDLE;

#define XXTEA_KEYCNT			(128)
#define XXTEA_KEY_CNT_CHAR		(XXTEA_KEYCNT / sizeof(char))

#define RAND_NUM_CNT			(16)
#define PRODUCT_ID_CNT			(16)

#define IPV4_ADDR_LENGTH		(16)
#endif
/*
 * Flags for db_store().
 */
#define DB_INSERT	   1	/* insert new record only */
#define DB_REPLACE	   2	/* replace existing record */
#define DB_STORE	   3	/* replace or insert */

/*
 * Implementation limits.
 */
#define IDXLEN_MIN	   6	/* key, sep, start, sep, length, \n */
#define IDXLEN_MAX	1024	/* arbitrary */
#define DATLEN_MIN	   2	/* data byte, newline */
#define DATLEN_MAX	1024	/* arbitrary */

DBHANDLE db_open(const char *pathname, int32_t oflag, ...);
void db_close(DBHANDLE h);
char *db_fetch(DBHANDLE h, const char *key);
int32_t db_store(DBHANDLE h, const char *key, const char *data, int32_t flag);
int32_t db_delete(DBHANDLE h, const char *key);
void db_rewind(DBHANDLE h);
char *db_nextrec(DBHANDLE h, char *key);

/*
 * 函数名      : CloudTombDestroy
 * 功能        : 云在线状态销毁，从系统中删除共享内存信息，
 * 参数        : s32Handle[in] (int32_t类型): CloudInit返回的句柄
 * 返回值      : 无
 * 作者        : 许龙杰
 */
void CloudTombDestroy(int32_t s32Handle);

/*
 * 函数名      : CloudInit
 * 功能        : 初始化云在线状态共享内存链表，并返回句柄，必须与CloudDestroy成对使用，
 * 				 不一定要与CloudTombDestroy成对
 * 参数        : pErr[out] (int32_t *): 在指针不为NULL的情况下返回错误码
 * 返回值      : int32_t 型数据, 0失败, 否则成功
 * 作者        : 许龙杰
 */
int32_t CloudInit(int32_t *pErr);
/*
 * 函数名      : CloudGetStat
 * 功能        : 通过句柄得到云状态
 * 参数        : s32Handle [in] (int32_t): CloudInit返回的句柄
 * 			   : pStat[out] (StCloudStat *): 保存云状态，详见定义
 * 返回值      : int32_t 型数据, 0成功, 否则表示错误码
 * 作者        : 许龙杰
 */
int32_t CloudGetStat(int32_t s32Handle, StCloudStat *pStat);

/*
 * 函数名      : CloudSetStat
 * 功能        : 通过句柄设置云状态
 * 参数        : s32Handle [in] (int32_t): CloudInit返回的句柄
 * 			   : pStat[out] (StCloudStat *): 保存云状态，详见定义
 * 返回值      : int32_t 型数据, 0成功, 否则表示错误码
 * 作者        : 许龙杰
 */
int32_t CloudSetStat(int s32Handle, StCloudStat *pStat);

/*
 * 函数名      : CloudDestroy
 * 功能        : 释放该进程空间中的句柄占用的资源与CloudInit成对使用
 * 参数        : s32Handle[in] (int32_t): CloudInit的返回值
 * 返回值      : 无
 * 作者        : 许龙杰
 */
void CloudDestroy(int32_t s32Handle);

typedef struct _tagStMMap
{
	FILE *pFile;					/* 指向打开的临时文件 */
	void *pMap;						/* 通过mmap函数得到该进程空间的影射地址 */
	uint32_t u32MapSize;			/* 影射内存的大小 */
}StMMap;							/* 影射文件信息 */

typedef struct _tagStSendInfo
{
	bool boIsGet;					/* 通过GET方法发送?, 否则通过POST方法发送 */
	const char *pSecondDomain;		/* 二级域名 */
	const char *pFile;				/* URL地址(后半部分) */
	const char *pSendBody;			/* 发送的实体 */
	int32_t s32BodySize;			/* 发送的实体的长度 */
}StSendInfo;						/* 发送信息 */

/*
 * 函数名      : SSL_Init
 * 功能        : OPENSSL库初始化，仅需在进程启动开始调用一次
 * 参数        : 无
 * 返回        : 无
 * 作者        : 许龙杰
 */
void SSL_Init(void);
/*
 * 函数名      : SSL_Destory
 * 功能        : OPENSSL库初销毁，仅需在进程结束时调用一次
 * 参数        : 无
 * 返回        : 无
 * 作者        : 许龙杰
 */
void SSL_Destory(void);

/*
 * 函数名      : CloudSendAndGetReturn
 * 功能        : 向云发送数据并得到返回，保存在pMap中
 * 参数        : pStat [in] (StCloudDomain * 类型): 云状态，详见定义
 *             : pSendInfo [in] (StSendInfo 类型): 发送信息，详见定义
 *             : pMap [out] (StMMap *): 保存返回内容，详见定义，使用过之后必须使用CloudMapRelease销毁
 * 返回        : 正确返回0, 错误返回错误码
 * 作者        : 许龙杰
 */
int32_t CloudSendAndGetReturn(StCloudDomain *pStat, StSendInfo *pSendInfo, StMMap *pMap);

/*
 * 函数名      : CloudMapRelease
 * 功能        : 销毁CloudSendAndGetReturn的返回内容
 * 参数        : pMap [out] (StMMap *): 返回的内容，详见定义
 * 返回        : 无
 * 作者        : 许龙杰
 */
void CloudMapRelease(StMMap *pMap);

typedef enum _tagEmRegionType
{
	_Region_HTTPS,
	_Region_UDP,

	_Region_Reserved,
}EmRegionType;

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
int32_t CloudSaveDomainViaRegion(int s32Handle, EmRegionType emType,
		const char *pRegion, const char *pDomain);

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
int32_t CloudGetDomainFromRegion(int s32Handle, EmRegionType emType,
		const char *pRegion, char *pDomain, uint32_t u32Size);


/*
 * 函数名      : GetRegionOfGateway
 * 功能        : 得到Gateway的区域信息
 * 参数        : pRegion [out] (char * 类型): 成功保存区域字符串
 *             : uint32_t [in] (uint32_t 类型): pRegion指向字符串的长度
 * 返回        : 正确返回0, 错误返回错误码
 * 作者        : 许龙杰
 */
int32_t GetRegionOfGateway(char *pRegion, uint32_t u32Size);

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
int32_t GetDomainPortFromString(const char *pStr, char *pDomain, uint32_t u32Size, int32_t *pPort);


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
		const char c8ID[PRODUCT_ID_CNT], const char c8Key[XXTEA_KEY_CNT_CHAR]);

/*
 * 函数名      : CloudKeepAlive
 * 功能        : 云保活
 * 参数        : pStat [in] (StCloudDomain * 类型): 云状态，详见定义
 *             : c8ID [in] (const char * 类型): 产品ID
 * 返回        : 正确返回0, 错误返回错误码
 * 作者        : 许龙杰
 */
int32_t CloudKeepAlive(StCloudDomain *pStat, const char c8ID[PRODUCT_ID_CNT]);

typedef struct _tagStIPV4Addr
{
	char c8Name[16];				/* 用于保存网卡的名字 */
	char c8IPAddr[IPV4_ADDR_LENGTH];				/* 用于保存网卡的IP */
}StIPV4Addr;						/* IPV4网卡信息 */

/*
 * 函数名      : GetIPV4Addr
 * 功能        : 得到支持IPV4的已经连接的网卡的信息
 * 参数        : pAddrOut [out] (StIPV4Addr * 类型): 详见定义
 *             : pCnt [in/out] (uint32_t * 类型): 作为输入指明pAddrOut数组的数量，
 *               作为输出指明查询到的可用网卡的数量
 * 返回        : 正确返回0, 错误返回错误码
 * 作者        : 许龙杰
 */
int32_t GetIPV4Addr(StIPV4Addr *pAddrOut, uint32_t *pCnt);
/*
 * 函数名      : GetHostIPV4Addr
 * 功能        : 解析域名或者IPV4的IPV4地址
 * 参数        : pHost [in] (const char * 类型): 详见定义
 *             : c8IPV4Addr [in/out] (char * 类型): 用于保存dotted-decimal类型IP地址
 *             : pInternetAddr [in/out] (struct in_addr * 类型): 用户保存Internet类型地址
 * 返回        : 正确返回0, 错误返回错误码
 * 作者        : 许龙杰
 */
int32_t GetHostIPV4Addr(const char *pHost, char c8IPV4Addr[IPV4_ADDR_LENGTH], struct in_addr *pInternetAddr);
/*
 * 函数名      : TimeGetTime
 * 功能        : 得到当前系统时间 (MS级)
 * 参数        : 无
 * 返回值      : 当前系统时间ms
 * 作者        : 许龙杰
 */
uint64_t TimeGetTime(void);


typedef void *DBHANDLE;
/*
 * Flags for db_store().
 */
#define DB_INSERT	   1	/* insert new record only */
#define DB_REPLACE	   2	/* replace existing record */
#define DB_STORE	   3	/* replace or insert */

/*
 * Implementation limits.
 */
#define IDXLEN_MIN	   6	/* key, sep, start, sep, length, \n */
#define IDXLEN_MAX	1024	/* arbitrary */
#define DATLEN_MIN	   2	/* data byte, newline */
#define DATLEN_MAX	1024	/* arbitrary */

DBHANDLE db_open(const char *pathname, int32_t oflag, ...);
void db_close(DBHANDLE h);
char *db_fetch(DBHANDLE h, const char *key);
int32_t db_store(DBHANDLE h, const char *key, const char *data, int32_t flag);
int32_t db_delete(DBHANDLE h, const char *key);
void db_rewind(DBHANDLE h);
char *db_nextrec(DBHANDLE h, char *key);



#ifdef __cplusplus
}
#endif

#endif /* UPGRADE_H_ */
