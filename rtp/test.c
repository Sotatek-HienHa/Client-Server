#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


int main() {
    while (1) {
        char input[64];
        printf("Enter command (\"exit\" to quit): ");
        fflush(stdout);

        if (!fgets(input, sizeof(input), stdin)) {
            break;
        }
        input[strlen(input) - 1] = 0;
        printf("%d\n", strlen(input));

        if (strcmp(input, "exit") == 0) {
            printf("Exiting...\n");
            break;
        }
    }
    return 0;
}