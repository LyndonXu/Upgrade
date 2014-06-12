/****************************************************************************
 * Copyright(c), 2001-2060, ******************************* 版权所有
 ****************************************************************************
 * 文件名称             : deamon.c
 * 版本                 : 0.0.1
 * 作者                 : 许龙杰
 * 创建日期             : 2014年4月18日
 * 描述                 : 
 ****************************************************************************/
#include "common_define.h"
#include "upgrade.h"

#define MAXLINE 4096            /* max line length */

/*
 * 函数名      : ErrDoit
 * 功能        : 打印错误信息
 * 参数        : boIsPrintErr [in] (bool类型): 是否打印错误码
 *             : s32Err [in] (int32_t类型): 错误码
 *             : pFmt [in] (const char * 类型): 格式
 *             : ap [in] (va_list 类型): 可变参数
 * 返回值      : 无
 * 作者        : 许龙杰
 */
static void ErrDoit(bool boIsPrintErr, int32_t s32Err, const char *pFmt,
        va_list ap)
{
    char    c8Buf[MAXLINE];

    vsnprintf(c8Buf, MAXLINE, pFmt, ap);

    PRINT("%s", c8Buf);

    if (boIsPrintErr)
    {
        PRINT("%s", strerror(s32Err));
    }
    PRINT("\n");
}
/*
 * 函数名      : ErrQuit
 * 功能        : 打印错误信息并退出
 * 参数        : pFmt [in] (const char * 类型): 格式
 *             : ... [in] : 可变参数
 * 返回值      : 无
 * 作者        : 许龙杰
 */
static  void ErrQuit(const  char *pFmt, ...)
{
    va_list     ap;

    va_start(ap, pFmt);
    ErrDoit(0, 0, pFmt, ap);
    va_end(ap);
    exit(1);
}
/*
 * 函数名      : Daemonize
 * 功能        : 创建守护进程
 * 参数        : boIsFork [in] (bool类型): 是否创建进程
 * 返回值      : 无
 * 作者        : 许龙杰
 */
void Daemonize(bool boIsFork)
{
    int32_t s32Fd0, s32Fd1, s32Fd2;
    int32_t s32PID;
    struct rlimit       stRlimit;
    struct sigaction    stSigaction;

    umask(0);

    /*
     * 得到文件描述符的最大限制
     */
    if (getrlimit(RLIMIT_NOFILE, &stRlimit) < 0)
    {
        ErrQuit("can't get file limit");
    }

    if(boIsFork)
    {
        /*
         * 成为会话终端header
         */
        if ((s32PID = fork()) < 0)
        {
            ErrQuit("can't fork");
        }
        else if (s32PID != 0)
        {
            exit(0);
        }
        setsid();
        /*
         * 保证将来的打开操作不会申请控制终端
         */
        stSigaction.sa_handler = SIG_IGN;
        sigemptyset(&stSigaction.sa_mask);
        stSigaction.sa_flags = 0;
        if (sigaction(SIGHUP, &stSigaction, NULL) < 0)
        {
             ErrQuit("can't ignore SIGHUP");
        }

        if ((s32PID = fork()) < 0)
        {
            ErrQuit("can't fork");
        }
		else if (s32PID != 0)
		{
			exit(0);
		}
    }



    /*
     * 改变当前工作目录到根目录, 以保证文件系统不会被卸载
     */
    if (chdir(WORK_DIR) < 0)
    {
        ErrQuit("can't change directory to "WORK_DIR);
    }

    /*
     * 关闭所有打开的描述符
     */
    if (stRlimit.rlim_max == RLIM_INFINITY)
    {
        stRlimit.rlim_max = 1024;
    }
    {
    	uint32_t i;
		for (i = 0; i < stRlimit.rlim_max; i++)
		{
			close(i);
		}
    }

    /*
     * 绑定标准输入, 输出, 错误到 /dev/null.
     */
    s32Fd0 = open("/dev/null", O_RDWR);
    s32Fd1 = dup(0);
    s32Fd2 = dup(0);

    if (s32Fd0 != 0 || s32Fd1 != 1 || s32Fd2 != 2)
    {
        ErrQuit("unexpected file descriptors %d %d %d", s32Fd0, s32Fd1, s32Fd2);
    }
}


/*
 * 函数名      : LockFile
 * 功能        : 尝试对文件加锁
 * 参数        : s32Fd [in] (int32_t类型): 文件描述符
 * 返回值      : int32_t fcntl的返回值
 * 作者        : 许龙杰
 */
static int32_t LockFile(int32_t s32Fd)
{
    struct flock stFlock;

    stFlock.l_type = F_WRLCK;
    stFlock.l_start = 0;
    stFlock.l_whence = SEEK_SET;
    stFlock.l_len = 0;
    return(fcntl(s32Fd, F_SETLK, &stFlock));
}

/*
 * 函数名      : AlreadyRunningUsingLockFile
 * 功能        : 检测程序是否已经运行
 * 参数        : 无
 * 返回值      : bool类型数据, 正在运行返回true, 否则返回false
 * 作者        : 许龙杰
 */
bool AlreadyRunningUsingLockFile(const char *pLockFile)
{
    int32_t s32Fd;
    char c8Buf[16];

    if (pLockFile == NULL)
    {
    	return false;
    }
    s32Fd = open(pLockFile, O_RDWR | O_CREAT, LOCK_MODE);
    if (s32Fd < 0)
    {
        syslog(LOG_ERR, "can't open %s: %s", pLockFile, strerror(errno));
        exit(1);
    }
    if (LockFile(s32Fd) < 0)
    {
        if (errno == EACCES || errno == EAGAIN)
        {
            close (s32Fd);
            return true;
        }
        syslog(LOG_ERR, "can't lock %s: %s", pLockFile, strerror(errno));
        exit(1);
    }
    ftruncate(s32Fd, 0);
    lseek(s32Fd, 0, SEEK_SET);
    sprintf(c8Buf, "%ld\n", (long)getpid());
    write(s32Fd, c8Buf, strlen(c8Buf)+1);
    return false;
}


int32_t TraversalDirCallback(const char *pCurPath, struct dirent *pInfo, void *pContext)
{
	/* not a directory */
	if ((pInfo->d_type & DT_DIR) == 0)
	{
		return 0;
	}
	/* not a process directory */
	if ((pInfo->d_name[0] > '9') || (pInfo->d_name[0] < '0'))
	{
		return 0;
	}

	{
		char c8Name[_POSIX_PATH_MAX];
		int32_t s32Err;

		s32Err = GetProcessNameFromPID(c8Name, _POSIX_PATH_MAX, atoi(pInfo->d_name));
		if (s32Err != 0)
		{
/*			PRINT("s32Err: 0x%08x\n", s32Err);*/
			return 0;
		}
/*		PRINT("pInfo->d_name: %s, c8Name: %s\n", pInfo->d_name, c8Name);*/
		if (strstr(c8Name, (const char *)pContext) != NULL)
		{
			return 1;
		}
	}

	return 0;
}

/*
 * 函数名      : AlreadyRunningUsingName
 * 功能        : 使用进程名字判断进程是否已经运行
 * 参数        : pLockFile[in] (const char *): 进程名
 * 返回值      : bool, 已经运行返回true, 否则false
 * 作者        : 许龙杰
 */
bool AlreadyRunningUsingName(const char *pName)
{

	int s32Rslt;

	if (pName == NULL)
	{
		return false;
	}

	s32Rslt = TraversalDir("/proc/", false, TraversalDirCallback, (void *)pName);

	if (s32Rslt == 1)
	{
		return true;
	}
	return false;
}
