#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include "parser.h"
#include "history.h"

// Max input length
#define MAX_LEN 1024

// Signal handler for Ctrl+C
void handle_sigint(int sig) {
    printf("\nsh> ");
    fflush(stdout);
}

int main() {
    char input[MAX_LEN];

    // Set up signal handling
    signal(SIGINT, handle_sigint);

    // Main shell loop
    while (1) {
        printf("sh> ");
        
        // Read input
        if (fgets(input, MAX_LEN, stdin) == NULL) {
            printf("\n");
            break;
        }

        // Remove trailing newline
        input[strcspn(input, "\n")] = '\0';

        // Exit command
        if (strcmp(input, "exit") == 0) {
            break;
        }

        // Save command in history
        save_to_history(input);

        // Parse and execute input
        parse_and_execute(input);
    }

    printf("Exiting shell. bye\n");
    return 0;
}
