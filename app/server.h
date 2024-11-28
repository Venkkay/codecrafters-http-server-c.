
#ifndef SERVER_H
#define SERVER_H

#define BUFFER_SIZE 1024

#define PATH_ECHO "/echo/"
#define PATH_USER_AGENT "/user-agent"
#define PATH_FILES "/files/"

int create_server_socket(void);
void accept_new_connection(int server_socket, fd_set *all_sockets, int *fd_max);
int read_data_from_socket(int socket, fd_set *all_sockets, int fd_max, int server_socket);
void compress_gzip(const char *input, int input_len, unsigned char **output, int *output_len);
int compressToGzip(unsigned char* input, int inputSize, unsigned char** output_pointer);

#endif //SERVER_H