//
// Created by lucas on 11/25/24.
//

#include "http.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include <sys/socket.h>

const int STATUS_TO_CODE[STATUS_T_SIZE] = {
    [OK] = 200,
    [CREATED] = 201,
    [NOT_FOUND] = 404,
    [INTERNAL_SERVER_ERROR] = 500,
};

const char *STATUS_TO_PHRASE[STATUS_T_SIZE] = {
    [OK] = "OK",
    [CREATED] = "Created",
    [NOT_FOUND] = "Not Found",
    [INTERNAL_SERVER_ERROR] = "Internal Server Error",
};

encoding_t encodings[] = {
    {"gzip", gzip_encoder},
};

int status_to_code(const http_status_t status) {
    return STATUS_TO_CODE[status];
}

const char *status_to_phrase(const http_status_t status) {
    return STATUS_TO_PHRASE[status];
}

method_t method_value(const char *input) {
    if(strcmp(input, "GET") == 0) {
        return GET;
    } else if(strcmp(input, "POST") == 0) {
        return POST;
    }
    return INVALID_METHOD;
}

header_t *headers_add(header_t *previous, const char *key, const char *value) {
    header_t *header = malloc(sizeof(header_t));
    if (header == NULL) {
        return NULL; // Handle memory allocation failure
    }

    strncpy(header->key, key, sizeof(header->key) - 1);
    header->key[sizeof(header->key) - 1] = '\0'; // Ensure null-termination

    strncpy(header->value, value, sizeof(header->value) - 1);
    header->value[sizeof(header->value) - 1] = '\0'; // Ensure null-termination

    header->next = previous;

    return header;
}

header_t *headers_add_number(header_t *previous, const char *key, const size_t value) {
    char buffer[32];
    sprintf(buffer, "%ld", value);
    return (headers_add(previous, key, buffer));
}

char *headers_get(header_t *first, const char *key) {
    header_t *header = first;

    while (header) {
        if (strcasecmp(key, header->key) == 0) {
            return (header->value);

        }
        header = header->next;
    }
    return NULL;
}

void clean_headers(header_t *header) {
    while (header) {
        //free(header->key);
        //free(header->value);
        header_t *next = header->next;
        free(header);
        header = next;
    }
}

size_t receive_line(const int socket, char *buffer, const size_t length) {
    size_t index;
    char value;

    for (index = 0; index < length - 1; ++index) {

        if (recv(socket, &value, 1, 0) != 1) {
            return (-1);
        }

        if (value == '\n') {
            break;
        }

        buffer[index] = value;
    }

    if (index != 0 && buffer[index - 1] == '\r') {
        index--;
    }

    buffer[index] = '\0';
    return (index);
}

size_t gzip(unsigned char* input, const size_t input_size, unsigned char** output_pointer)
{
    const size_t output_size = 128 + input_size;
    unsigned char *output = malloc(output_size);

    if (output == NULL) {
        fprintf(stderr, "Memory allocation error\n");
        return Z_MEM_ERROR;
    }

    z_stream zs;
    zs.zalloc = Z_NULL;
    zs.zfree = Z_NULL;
    zs.opaque = Z_NULL;
    zs.avail_in = input_size;
    zs.next_in = input;
    zs.avail_out = output_size;
    zs.next_out = output;

    // hard to believe they don't have a macro for gzip encoding, "Add 16" is the best thing zlib can do:
    // "Add 16 to windowBits to write a simple gzip header and trailer around the compressed data instead of a zlib wrapper"
    int ret = deflateInit2(&zs, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 15 | 16, 8, Z_DEFAULT_STRATEGY);
    if (ret != Z_OK) {
        free(output);
        return 0;
    }
    ret = deflate(&zs, Z_FINISH);
    if (ret != Z_STREAM_END) {
        deflateEnd(&zs);
        free(output);
        return 0;
    }
    deflateEnd(&zs);

    *output_pointer = output;
    return zs.total_out;
}

size_t gzip_encoder(const unsigned char *input, size_t input_size, unsigned char **output, size_t buffer_size) {

    z_stream z;
    z.zalloc = Z_NULL;
    z.zfree = Z_NULL;
    z.opaque = Z_NULL;
    z.next_in = (unsigned char *)input;
    z.avail_in = input_size;
    z.next_out = output;
    z.avail_out = buffer_size;
    deflateInit2(&z, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 15 | 16, 8, Z_DEFAULT_STRATEGY);
    deflate(&z, Z_FINISH);
    deflateEnd(&z);
    return (z.total_out);
}

encoder_t get_encoder(const char *name) {
    const int encodings_size = sizeof(encodings) / sizeof(encoding_t);
    for(int i = 0; i < encodings_size; i++) {
        if(strcmp(name, encodings[i].name) == 0) {
            return encodings[i].encoder;
        }
    }
    return NULL;
}