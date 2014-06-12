/****************************************************************************
 * Copyright(c), 2001-2060, ************************** 版权所有
 ****************************************************************************
 * 文件名称             : common_define.h
 * 版本                 : 0.0.1
 * 作者                 : 许龙杰
 * 创建日期             : 2014年04月08日
 * 描述                 :
 ****************************************************************************/

#ifndef COMMON_DEFINE_H_
#define COMMON_DEFINE_H_


#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <limits.h>
#include <signal.h>

#include <dirent.h>
#include <syslog.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

/*错误码定义
 * -------------------------------------------------------------------
 * | 31~28  | 27~26 |    25   |  24  |     23 ~ 16    |    15 ~ 0    |
 * |--------|-------|---------|------|----------------|--------------|
 * | 标识域 |  保留 | 0：保留 | 保留 |        保留    | 各模块错误码 |
 * |  0x0E  |       | 1：用户 |      | 暂用于模块分配 |              |
 * -------------------------------------------------------------------
 */
#define FULL_ERROR_CODE(Identification, UserOrReserved, Module, Code) \
        ((((Identification) & 0x0F) << 28) | \
        (((UserOrReserved) & 0x01) << 25) | \
        (((Module) & 0xFF) << 16) | \
        ((Code) & 0xFFFF))

#define ERROR_CODE(Module, Code) FULL_ERROR_CODE(0x0E, 0, (Module), (Code))

#define MY_ERR(code)        ERROR_CODE(0x15, (code))

#pragma pack(4)

#ifdef __cplusplus
extern "C" {
#endif

#if HAS_CROSS
#define WORK_DIR		"/var/workdir/"
#else
#define WORK_DIR		"/tmp/workdir/"
#endif

#define PROCESS_DIR		WORK_DIR

#define LOG_DIR			WORK_DIR"log/"

#define LOG_SOCKET		LOG_DIR"logserver.socket" /* 套接字关联文件 */
#define LOG_LOCK_FILE	LOG_DIR"logserver.lock"

#define LOG_FILE		LOG_DIR"logserver.log" /* 套接字关联文件 */

#if HAS_CROSS
#if defined _DEBUG
#define GATEWAY_INFO_FILE	"/mnt/GatewayInfo.info"
#else
#define GATEWAY_INFO_FILE	WORK_DIR"GatewayInfo.info"
#endif
#else
#define GATEWAY_INFO_FILE	"/home/lyndon/workspace/nfsboot/GatewayInfo.info"
#endif

#define DB_NAME				WORK_DIR"db"


#define CLI_PERM				S_IRWXU
#define MAX_LINE				(1024)
#define MAX_LISTEN_QUEUE		(10)
#define PAGE_SIZE				(4096)


#define KEY_FILE	"key.key"
#define SEM_FILE	"sem.sem"
#define SHM_FILE	"shm.shm"

#define MODE_RW		(S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)
#define LOCK_MODE	(S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

#define SEM_MODE	0600
#define KEY_MODE	0600/* (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH) */

#define SHM_SIZE    (PAGE_SIZE * 256)
#define SHM_MODE    0600

#define PEOCESS_LIST_NAME		"ProcessList"
#define PEOCESS_LIST_CNT		64

#define CLOUD_NAME				"CloudStat"
#define CLOUD_SOCKET			WORK_DIR"CloudSocket.socket"


#define XXTEA_KEYCNT			(128)
#define XXTEA_KEY_CNT_CHAR		(XXTEA_KEYCNT / sizeof(char))

#define RAND_NUM_CNT			(16)
#define PRODUCT_ID_CNT			(16)

#define IPV4_ADDR_LENGTH		(16)

#define COORDINATION_DOMAINA				"www.jiuan.com:443"
#define AUTHENTICATION_SC					"asdfaslkjfhaslkjdcaskjdh"
#define AUTHENTICATION_SV					"asdfdfdadfabghkhjhkykjdh"
#define AUTHENTICATION_ADDR					"gateway/auth"
#define AUTHENTICATION_SECOND_DOMAIN		"api"

#define KEEPALIVE_SC						"asdfaslkjfhaslkjdcaskjdh"
#define KEEPALIVE_SV						"asdfdfdadfabghkhjhkykjdh"
#define KEEPALIVE_ADDR						"keepalive.htm"


#define LIB_COMPLINE			1

typedef struct _tagStMCSCmdHeader
{
    uint32_t u32CmdNum;      /* 命令号 */
    uint32_t u32CmdCnt;      /* 命令数量 */
    uint32_t u32CmdSize;     /* 命令大小 */
}StMCSCmdHeader;

typedef struct _tagStMSCCmdTag
{
    StMCSCmdHeader stMCSCmdHeader;
    uint32_t u32TotalCnt;
    void *pData;                                  	/* 指向命令数据 */
    struct _tagStMSCCmdTag *pFrontmostPart;         /* 指向本命令的下一部分 */
    struct _tagStMSCCmdTag *pLastPart;              /* 指向本命令的最后一部分 */
    struct _tagStMSCCmdTag *pPrevCmd;               /* 指向上一条命令 */
    struct _tagStMSCCmdTag *pNextCmd;               /* 指向下一条命令 */
}StMCSCmdTag;

typedef struct _tagStMCSHeader
{
    uint8_t u8MixArr[4];                 /* 混合参数 */
    uint32_t u32CmdCnt;                  /* 命令包的数量 */
    uint32_t u32CmdTotalSize;            /* 命令总大小 */
    uint8_t u8CheckSumArr[4];            /* 校验和 */
}StMCSHeader;

typedef struct _tagStMCS
{
    StMCSHeader stMCSHeader;        /* 协议头 */
    StMCSCmdTag *pFrontmostCmd;     /* 指向最前面的命令 */
    StMCSCmdTag *pLastCmd;          /* 指向最后面的命令 */
    uint32_t u32TotalSize;          /* 整个命令流的大小, 在getbuf的时候更新 */
    void *pBuf;                  	/* 用户获取命令数据的时候用到的指针 */
}StMCS;


typedef struct _tagStMemCheck
{
	int32_t s32FlagHead; 		/* 0x01234567 */
	int16_t s16RandArray[8];	/* 随机数 */
	int32_t s32FlagTail; 		/* 0x89ABCDEF */
	int32_t s32CheckSum;		/* 校验码 */
} StMemCheck;

typedef struct _tagStSLO
{
	StMemCheck stMemCheck;				/* 校验标志 */
	uint32_t u32SLOTotalSize;			/* 链表的总大小 */
	uint32_t u32RealEntitySize;			/* 链表中实体实际总大小 */
	uint32_t u32EntityWithIndexSize;	/* 链表中实体和游标的总大小 */
	uint32_t u32FlagDataSize;			/* 标志区域的大小  */
	uint32_t u32FlagDataOffset;			/* 标志区域的偏移量 */
	uint32_t u32AdditionDataSize;		/* 附加数据大小 */
	uint32_t u32AdditionDataOffset;		/* 附加数据的偏移量 */
	uint32_t u32EntityOffset;			/* 实体的偏移量 */

	uint16_t u16SLOTotalCapacity;		/* 链表的元素容量 */
	uint16_t u16SLOCurrentCapacity;		/* 链表中当前使用量 */

	uint16_t u16HeadIdle;				/* 空闲链表的头索引 */
	uint16_t u16TailIdle;				/* 空闲链表的尾索引 */

	uint16_t u16Head;					/* 当前正在使用链表的头索引 */
	uint16_t u16Tail;					/* 当前正在使用链表的尾索引 */
} StSLO;

typedef struct _tagStProcessInfo
{
	uint32_t u32Pid;					/* 进程ID */
	char c8Name[64];					/* 进程名字 */

	uint32_t u32UserTime;				/* 进程用户时间 */
	uint32_t u32SysTime;				/* 进程系统时间 */

	uint32_t u32VSize;					/* 进程虚拟内存 */
	uint32_t u32RSS;					/* 进程实际内存 */

	uint16_t u16CPUUsage;				/* 当前进程CPU使用率 */
	uint16_t u16MemUsage;				/* 当前进程MEM使用率 */

	uint16_t u16CPUAverageUsage;		/* 平均进程CPU使用率 */
	uint16_t u16MemAverageUsage;		/* 平均进程MEM使用率 */

} StProcessInfo;						/* 进程信息 */

typedef struct _tagStSLOIndex
{
	uint16_t u16Prev;					/* 前一个内存块 */
	uint16_t u16Next;					/* 后一个内存块 */
	uint16_t u16SelfIndex;				/* 当前索引 */
	uint16_t u16Reserved;				/* 保留，内存对齐 */
} StSLOCursor;

typedef struct _tagStSLOEntity
{
	StProcessInfo stProcessInfo;		/* 自组织链表的实体 */
	StSLOCursor stSLOCursor;			/* 自组织链表的游标 */
} StSLOEntityWithIndex;

typedef struct _tagStSLOHandle
{
	int32_t s32LockHandle;				/* 锁句柄 */
	int32_t s32SHMHandle;				/* 共享内存句柄 */
	StSLO *pSLO;						/* 该进程空间的共享内存影射地址 */
} StSLOHandle;							/* 共享内存操作句柄 */


#define MEM_INFO_CNT		(4)			/* 需要读取MEM信息的数量 */
#define CPU_INFO_CNT		(8)			/* 需要读取CPU信息的数量 */
#define AVERAGE_WEIGHT		(12)		/* 一节滤波权重值 */




#define LOG_DATE_FORMAT		"[%04d-%02d-%02d %02d:%02d:%02d.%06d]" 	/* 日志时间的格式 */
typedef struct _tagStFileCtrlElement StFileCtrlElement;
struct _tagStFileCtrlElement			/* 日志控制中单个文件的控制信息 */
{
	uint64_t u64StartTime;				/* 日志控制中单个文件的开始时间us */
	uint64_t u64EndTime;				/* 日志控制中单个文件的结束时间us */
	uint32_t u32FileSize;				/* 日志控制中单个文件的文件大小 */
	uint32_t u32ValidSize;				/* 日志控制中单个文件的有效内容大小 */
	StFileCtrlElement *pNext;
	StFileCtrlElement *pPrev;
	FILE *pFile;
#ifdef _DEBUG
	char c8Name[_POSIX_PATH_MAX];
#endif
};

typedef struct _tagStFileCtrl StFileCtrl;
struct _tagStFileCtrl								/* 日志控制 */
{
	uint32_t u32MaxCnt;								/* 日志控制包含的最大文件数 */
	uint32_t u32CurUsing;							/* 已经使用的文件数 */
	StFileCtrlElement *pOldestFile;					/* 指向最旧的文件 */
	StFileCtrlElement *pLatestFile;					/* 指向最新的文件 */
	StFileCtrlElement *pFileCtrlElement;			/* 指向文件控制数组 */
	uint64_t u64GlobalStartTime;					/* 全局日志开始时间 */
	uint64_t u64GlobalEndTime;						/* 全局日志结束时间 */
	uint32_t u32GlobalValidSize;					/* 全局日志有效长度 */

	uint64_t u64BufStartTime;						/* 缓存开始时间 */
	uint64_t u64BufEndTime;							/* 缓存结束时间 */
	uint32_t u32BufUsingCnt;						/* 当前缓存的字节数 */
	char c8Buf[PAGE_SIZE];							/* 缓存 */
};

typedef struct _tagStLogFileCtrl					/* 日志控制句柄 */
{
	StFileCtrl *pFileCtrl;							/* 指向日志控制 */
	pthread_mutex_t stMutex;						/* 互斥量 */
}StLogFileCtrl;


int32_t CheckSum(int32_t *pData, uint32_t u32Cnt);

/* 详见 man /proc */
typedef struct _tagStProcessStat
{
	int32_t s32Pid;
/*	char c8Name[256];*/
	char c8State;
/*
	int32_t s32Ppid;
	int32_t s32Pgrp;
	int32_t s32Session;
	int32_t s32Tty_nr;
	int32_t s32Tpgid;

	uint32_t u32Flags;

	uint32_t u32Minflt;
	uint32_t u32Cminflt;
	uint32_t u32Majflt;
	uint32_t u32Cmajflt;
*/
	uint32_t u32Utime;
	uint32_t u32Stime;

/*
	int32_t s32Cutime;
	int32_t s32Cstime;
	int32_t s32Priority;
	int32_t s32Nice;
	int32_t s32Threads;
	int32_t s32Iterealvalue;

	uint64_t u64Starttime;
*/
	uint32_t u32Vsize;
	int32_t s32Rss;
}StProcessStat;

/* 详见 man /proc */
typedef struct _tagStUpTime
{
	double d64UpTime;
	double d64IdleTime;
}StUpTime;
/* 详见 man /proc */
typedef struct _tagStJiffies
{
	uint64_t u64Usr;
	uint64_t u64Nice;
	uint64_t u64Sys;
	uint64_t u64Idle;
	uint64_t u64IOWait;
	uint64_t u64Irq;
	uint64_t u64SoftIrq;
	uint64_t u64Streal;
	uint64_t u64PrevTotal;
	uint64_t u64Total;
	uint32_t u32Usage;
	uint32_t u32AverageUsage;
}StJiffies;
/* 详见 man /proc */
typedef struct _tagStMemInfo
{
	uint32_t u32MemTotal;
	uint32_t u32MemFree;
	uint32_t u32Buffers;
	uint32_t u32Cached;
	uint32_t u32Usage;
	uint32_t u32AverageUsage;
}StMemInfo;

typedef struct _tagStSYSInfo
{
	StJiffies stCPU;
	StMemInfo stMem;
}StSYSInfo;		/* 系统CPU和MEM信息 */


typedef struct _tagStProcessList
{
	int32_t s32SLOHandle;			/* 自组织链表控制句柄 */
	int32_t s32EntityIndex;			/* 进程申请到的实体索引 */
}StProcessList;


int32_t MemInfo(StMemInfo *pMemInfo);
int32_t CpuInfo(StJiffies *pJiffies);
int32_t GetUpTime(StUpTime *pUpTime);
int32_t GetProcessStat(pid_t s32Pid, StProcessStat *pProcessStat);



extern bool g_boIsExit;

void *ThreadLogServer(void *pArg);
void *ThreadStat(void *pArg);
void *ThreadCloud(void *pArg);

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

typedef struct _tagStCloud
{
	StMemCheck stMemCheck;				/* */
	StCloudStat stStat;					/*  */
}StCloud;								/* 云状态共享内存实体 */

typedef struct _tagStCloudHandle		/* 与StSLOHandle类似 */
{
	int32_t s32LockHandle;
	int32_t s32SHMHandle;
	StCloud *pCloud;
	void *pDBHandle;					/* 数据库句柄 */
}StCloudHandle;



int		lock_reg(int, int, int, off_t, int, off_t); /* {Prog lockreg} */

#define	read_lock(fd, offset, whence, len) \
			lock_reg((fd), F_SETLK, F_RDLCK, (offset), (whence), (len))
#define	readw_lock(fd, offset, whence, len) \
			lock_reg((fd), F_SETLKW, F_RDLCK, (offset), (whence), (len))
#define	write_lock(fd, offset, whence, len) \
			lock_reg((fd), F_SETLK, F_WRLCK, (offset), (whence), (len))
#define	writew_lock(fd, offset, whence, len) \
			lock_reg((fd), F_SETLKW, F_WRLCK, (offset), (whence), (len))
#define	un_lock(fd, offset, whence, len) \
			lock_reg((fd), F_SETLK, F_UNLCK, (offset), (whence), (len))

pid_t	lock_test(int, int, off_t, int, off_t);		/* {Prog locktest} */

#define	is_read_lockable(fd, offset, whence, len) \
			(lock_test((fd), F_RDLCK, (offset), (whence), (len)) == 0)
#define	is_write_lockable(fd, offset, whence, len) \
			(lock_test((fd), F_WRLCK, (offset), (whence), (len)) == 0)

void	err_msg(const char *, ...);			/* {App misc_source} */
void	err_dump(const char *, ...) __attribute__((noreturn));
void	err_quit(const char *, ...) __attribute__((noreturn));
void	err_cont(int, const char *, ...);
void	err_exit(int, const char *, ...) __attribute__((noreturn));
void	err_ret(const char *, ...);
void	err_sys(const char *, ...) __attribute__((noreturn));

enum
{
	_Err_Identification = 0x01,
	_Err_Handle,
	_Err_InvalidParam ,
	_Err_CmdLen,
	_Err_CmdType,
	_Err_NULLPtr,
	_Err_Time,
	_Err_Mem,
	_Err_NoneCmdData ,
	_Err_TimeOut,

	_Err_SLO_NoSpace = 0x100,
	_Err_SLO_NoElement,
	_Err_SLO_Exist,
	_Err_SLO_NotExist,
	_Err_SLO_Index,
	_Err_SLO_IndexNotUsing,

	_Err_LOG_NoText = 0x110,
	_Err_LOG_Time_FMT,
	_Err_NotADir,
	_Err_JSON,
	_Err_Authentication,
	_Err_IDPS,

	_Err_Cloud_IsNotOnline = 0x120,
	_Err_Cloud_Body,
	_Err_Cloud_JSON,
	_Err_Cloud_CMD,
	_Err_Cloud_Data,
	_Err_Cloud_Authentication,
	_Err_Cloud_Save_Domain,
	_Err_Cloud_Get_Domain,

	_Err_Cloud_Result = 0x130,

	_Err_Unkown_Host = 0x140,


	_Err_HTTP = 0x1000,

	_Err_SSL = 0x2000,

	_Err_SYS = 0xFF00,

	_Err_Common = 0xFFFF,
};

#ifdef _DEBUG
#define PRINT(x, ...) printf("[%s:%d]: "x, __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define PRINT(x, ...)
#endif

#ifdef __cplusplus
}
#endif

#endif /* COMMON_DEFINE_H_ */
