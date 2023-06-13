#include "utils.h"

int N, book_num, return_book = 0, was_waiting = 0;
unsigned short port_1, port_2;
char* servIP;
int tries;
struct sigaction myAction;
unsigned int fromSize;

int sendAndGet(int num, unsigned short port) {
    int book_num = num;
    int sock;
    int result = -1;
    struct sockaddr_in echoServAddr, fromAddr;
    tries = 0;
    // Отправляем библиотекарю book_num
    if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
        dieWithError("CLIENT: socket() failed\n");

    /* Set signal handler for alarm signal */
    myAction.sa_handler = catchAlarm;
    if (sigfillset(&myAction.sa_mask) < 0) /* block everything in handler */
        dieWithError("sigfillset() failed");
    myAction.sa_flags = 0;

    if (sigaction(SIGALRM, &myAction, 0) < 0)
        dieWithError("sigaction() failed for SIGALRM");



    memset(&echoServAddr, 0, sizeof(echoServAddr));     /* Zero out structure */
    echoServAddr.sin_family      = AF_INET;             /* Internet address family */
    echoServAddr.sin_addr.s_addr = inet_addr(servIP);   /* Server IP address */
    echoServAddr.sin_port        = htons(port); /* Server port */

    if (sendto(sock, &book_num, sizeof(int), 0, (struct sockaddr *) &echoServAddr, sizeof(echoServAddr)) != sizeof(int))
        dieWithError("CLIENT: send() sent a different number of bytes than expected\n");

    /* Get a response */
    fromSize = sizeof(fromAddr);
    alarm(TIMEOUT_SECS);        /* Set the timeout */
    int recvLen;
    while ((recvLen = recvfrom(sock, &result, 4, 0,
           (struct sockaddr *) &fromAddr, &fromSize)) < 0)
        if (errno == EINTR)     /* Alarm went off  */
        {
            if (tries < MAXTRIES)      /* incremented by signal handler */
            {
                printf("timed out, %d more tries...\n", MAXTRIES-tries);
                if (sendto(sock, &book_num, sizeof(int), 0, (struct sockaddr *) &echoServAddr, sizeof(echoServAddr)) != sizeof(int))
                    dieWithError("sendto() failed");
                alarm(TIMEOUT_SECS);
            }
            else
                dieWithError("No Response");
        }
        else
            dieWithError("recvfrom() failed");

    /* recvfrom() got something --  cancel the timeout */
    alarm(0);

    close(sock);
    return result;
}

void handler(int signum) {
    printf("CLIENT: Завершаю работу...\n");
    if (!return_book) {
        exit(0);
    }
    printf("CLIENT: Возвращаю книгу %d\n", -book_num);

    if (sendAndGet(book_num, port_1) < 0)
        dieWithError("Не удалось вернуть книгу\n");
    printf("CLIENT: Книга %d возвращена\n", -book_num);
    return_book = 0;

    exit(0);
}

int bookIsHere(int arg) {
    return sendAndGet(arg, port_2);
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
            int res = sendAndGet(book_num, port_1);
            if (res < 0)
                dieWithError("Не удалось вернуть книгу");
            printf("CLIENT: Книга %d возвращена\n", -book_num);
            return_book = 0;
            sleep(1);
            continue;
        }
        // Получение ответа
        int ans = sendAndGet(book_num, port_1);
        // Обработка ответа
        if (ans < 0) {
            dieWithError("Не удалось взять книгу");
        }
        if (ans == 0) {
            printf("CLIENT: Книги %d нет. Жду.\n", book_num);
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
            printf("CLIENT: Книга %d есть. Читаю %d дн.\n", -book_num, reading);
            sleep(2 * reading);
        }
    }
}

void catchAlarm(int ignored) {   /* Handler for SIGALRM */
    tries += 1;
}
