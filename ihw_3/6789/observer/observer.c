#include "utils.h"

#define MAXPENDING 256
#define RCVBUFSIZE 128

unsigned short port;
char* servIP;
int sock;

void handler(int signum) {
    printf("OBSERVER: завершаю работу...\n");
    close(sock);
    exit(0);
}

int main(int argc, char* argv[]) {
    signal(SIGINT, handler);
    if (argc != 3) {
        printf("OBSERVER: Wrong args\n");
        exit(0);
    }

    port = atoi(argv[1]);
    servIP = argv[2];
    char buffer[RCVBUFSIZE];

    struct sockaddr_in echoServAddr;
    if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        dieWithError("CLIENT: socket() failed\n");

    memset(&echoServAddr, 0, sizeof(echoServAddr));     /* Zero out structure */
    echoServAddr.sin_family      = AF_INET;             /* Internet address family */
    echoServAddr.sin_addr.s_addr = inet_addr(servIP);   /* Server IP address */
    echoServAddr.sin_port        = htons(port); /* Server port */

    if (connect(sock, (struct sockaddr *) &echoServAddr, sizeof(echoServAddr)) < 0)
        dieWithError("CLIENT: connect() failed\n");

    while (1) {
        if ((recv(sock, buffer, sizeof(buffer), 0)) <= 0) {
            printf("recv() failed or connection closed prematurely\n");
            exit(0);
        }
        printf("%s", buffer);
    }
}
