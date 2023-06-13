#include "utils.h"

#define MAXPENDING 256

int N;
int* library;
unsigned short port_1, port_2;
pthread_mutex_t lock;

// Поток для уведомления читателей
// Использует второй порт для соединения с клиентами
// На каждой запрос в виде числа возвращает 1,
// если книга с этим номером есть, и 0 иначе
void* thread_1() {
    int servSock;
    if ((servSock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
        dieWithError("SERVER: socket() failed\n");

    struct sockaddr_in serv_struct, client_struct;
    memset(&serv_struct, 0, sizeof(serv_struct));
    serv_struct.sin_family = AF_INET;
    serv_struct.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_struct.sin_port = htons(port_2);

    if (bind(servSock, (struct sockaddr *) &serv_struct, sizeof(serv_struct)) < 0)
        dieWithError("SERVER: bind() failed\n");

    int clntLen;
    // Программа не должна умирать, если
    // клиент получил SIGINT и отправить пакет
    // или прочитать не удалось
    while (1) {
        clntLen = sizeof(client_struct);
        int shared, bytes;
        if ((bytes = recvfrom(servSock, &shared, sizeof(int), 0,
            (struct sockaddr *) &client_struct, &clntLen)) != sizeof(int)) {
            printf("SERVER: [WARNING] received %d bytes: recvfrom()\n", bytes);
            continue;
        }

        if (shared > 0) {
            --shared;
            // Читаем library
            pthread_mutex_lock(&lock);
            shared = library[shared];
            pthread_mutex_unlock(&lock);
        }
        if (sendto(servSock, &shared, sizeof(int), 0, (struct sockaddr *) &client_struct, sizeof(client_struct)) != sizeof(int))
            printf("SERVER: [WARNING] send() failed\n");
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
    int servSock;
    if ((servSock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
        dieWithError("SERVER: socket() failed\n");

    struct sockaddr_in serv_struct, client_struct;
    memset(&serv_struct, 0, sizeof(serv_struct));
    serv_struct.sin_family = AF_INET;
    serv_struct.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_struct.sin_port = htons(port_1);

    if (bind(servSock, (struct sockaddr *) &serv_struct, sizeof(serv_struct)) < 0)
        dieWithError("SERVER: bind() failed\n");

    int clntLen;
    while (1) {
        clntLen = sizeof(client_struct);

        int shared, bytes;
        if ((bytes = recvfrom(servSock, &shared, sizeof(int), 0, (struct sockaddr *) &client_struct, &clntLen)) != sizeof(int)) {
            printf("SERVER: received %d bytes\n", bytes);
            dieWithError("SERVER: recv() failed\n");
        }

        if (shared > 0) {
            --shared;
            // Изменяем/читаем library
            pthread_mutex_lock(&lock);
            if (library[shared] == 0) {
                printf("SERVER: Книги %d нет\n", shared + 1);
                shared = 0;
            } else {
                --library[shared];
                printf("SERVER: Книга %d отдана\n", shared + 1);
                shared = 1;
            }
            pthread_mutex_unlock(&lock);
        } else {
            shared *= -1;
            --shared;
            // Изменяем/читаем library
            pthread_mutex_lock(&lock);
            library[shared] = 1;
            printf("SERVER: Книга %d возвращена\n", shared + 1);
            pthread_mutex_unlock(&lock);
            shared = 1;
        }
        if (sendto(servSock, &shared, sizeof(int), 0, (struct sockaddr *) &client_struct, sizeof(client_struct)) != sizeof(int))
            printf("SERVER: [WARNING] send() failed\n");
    }
}

void handler(int signum) {
    printf("SERVER: завершаю работу...\n");
    free(library);
    pthread_mutex_destroy(&lock);
    exit(0);
}

int main(int argc, char* argv[]) {
    signal(SIGINT, handler);

    if (argc != 4) {
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
    if (port_1 == port_2) {
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

    pthread_t first, second;
    pthread_create(&first, NULL, thread_2, NULL);
    pthread_create(&second, NULL, thread_1, NULL);

    pthread_join(first,NULL);
    pthread_join(second,NULL);
}
