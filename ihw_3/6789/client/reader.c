#include "utils.h"

int N, book_num, return_book = 0, was_waiting = 0, ID;
unsigned short port_1, port_2;
char* servIP;
int socketToClose = -1;

void sendToPort(int num, unsigned short port) {
    int book_num[2];
    book_num[0] = num;
    book_num[1] = ID;
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

    if (send(sock, book_num, sizeof(int) * 2, 0) != sizeof(int) * 2)
        dieWithError("CLIENT: send() sent a different number of bytes than expected\n");

    close(sock);
}

int getFromPort(int num, unsigned short port) {
    int book_num[2];
    book_num[0] = num;
    book_num[1] = ID;
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

    if (send(sock, book_num, sizeof(int) * 2, 0) != sizeof(int) * 2)
        dieWithError("CLIENT: send() sent a different number of bytes than expected\n");

    // Получение ответа
    int ans[2];
    if ((recv(sock, ans, sizeof(int) * 2, 0)) <= 0)
        dieWithError("CLIENT: recv() failed or connection closed prematurely\n");
    socketToClose = sock;
    return ans[0];
}

void handler(int signum) {
    printf("CLIENT%d: Завершаю работу...\n", ID);
    if (!return_book) {
        exit(0);
    }
    printf("CLIENT%d: Возвращаю книгу %d\n", ID, -book_num);

    sendToPort(book_num, port_1);
    printf("CLIENT%d: Книга %d возвращена\n", ID, -book_num);
    return_book = 0;

    exit(0);
}

int bookIsHere(int arg) {
    int result = getFromPort(arg, port_2);
    close(socketToClose);
    return result;
}

int main(int argc, char* argv[]) {
    signal(SIGINT, handler);
    srand(time(0));
    int upper = 99999, lower = 10000;
    ID = (rand() % (upper - lower + 1)) + lower;

    if (argc != 5) {
        printf("CLIENT%d: Неправильные аргументы\n", ID);
        exit(1);
    }

    N = atoi(argv[1]);
    if (N == 0) {
        printf("CLIENT%d: N is 0", ID);
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
            printf("CLIENT%d: Иду за книгой %d\n", ID, book_num);
        } else {
            printf("CLIENT%d: Возвращаю книгу %d\n", ID, -book_num);
        }
        // Отправляем библиотекарю book_num
        if (return_book) {
            sendToPort(book_num, port_1);
            printf("CLIENT%d: Книга %d возвращена\n", ID, -book_num);
            return_book = 0;
            sleep(1);
            continue;
        }
        // Получение ответа
        int ans = getFromPort(book_num, port_1);
        // Обработка ответа
        if (ans == 0) {
            printf("CLIENT%d: Книги %d нет. Жду.\n", ID, book_num);
            close(socketToClose);
            was_waiting = 1;
            while (1) {
                if (bookIsHere(book_num)) {
                    break;
                }
                sleep(1);
            }
        } else {
            return_book = 1;
            book_num *= -1;
            int reading = rand() % 3 + 1;
            printf("CLIENT%d: Книга %d есть. Читаю %d дн.\n", ID, -book_num, reading);
            int vars[2];
            vars[0] = reading;
            vars[1] = ID;
            if (send(socketToClose, vars, sizeof(int) * 2, 0) != sizeof(int) * 2)
                dieWithError("CLIENT: send() sent a different number of bytes than expected\n");
            close(socketToClose);
            sleep(5 * reading);
        }
    }
}
