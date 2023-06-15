#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <malloc.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <kernel/elog.h>
#include <kernel/lib/console.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <pthread.h>
#include <time.h>

extern int UTILS_TimespecAdd(struct timespec * const pxResult, const struct timespec * const x,
                       const struct timespec * const y );

#define BARRIER_NUM 10
static pthread_t barrierth[BARRIER_NUM] = {0};
static pthread_mutex_t barrier_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t signal_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t signal_cond = PTHREAD_COND_INITIALIZER;
static int barriers_run = 0;
static void* doSomething(void *arg)

{
	pthread_barrier_t *barrier = (pthread_barrier_t *)arg;
	pthread_barrier_wait(barrier);//所有线程都被阻塞在这里

	pthread_mutex_lock(&barrier_mutex);
	barriers_run++;
	printf ("%d thread run\n", barriers_run);
	//sleep(1);
	pthread_mutex_unlock(&barrier_mutex);
	return NULL;
}

static void *barrier_test_thread(void *arg)
{
	int i;
	char a;
	pthread_barrier_t barrier;
	struct timespec abstime = {10, 0};
	struct timespec curtime = {0, 0};
	struct timespec tartime = {0, 0};
	pthread_detach(pthread_self());
	
	pthread_barrier_init(&barrier, NULL, BARRIER_NUM + 1);

	for(i = 0; i < BARRIER_NUM; i++) {
		pthread_create(&barrierth[i], NULL, doSomething, (void *)&barrier);
		printf ("barrierth[i] %p\n", barrierth[i]);
	}

	pthread_mutex_lock(&signal_mutex);
	
	clock_gettime(CLOCK_REALTIME, &curtime);//get curtime
	UTILS_TimespecAdd(&tartime, &curtime, &abstime);//calculate the wakeup time
	printf ("enter cond wait\n");
	pthread_cond_timedwait(&signal_cond, &signal_mutex, &tartime);
	pthread_mutex_unlock(&signal_mutex);
	printf("wake up all thread\n");
	pthread_barrier_wait(&barrier);

	pthread_mutex_lock(&barrier_mutex);
	barriers_run++;
	printf ("%d thread run\n", barriers_run);
	//sleep(1);
	pthread_mutex_unlock(&barrier_mutex);

	for(i = 0; i < BARRIER_NUM; i++) {
		pthread_join(barrierth[i], NULL);
	}
	barriers_run = 0;

	printf ("barrier_test_thread exit\n");
	pthread_exit(NULL);
}

static int barrier_test(int argc, char *argv[])
{
	pthread_t threadid;
	pthread_create(&threadid, NULL, barrier_test_thread, NULL);
	return 0;
}

static int break_barrier_test(int argc, char *argv[])
{
	pthread_mutex_lock(&signal_mutex);
	pthread_cond_signal(&signal_cond);
	pthread_mutex_unlock(&signal_mutex);
}

static void show_help(void)
{
	printf ("pthread test cmd enter. press help to show detail\n");
}

static int pthread_enter(int argc, char *argv[])
{
	show_help();
	return 0;
}


CONSOLE_CMD(pthread, NULL, pthread_enter, CONSOLE_CMD_MODE_SELF, "enter pthread cmd console")
CONSOLE_CMD(barrier, "pthread", barrier_test, CONSOLE_CMD_MODE_SELF, "do barrier test")
CONSOLE_CMD(break_barrier, "pthread", break_barrier_test, CONSOLE_CMD_MODE_SELF, "do barrier test")

