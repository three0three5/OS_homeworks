#include "common.h"

int shm_1, shm_2, shm_3;
int* library;
int* window;
int* answer;

sem_t* r_window_s;
sem_t* l_window_s;
sem_t* r_answer_s;

int open_everything(int N) {

    if ((r_window_s = sem_open(WINDOW_SEMAPHORE_READER, O_CREAT, 0666, 1)) == 0) {
        perror("sem_open");
        exit(-1);
    }

    if ((l_window_s = sem_open(WINDOW_SEMAPHORE_LIBRARIAN, O_CREAT, 0666, 0)) == 0) {
        perror("sem_open");
        exit(-1);
    }

    if ((r_answer_s = sem_open(ANSWER_SEMAPHORE_READER, O_CREAT, 0666, 0)) == 0) {
        perror("sem_open");
        exit(-1);
    }

    if ((shm_1 = shm_open(LIBRARY, O_CREAT|O_RDWR, 0666)) == -1 ) {
        printf("Opening error\n");
        perror("shm_open");
        return 1;
    }

    if (ftruncate(shm_1, (N + 1) * sizeof(int)) == -1) {
        printf("Memory sizing error\n");
        perror("ftruncate");
        return 1;
    }

    library = mmap(0, (N + 1) * sizeof(int), PROT_WRITE|PROT_READ, MAP_SHARED, shm_1, 0);
    if (library == (int*)-1 ) {
        printf("Error getting pointer to shared memory\n");
        perror("mmap");
        return 1;
    }

    if ((shm_2 = shm_open(WINDOW, O_CREAT|O_RDWR, 0666)) == -1 ) {
        printf("Opening error\n");
        perror("shm_open");
        return 1;
    }

    if (ftruncate(shm_2, sizeof(int)) == -1) {
        printf("Memory sizing error\n");
        perror("ftruncate");
        return 1;
    }

    window = mmap(0, sizeof(int), PROT_WRITE|PROT_READ, MAP_SHARED, shm_2, 0);
    if (window == (int*)-1 ) {
        printf("Error getting pointer to shared memory\n");
        perror("mmap");
        return 1;
    }

    if ((shm_3 = shm_open(ANSWER, O_CREAT|O_RDWR, 0666)) == -1 ) {
        printf("Opening error\n");
        perror("shm_open");
        return 1;
    }

    if (ftruncate(shm_3, sizeof(int)) == -1) {
        printf("Memory sizing error\n");
        perror("ftruncate");
        return 1;
    }

    answer = mmap(0, sizeof(int), PROT_WRITE|PROT_READ, MAP_SHARED, shm_3, 0);
    if (answer == (int*)-1 ) {
        printf("Error getting pointer to shared memory\n");
        perror("mmap");
        return 1;
    }

    for (int i = 1; i <= N; ++i) {
        library[i] = 1;
    }

    return 0;
}

int close_and_unlink_everything() {
    close(shm_1);
    close(shm_2);
    close(shm_3);

    if (sem_close(r_window_s) == -1) {
        perror("sem_close: r_window_s\n");
        exit(-1);
    }

    if (sem_close(l_window_s) == -1) {
        perror("sem_close: l_window_s\n");
        exit(-1);
    }

    if (sem_close(r_answer_s) == -1) {
        perror("sem_close: r_answer_s\n");
        exit(-1);
    }

    if (sem_unlink(WINDOW_SEMAPHORE_READER) == -1) {
        perror("sem_unlink: window semaphore reader\n");
        exit(-1);
    }

    if (sem_unlink(ANSWER_SEMAPHORE_READER) == -1) {
        perror("sem_unlink: answer semaphore reader\n");
        exit(-1);
    }

    if (sem_unlink(WINDOW_SEMAPHORE_LIBRARIAN) == -1) {
        perror("sem_unlink: window semaphore librarian\n");
        exit(-1);
    }

    if(shm_unlink(LIBRARY) == -1) {
        printf("Shared memory is absent\n");
        perror("shm_unlink");
        return 1;
    }
    if(shm_unlink(WINDOW) == -1) {
        printf("Shared memory is absent\n");
        perror("shm_unlink");
        return 1;
    }
    if(shm_unlink(ANSWER) == -1) {
        printf("Shared memory is absent\n");
        perror("shm_unlink");
        return 1;
    }

    return 0;
}
