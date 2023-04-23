#include "common.h"

pid_t main_pid;

void reader_proc(int r_id, int N) {
    sleep(r_id);
    srand(time(NULL));
    int book_num;
    int return_book = 0, was_waiting = 0;
    struct sembuf buf = {.sem_num = 0, .sem_op = 0, .sem_flg = 0};
    while (1) {
        if (return_book == 0) {
            if (was_waiting == 0) {
                book_num = rand() % N + 1;
            } else {
                was_waiting = 0;
            }
            printf("Reader_%d: Иду за книгой %d\n", r_id, book_num);
        } else {
            printf("Reader_%d: Возвращаю книгу %d\n", r_id, -book_num);
        }
        // Получаем доступ к переменной window
        buf.sem_num = 0;
        buf.sem_op = -1;
        buf.sem_flg = 0;
        if (semop(semid, &buf, 1) < 0) {
            printf("Can\'t sub 1 from semaphor: main.c 27\n");
            perror("semop");
            exit(-1);
        }
        // Меняем её значение
        library_window_answer[0] = book_num;
        // Уведомляем библиотекаря о том, что запрос готов
        buf.sem_num = 1;
        buf.sem_op = 1;
        buf.sem_flg = 0;
        if (semop(semid, &buf, 1) < 0) {
            printf("Can\'t add 1 to semaphor: main.c 37\n");
            exit(-1);
        }
        // Ждем ответ
        buf.sem_num = 2;
        buf.sem_op = -1;
        buf.sem_flg = 0;
        if (semop(semid, &buf, 1) < 0) {
            printf("Can\'t sub 1 from semaphor: main.c 45\n");
            exit(-1);
        }
        int ans = library_window_answer[N + 1];
        // Отдаем мьютекс
        buf.sem_num = 0;
        buf.sem_op = 1;
        buf.sem_flg = 0;
        if (semop(semid, &buf, 1) < 0) {
            printf("Can\'t add 1 to semaphor: main.c 54\n");
            exit(-1);
        }
        if (return_book == 1) {
            // Книгу успешно вернули
            return_book = 0;
            continue;
        }
        // Обработка ответа
        if (ans == -1) {
            printf("Reader_%d: Книги %d нет. Жду.\n", r_id, book_num);
            was_waiting = 1;
            while (library_window_answer[book_num] != 1) {;}
        } else {
            int reading = rand() % 3 + 1;
            printf("Reader_%d: Книга %d есть. Читаю %d дн.\n", r_id, book_num, reading);
            sleep(2 * reading);
            // После прочтения книгу нужно вернуть.
            book_num *= -1;
            return_book = 1;
        }
    }
    return;
}

void librarian_proc(int N) {
    struct sembuf buf = {.sem_num = 0, .sem_op = 0, .sem_flg = 0};
    while (1) {
        // Ждем, когда кто-то "откроет окно"
        buf.sem_num = 1;
        buf.sem_op = -1;
        buf.sem_flg = 0;
        if (semop(semid, &buf, 1) < 0) {
            printf("Can\'t sub 1 from semaphor: main.c 87\n");
            exit(-1);
        }
        // получаем реквест и обрабатываем его
        int request = library_window_answer[0];
        printf("Librarian: ");
        int result = 1;
        if (request > 0) {
            if (library_window_answer[request] == 0) {
                printf("книги %d нет.\n", request);
                result = -1;
            } else {
                printf("книга %d отдана.\n", request);
                --library_window_answer[request];
            }
        } else {
            printf("книга %d получена.\n", -request);
            ++library_window_answer[-request];
        }
        // Отдаем ответ
        library_window_answer[N + 1] = result;
        buf.sem_num = 2;
        buf.sem_op = 1;
        buf.sem_flg = 0;
        if (semop(semid, &buf, 1) < 0) {
            printf("Can\'t add 1 to semaphor: main.c 112\n");
            exit(-1);
        }
    }
    return;
}

void handler(int sig) {
    if (getpid() == main_pid) {
        printf("\nПрограмма останавливается...\n");
        if (close_and_unlink_everything()) {
            perror("close and unlink everything\n");
            exit(-1);
        }
        printf("Программа завершена.\n");
    }
    exit(0);
}

int main(int argc, char* argv[]) {
    signal(SIGINT, handler);
    main_pid = getpid();
    if (argc != 3) {
        printf("Неправильные аргументы\n");
        return 1;
    }
    int N, M;
    N = atoi(argv[1]);
    M = atoi(argv[2]);
    if (!(N * M)) {
        printf("Неправильные аргументы\n");
        return 1;
    }

    if (open_everything(N)) {
        return 1;
    }

    int index = 0;
    while (M--) {
        pid_t pid = fork();
        if (pid == 0) {
            reader_proc(index, N);
            return 0;
        } else {
            index++;
        }
    }
    librarian_proc(N);

    if (close_and_unlink_everything()) {
        return 1;
    }

    return 0;
}
