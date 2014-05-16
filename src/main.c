/*
 * main.c
 *
 *  Created on: 2014年4月17日
 *      Author: lyndon
 */
#include "common_define.h"
#include "upgrade.h"

#define MAIN_PROCESS		"NameOfTheMainProcess"

bool g_boIsExit = false;

#if 1
static void ProcessStop(int32_t s32Signal)
{
    PRINT("Signal is %d\n", s32Signal);
    g_boIsExit = true;
}

int main(int argc, char *argv[])
{
	pthread_t s32LogServerPid = 0;
	pthread_t s32CloudPid = 0;
	int32_t s32Err;

	system("mkdir -p "LOG_DIR);

	/* check whether this process is running, if not, lock the LOG_LOCK_FILE file */
	if(AlreadyRunningUsingLockFile(LOG_LOCK_FILE))
	{
		PRINT("the process is running\n");
		exit(-1);
	}
	PRINT("the process %s(%d)is in\n", argv[0], getpid());

	signal(SIGINT, ProcessStop);
	signal(SIGTERM, ProcessStop);

	/* create the log server */
	s32Err = MakeThread(ThreadLogServer, NULL, false, &s32LogServerPid, false);

	if (s32Err != 0)
	{
		PRINT("MakeThread error: 0x%08x\n", s32Err);
		return -1;
	}

	/* create the cloud thread */
	s32Err = MakeThread(ThreadCloud, NULL, false, &s32CloudPid, false);
	if (s32Err != 0)
	{
		PRINT("MakeThread error: 0x%08x\n", s32Err);
		goto end;
	}

	/* check whether the MAIN_PROCESS process is running, if not, fork an run it */
	if (!AlreadyRunningUsingName(MAIN_PROCESS))
	{
		/* fork and run it */
		pid_t s32Pid;
		s32Pid = fork();
		if (s32Pid < 0)	/* error happened */
		{
			PRINT("fork %s error %d\n", MAIN_PROCESS, errno);
			PrintLog("fork %s error %d\n", MAIN_PROCESS, errno);
		}
		else if (s32Pid == 0) /* child process */
		{
			if (execl(PROCESS_DIR""MAIN_PROCESS, MAIN_PROCESS, (char *)NULL) < 0)
			{
				PRINT("execl %s error %d\n", MAIN_PROCESS, errno);
				PrintLog("execl %s error %d\n", MAIN_PROCESS, errno);
				exit(-1);
			}
		}
	}
	else
	{
		PRINT("the "MAIN_PROCESS" is running\n");
	}

	ThreadStat(NULL);

	g_boIsExit = true;
end:
	if (s32LogServerPid != 0)
	{
		pthread_join(s32LogServerPid, NULL);
	}
	if (s32CloudPid != 0)
	{
		pthread_join(s32CloudPid, NULL);
	}

	return 0;
}
#else
#if 1
int main(int argc, char *argv[])
{
	if (argc < 2)
	{
		printf("usage: %s process name\n", argv[0]);
		exit(-1);
	}

	if(AlreadyRunningUsingName(argv[1]))
	{
		printf("process %s is running\n", argv[1]);
	}
	else
	{
		printf("process %s is not running\n", argv[1]);
	}
	getchar();
	return 0;
}
#endif

#endif
