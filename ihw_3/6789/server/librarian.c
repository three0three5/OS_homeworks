#include "utils.h"

#define MAXPENDING 256
#define BUFSIZE 128

int N;
int observerSockets[32];
int* library;
unsigned short port_1, port_2, port_3;
pthread_mutex_t lock, iolock;

// Функция для отправки наблюдателям происходящих событий
void sendAndForget(char* message) {
    for (int i = 0; i < 32; ++i) {
        if (observerSockets[i] == -1) {
            continue;
        }
        pthread_mutex_lock(&iolock);
        int result = send(observerSockets[i], message, 128, 0);
        pthread_mutex_unlock(&iolock);
        if (result == -1) {
            observerSockets[i] = -1;
            continue;
        }
    }
}

// Поток для уведомления читателей
// Использует второй порт для соединения с клиентами
// На каждой запрос в виде числа возвращает 1,
// если книга с этим номером есть, и 0 иначе
void* thread_1() {
    int servSock;
    if ((servSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        dieWithError("SERVER: socket() failed\n");

    struct sockaddr_in serv_struct, client_struct;
    memset(&serv_struct, 0, sizeof(serv_struct));
    serv_struct.sin_family = AF_INET;
    serv_struct.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_struct.sin_port = htons(port_2);

    if (bind(servSock, (struct sockaddr *) &serv_struct, sizeof(serv_struct)) < 0)
        dieWithError("SERVER: bind() failed\n");

    if (listen(servSock, MAXPENDING) < 0)
        dieWithError("SERVER: listen() failed\n");

    int clntLen, clntSock;
    // Программа не должна умирать, если
    // клиент получил SIGINT и отправить пакет
    // или прочитать не удалось
    while (1) {
        clntLen = sizeof(client_struct);
        if ((clntSock = accept(servSock, (struct sockaddr *) &client_struct,
                               &clntLen)) < 0) {
            printf("SERVER: [WARNING] accept() failed\n");
            continue;
        }
        int shared, bytes;
        if ((bytes = recv(clntSock, &shared, sizeof(int), 0)) != sizeof(int)) {
            printf("SERVER: [WARNING] received %d bytes: recv()\n", bytes);
            close(clntSock);
            continue;
        }

        if (shared > 0) {
            --shared;
            // Читаем library
            pthread_mutex_lock(&lock);
            shared = library[shared];
            pthread_mutex_unlock(&lock);
        }
        if (send(clntSock, &shared, sizeof(int), 0) != sizeof(int))
            printf("SERVER: [WARNING] send() failed\n");
        close(clntSock);
    }
}

// Поток для взаимодействия с читателями
// Использует первый порт для соединения с клиентами
// Если получает положительное число n, возвращает 1,
// если данная книга есть, и отнимает 1 от library[n - 1],
// и возвращает 0, если книги нет
// Если получает отрицательное число, то добавляет 1 к
// library[-n - 1]
void* thread_2() {
    char buffToSend[128];
    int servSock;
    if ((servSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        dieWithError("SERVER: socket() failed\n");

    struct sockaddr_in serv_struct, client_struct;
    memset(&serv_struct, 0, sizeof(serv_struct));
    serv_struct.sin_family = AF_INET;
    serv_struct.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_struct.sin_port = htons(port_1);

    if (bind(servSock, (struct sockaddr *) &serv_struct, sizeof(serv_struct)) < 0)
        dieWithError("SERVER: bind() failed\n");

    printf("SERVER: ip = %s\n", inet_ntoa(serv_struct.sin_addr));

    if (listen(servSock, MAXPENDING) < 0)
        dieWithError("SERVER: listen() failed");

    int clntLen, clntSock;
    while (1) {
        clntLen = sizeof(client_struct);
        if ((clntSock = accept(servSock, (struct sockaddr *) &client_struct,
                               &clntLen)) < 0)
            dieWithError("SERVER: accept() failed\n");

        printf("SERVER: Подключен клиент %s\n", inet_ntoa(client_struct.sin_addr));

        int shared[2], bytes;
        if ((bytes = recv(clntSock, shared, sizeof(int) * 2, 0)) != sizeof(int) * 2) {
            printf("SERVER: received %d bytes\n", bytes);
            dieWithError("SERVER: recv() failed\n");
        }
        int bookWasGiven = 0;
        if (shared[0] > 0) {
            snprintf(buffToSend, sizeof(buffToSend), "CLIENT%d: Иду за книгой %d\n", shared[1], shared[0]);
            sendAndForget(buffToSend);
            --shared[0];
            // Изменяем/читаем library
            pthread_mutex_lock(&lock);
            if (library[shared[0]] == 0) {
                printf("SERVER: Книги %d нет\n", shared[0] + 1);
                snprintf(buffToSend, sizeof(buffToSend), "SERVER: Книги %d нет\n", shared[0] + 1);
                sendAndForget(buffToSend);
                shared[0] = 0;
            } else {
                --library[shared[0]];
                printf("SERVER: Книга %d отдана\n", shared[0] + 1);
                bookWasGiven = shared[0] + 1;
                snprintf(buffToSend, sizeof(buffToSend), "SERVER: Книга %d отдана\n", shared[0] + 1);
                sendAndForget(buffToSend);
                shared[0] = 1;
            }
            pthread_mutex_unlock(&lock);
            if (send(clntSock, shared, sizeof(int) * 2, 0) != sizeof(int) * 2)
                dieWithError("SERVER: send() failed\n");
            // Получаем кол-во дней, сколько читает
            if (bookWasGiven) {
                if (recv(clntSock, shared, sizeof(int) * 2, 0) < 0)
                dieWithError("SERVER: recv() failed\n");
                snprintf(buffToSend, sizeof(buffToSend), "CLIENT%d: Книга %d есть. Читаю %d дн.\n", shared[1], bookWasGiven, shared[0]);
                sendAndForget(buffToSend);
            }

        } else {
            shared[0] *= -1;
            --shared[0];
            // Изменяем/читаем library
            pthread_mutex_lock(&lock);
            ++library[shared[0]];
            printf("SERVER: Книга %d возвращена\n", shared[0] + 1);
            snprintf(buffToSend, sizeof(buffToSend), "SERVER: Книга %d возвращена\n", shared[0] + 1);
            sendAndForget(buffToSend);
            pthread_mutex_unlock(&lock);
        }

        close(clntSock);
    }
}

// Функция для соединения клиентов-наблюдателей с сервером ПО 3 ПОРТУ
// Максимум 32 активных наблюдателей
void* connectObserver() {
    int servSock;
    if ((servSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        dieWithError("SERVER: socket() failed\n");

    struct sockaddr_in serv_struct, client_struct;
    memset(&serv_struct, 0, sizeof(serv_struct));
    serv_struct.sin_family = AF_INET;
    serv_struct.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_struct.sin_port = htons(port_3);

    if (bind(servSock, (struct sockaddr *) &serv_struct, sizeof(serv_struct)) < 0)
        dieWithError("SERVER: bind() failed\n");

    if (listen(servSock, MAXPENDING) < 0)
        dieWithError("SERVER: listen() failed\n");

    while (1) {
        int clntLen, clntSock;
        clntLen = sizeof(client_struct);
        if ((clntSock = accept(servSock, (struct sockaddr *) &client_struct,
                            &clntLen)) < 0) {
            printf("SERVER: [WARNING] accept() observer socket failed\n");
            continue;
        }
        int inserted = 0;
        for (int i = 0; i < 32; ++i) {
            if (observerSockets[i] == -1) {
                observerSockets[i] = clntSock;
                inserted = 1;
                break;
            }
        }
        if (!inserted) {
            close(clntSock);
        }
    }
}

void handler(int signum) {
    printf("SERVER: завершаю работу...\n");
    sendAndForget("SERVER: завершаю работу...\n");
    free(library);
    pthread_mutex_destroy(&lock);
    exit(0);
}

int main(int argc, char* argv[]) {
    signal(SIGINT, handler);
    signal(SIGPIPE, SIG_IGN);

    for (int i = 0; i < 32; ++i) {
        observerSockets[i] = -1;
    }

    if (argc != 5) {
        printf("SERVER: Wrong arguments\n");
        exit(1);
    }

    N = atoi(argv[1]);
    if (N == 0) {
        printf("SERVER: N is 0\n");
        exit(1);
    }
    port_1 = atoi(argv[2]);
    port_2 = atoi(argv[3]);
    port_3 = atoi(argv[4]);

    if (port_1 == port_2 || port_1 == port_3 || port_2 == port_3) {
        printf("SERVER: No difference\n");
        exit(1);
    }
    library = (int*)malloc(N * sizeof(int));
    if (!library) {
        printf("SERVER: library is NULL\n");
        exit(1);
    }

    for (int i = 0; i < N; ++i) {
        library[i] = 1;
    }

    pthread_mutex_init(&lock, NULL);
    pthread_mutex_init(&iolock, NULL);

    pthread_t first, second, third;
    pthread_create(&first, NULL, thread_2, NULL);
    pthread_create(&second, NULL, thread_1, NULL);
    // Если появился observer - соединяемся с ним
    pthread_create(&third, NULL, connectObserver, NULL);
    pthread_join(first,NULL);
    pthread_join(second,NULL);
}
