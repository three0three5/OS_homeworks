#include "common.h"

int* library_window_answer;

int semid, shmid;

int open_everything(int N) {

    key_t key_1 = ftok(PATHNAME_FOR_VARS, 0), key_2 = ftok(PATHNAME_FOR_SEMAPHORES, 0);

    if (key_1 < 0 || key_2 < 0) {
        printf("Can't generate keys\n");
        return -1;
    }

    if ((semid = semget(key_1, 3, 0666 | IPC_CREAT)) < 0) {
        printf("Can\'t create semaphore\n");
        return -1;
    }

    semctl(semid, 0, SETVAL, 1);
    semctl(semid, 1, SETVAL, 0);
    semctl(semid, 2, SETVAL, 0);

    if ((shmid = shmget(key_2, (N + 2) * sizeof(int), 0666|IPC_CREAT)) < 0) {
        printf("Can't create shared memory\n");
        return -1;
    }

    if ((library_window_answer = (int*)shmat(shmid, NULL, 0)) == (int*)(-1)) {
        printf("Can't attach shared memory\n");
        return -1;
    }

    for (int i = 1; i <= N; ++i) {
        library_window_answer[i] = 1;
    }

    return 0;
}

int close_and_unlink_everything() {

    shmdt(library_window_answer);

    if (shmctl(shmid, IPC_RMID, NULL) < 0) {
        printf("eraser: shared memory remove error\n");
        return 1;
    }

    if (semctl(semid, 0, IPC_RMID, 0) < 0) {
        printf("Can\'t delete semaphore\n");
        return 1;
    }

    return 0;
}
