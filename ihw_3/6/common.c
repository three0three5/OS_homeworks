#include "common.h"

int* library_window_answer;

int semid, shmid;

int open_everything(int N) {



    for (int i = 1; i <= N; ++i) {
        library[i] = 1;
    }

    return 0;
}

int close_and_unlink_everything() {



    return 0;
}
