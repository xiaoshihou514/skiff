#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int count_occur(char *str, char pattern) {
    int count = 0;
    while (*str != '\0') {
        if (*str == pattern) {
            count++;
        }
        str++;
    }
    return count;
}

void strsplit(char *arr[], char *str, char *pattern) {
    unsigned i = 0;
    char *token = strtok(str, pattern);
    while (token != NULL) {
        arr[i] = token;
        // strtok remembers the state and also edits the original string to be
        // what is left after the take
        token = strtok(NULL, pattern);
        i++;
    }
}

char *str_path_clamp(char *parts[], int till) {
    // space for / and the \0 (till is one indexed!)
    char *result = malloc(PATH_MAX * sizeof(char));
    // Important! Or strcat start at weird positions
    result[0] = '\0';
    for (int i = 0; i < till; i++) {
        strcat(result, "/");
        strcat(result, parts[i]);
    }
    return result;
}

char *str_path_tail(char *parts[], unsigned len, unsigned size) {
    if (!len) {
        // happily do nothing
        return "";
    }
    // space for / and \0, last str does not have / but has \0
    unsigned str_size = len + 1;
    for (int i = size - 1; i > size - len - 1; i--) {
        str_size += strlen(parts[i]);
    }
    char *result = malloc(str_size * sizeof(char));
    result[0] = '\0';
    for (int i = size - len; i < size - 1; i++) {
        strcat(result, parts[i]);
        strcat(result, "/");
    }
    strcat(result, parts[size - 1]);
    return result;
}
