//
// Created by lucas on 11/25/24.
//

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

#include "http.h"
#include "helpers.h"


// int split_into_array(const char *str, const char *delim, char result[][BUFFER_SIZE]) {
//     char buffer[BUFFER_SIZE]; // Copie de la chaîne originale pour ne pas la modifier
//     strncpy(buffer, str, sizeof(buffer));
//     buffer[sizeof(buffer) - 1] = '\0'; // Sécurité contre débordement
//
//     int count = 0; // Compteur de tokens
//     char *part = strtok(buffer, delim);
//
//     while (part != NULL) {
//         if (count >= MAX_READ_SIZE) { // Empêcher de dépasser la capacité
//             fprintf(stderr, "Erreur : trop de tokens\n");
//             return -1;
//         }
//         strncpy(result[count], part, BUFFER_SIZE); // Copier le token dans le tableau
//         result[count][BUFFER_SIZE - 1] = '\0';     // Sécurité de fin de chaîne
//         count++;
//         part = strtok(NULL, delim);
//     }
//
//     return count; // Retourner le nombre de tokens
// }

// void get_value_in_str(const char *input, char *output, int max_len, char *prefix) {
//     const char *start = strstr(input, prefix);
//     if (start != NULL) {
//         start += strlen(prefix);
//         const char *end = strstr(start, "\r\n");
//         if (end != NULL) {
//             int len = end - start;
//             if (len >= max_len) len = max_len - 1;
//             strncpy(output, start, len);
//             output[len] = '\0';
//         } else {
//             strncpy(output, start, max_len - 1);
//             output[max_len - 1] = '\0';
//         }
//     } else {
//         // Si "User-Agent: " est introuvable
//         output[0] = '\0';
//     }
// }

// void trimString(char *str) {
//     while (isspace((unsigned char)str[0])){
//         memmove(str, str + 1, strlen(str));
//     }
//     while (isspace((unsigned char)str[strlen(str) - 1])){
//         str[strlen(str) - 1] = '\0';
//     }
// }

unsigned char* read_file_to_response(const char *path, http_status_t *status, size_t *body_length) {
    const int file = open(path, O_RDONLY);
    if (file == -1) {
        perror("Error opening file");
        *status = NOT_FOUND;
        return NULL;
    }

    struct stat stat_buffer;
    if (fstat(file, &stat_buffer) == -1) {
        perror("Error getting file stats");
        close(file);
        *status = INTERNAL_SERVER_ERROR;
        return NULL;
    }

    const size_t length = stat_buffer.st_size;
    unsigned char *body = malloc(length);
    if (body == NULL) {
        perror("Error allocating memory");
        close(file);
        *status = INTERNAL_SERVER_ERROR;
        return NULL;
    }

    if (read(file, body, length) != length) {
        perror("Error reading file");
        free(body);
        close(file);
        *status = INTERNAL_SERVER_ERROR;
        return NULL;
    }

    *body_length = length;

    close(file);
    return body;
}

