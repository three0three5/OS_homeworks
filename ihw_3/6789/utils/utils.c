#include "utils.h"

void dieWithError(char* message) {
    perror(message);
    exit(1);
}
