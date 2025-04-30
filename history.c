// history.c
#include <stdio.h>
#include <stdlib.h>

#include <string.h>

#define MAX_HISTORY 100

static char* history[MAX_HISTORY];
static int history_count = 0;

// Save command to history buffer 
void save_to_history(const char* command) {
    if (history_count < MAX_HISTORY) {
        history[history_count] = strdup(command);
        history_count++;
    } else {
        // Shift old history up and add  new cmd at the end
        free(history[0]);
        for (int i = 1; i < MAX_HISTORY; i++) {
            history[i - 1] = history[i];
        }
        history[MAX_HISTORY - 1] = strdup(command);
    }
}

// Print history to  shell
void show_history() {
    for (int i = 0; i < history_count; i++) {
        printf("%d: %s\n", i + 1, history[i]);
    }
}
