#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <semaphore.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <sys/mman.h>

#define LIBRARY "library"
#define WINDOW "window"
#define ANSWER "answer"

#define WINDOW_SEMAPHORE_READER "window_semaphore_1"
#define ANSWER_SEMAPHORE_READER "answer_semaphore_1"
#define WINDOW_SEMAPHORE_LIBRARIAN "window_semaphore_2"

extern int shm;
extern int* library;
extern int* window;
extern int* answer;

extern sem_t* r_window_s;
extern sem_t* l_window_s;
extern sem_t* r_answer_s;

int open_everything(int N);

int close_and_unlink_everything(void);

void handler(int sig);
