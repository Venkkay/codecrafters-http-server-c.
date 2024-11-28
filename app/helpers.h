//
// Created by lucas on 11/25/24.
//

#ifndef HELPERS_H
#define HELPERS_H
#include "http.h"

// int split_into_array(const char *str, const char *delim, char result[][BUFFER_SIZE]);
// void get_value_in_str(const char *input, char *output, int max_len, char *prefix);
// void trimString(char *str);
unsigned char* read_file_to_response(const char *path, http_status_t *status, size_t *body_length);

#endif //HELPERS_H
