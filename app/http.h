//
// Created by lucas on 11/25/24.
//

#ifndef HTTP_H
#define HTTP_H

#include <stddef.h>

#define HTTP_VERSION "HTTP/1.1"

// Define status
#define STATUS_T_SIZE 4
#define OK 0
#define CREATED 1
#define NOT_FOUND 2
#define INTERNAL_SERVER_ERROR 3

// Define methods
#define INVALID_METHOD -1
#define GET 0
#define POST 1

// Define header
#define HEADER_KEY_SIZE 32
#define HEADER_VALUE_SIZE 88
#define HEADER_SIZE HEADER_KEY_SIZE+HEADER_VALUE_SIZE+8
#define HEADER_TOTAL_SIZE HEADER_SIZE*8

#define HEADER_KEY_CONTENT_LENGTH "Content-Length"
#define HEADER_KEY_CONTENT_TYPE "Content-Type"
#define HEADER_VAL_APPLICATION_OCTET_STREAM "application/octet-stream"
#define HEADER_KEY_ACCEPT_ENCODING "Accept-Encoding"
#define HEADER_KEY_CONTENT_ENCODING "Content-Encoding"


#define BODY_MAX_SIZE 2048


// Enumération pour les codes de statut HTTP
typedef enum {
    HTTP_OK = OK,                // Requête réussie
    HTTP_CREATED = CREATED,       // Mauvaise requête
    HTTP_NOT_FOUND = NOT_FOUND,         // Ressource non trouvée
    HTTP_INTERNAL_SERVER_ERROR = INTERNAL_SERVER_ERROR, // Erreur interne du serveur
} http_status_t;

typedef enum {
    HTTP_GET = GET,                // Requête réussie
    HTTP_POST = POST,       // Mauvaise requête
} method_t;

typedef struct header_struct{
    char key[HEADER_KEY_SIZE];
    char value[HEADER_VALUE_SIZE];
    struct header_struct* next;
} header_t;

// Définition de request_t
typedef struct request_struct{
    method_t method;       // Méthode HTTP : "GET", "POST", etc.
    char path[256];        // URI de la requête
    header_t *headers;   // En-têtes HTTP
    size_t body_length;       // Longueur du corps de la requête
    unsigned char body[2048];      // Corps de la requête
} request_t;

// Définition de response_t
typedef struct response_struct{
    http_status_t status;      // Code de réponse : 200, 404, etc.
    header_t* headers;   // En-têtes HTTP
    size_t body_length;       // Longueur du corps de la requête
    unsigned char body[2048];      // Corps de la réponse
} response_t;

typedef size_t (*encoder_t)(const unsigned char *input, size_t input_size, unsigned char **output, size_t buffer_size);

typedef struct encoding_struct{
    char name[32];
    encoder_t encoder;
} encoding_t;

int status_to_code(http_status_t status);
const char *status_to_phrase(http_status_t status);
method_t method_value(const char *input);
header_t *headers_add(header_t *previous, const char *key, const char *value);
header_t *headers_add_number(header_t *previous, const char *key, size_t value);
void clean_headers(header_t *header);
char *headers_get(header_t *first, const char *key);
size_t receive_line(int socket, char *buffer, size_t length);
size_t gzip(unsigned char* input, size_t input_size, unsigned char** output_pointer);
size_t gzip_encoder(const unsigned char *input, size_t input_size, unsigned char **output, size_t buffer_size);
encoder_t get_encoder(const char *name);

#endif //HTTP_H
