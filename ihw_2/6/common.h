#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>

#define PATHNAME_FOR_VARS "common.c"

#define PATHNAME_FOR_SEMAPHORES "common.h"

extern int* library_window_answer; // window, library, answer

extern int semid; // window_r, window_l, answer

int open_everything(int N);

int close_and_unlink_everything(void);

void handler(int sig);
