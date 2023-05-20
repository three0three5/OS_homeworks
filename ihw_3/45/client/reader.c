#include "utils.h"

int N, book_num, return_book = 0, was_waiting = 0;
unsigned short port_1, port_2;
char* servIP;

void sendToPort(int num, unsigned short port) {
    int book_num = num;
    int sock;
    struct sockaddr_in echoServAddr;
    // Отправляем библиотекарю book_num
    if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        dieWithError("CLIENT: socket() failed\n");

    memset(&echoServAddr, 0, sizeof(echoServAddr));     /* Zero out structure */
    echoServAddr.sin_family      = AF_INET;             /* Internet address family */
    echoServAddr.sin_addr.s_addr = inet_addr(servIP);   /* Server IP address */
    echoServAddr.sin_port        = htons(port); /* Server port */

    if (connect(sock, (struct sockaddr *) &echoServAddr, sizeof(echoServAddr)) < 0)
        dieWithError("CLIENT: connect() failed\n");

    if (send(sock, &book_num, sizeof(int), 0) != sizeof(int))
        dieWithError("CLIENT: send() sent a different number of bytes than expected\n");

    close(sock);
}

int getFromPort(int num, unsigned short port) {
    int book_num = num;
    int sock;
    struct sockaddr_in echoServAddr;
    // Отправляем библиотекарю book_num
    printf("[info] send book_num\n");
    if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        dieWithError("CLIENT: socket() failed\n");

    memset(&echoServAddr, 0, sizeof(echoServAddr));     /* Zero out structure */
    echoServAddr.sin_family      = AF_INET;             /* Internet address family */
    echoServAddr.sin_addr.s_addr = inet_addr(servIP);   /* Server IP address */
    echoServAddr.sin_port        = htons(port); /* Server port */

    if (connect(sock, (struct sockaddr *) &echoServAddr, sizeof(echoServAddr)) < 0)
        dieWithError("CLIENT: connect() failed\n");

    if (send(sock, &book_num, sizeof(int), 0) != sizeof(int))
        dieWithError("CLIENT: send() sent a different number of bytes than expected\n");

    // Получение ответа
    int ans;
    printf("[info] ans\n");
    if ((recv(sock, &ans, sizeof(int), 0)) <= 0)
        dieWithError("CLIENT: recv() failed or connection closed prematurely\n");
    close(sock);
    printf("[info] ans is %d\n", ans);
    return ans;
}

void handler(int signum) {
    printf("CLIENT: Завершаю работу...\n");
    if (!return_book) {
        exit(0);
    }
    printf("CLIENT: Возвращаю книгу %d\n", -book_num);

    sendToPort(book_num, port_1);
    printf("CLIENT: Книга %d возвращена\n", -book_num);
    return_book = 0;

    exit(0);
}

int bookIsHere(int arg) {
    printf("[info] bookIsHere\n");
    return getFromPort(arg, port_2);
}

int main(int argc, char* argv[]) {
    signal(SIGINT, handler);

    if (argc != 5) {
        printf("CLIENT: Неправильные аргументы\n");
        exit(1);
    }

    N = atoi(argv[1]);
    if (N == 0) {
        printf("CLIENT: N is 0");
        exit(1);
    }

    port_1 = atoi(argv[2]);
    port_2 = atoi(argv[3]);
    servIP = argv[4];

    while (1) {
        if (return_book == 0) {
            if (was_waiting == 0) {
                book_num = rand() % N + 1;
            } else {
                was_waiting = 0;
            }
            printf("CLIENT: Иду за книгой %d\n", book_num);
        } else {
            printf("CLIENT: Возвращаю книгу %d\n", -book_num);
        }
        // Отправляем библиотекарю book_num
        if (return_book) {
            sendToPort(book_num, port_1);
            printf("CLIENT: Книга %d возвращена\n", -book_num);
            return_book = 0;
            continue;
        }
        // Получение ответа
        int ans = getFromPort(book_num, port_1);
        // Обработка ответа
        if (ans == 0) {
            printf("CLIENT: Книги %d нет. Жду.\n", book_num);
            was_waiting = 1;
            while (1) {
                int answer;
                if (answer = bookIsHere(book_num)) {
                    break;
                }
                printf("[info]: %d", answer);
            }
        } else {
            return_book = 1;
            book_num *= -1;
            int reading = rand() % 3 + 1;
            printf("CLIENT: Книга %d есть. Читаю %d дн.\n", -book_num, reading);
            sleep(2 * reading);
        }
    }
}
