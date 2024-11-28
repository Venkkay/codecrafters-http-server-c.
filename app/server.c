#include <stdio.h>
#include <stdlib.h>
#include <zlib.h>
#include <netdb.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "server.h"
#include "http.h"
#include "helpers.h"


#define PORT 4221
#define MAX_READ_SIZE 10  // Taille maximale du tableau
#define CRLF "\r\n"
#define COMMA ","


int main(int argc, char **argv) {
	setbuf(stdout, NULL);
 	setbuf(stderr, NULL);

	printf("Logs from your program will appear here!\n");

	if (argc == 3) {
		const char *directory = argv[2];
		printf("directory: %s\n", directory);
		chdir(directory);
	}

	// Pour surveiller les sockets clients :
	fd_set all_sockets; // Ensemble de toutes les sockets du serveur
	fd_set read_fds;    // Ensemble temporaire pour select()
	int fd_max;         // Descripteur de la plus grande socket
	struct timeval timer;


	const int server_fd = create_server_socket();
	 if (server_fd == -1) {
		return 1;
	 }

	const int connection_backlog = 5;
	 if (listen(server_fd, connection_backlog) != 0) {
	 	printf("Listen failed: %s \n", strerror(errno));
	 	return 1;
	 }
	printf("[Server] Listening on port %d\n", PORT);

	// Préparation des ensembles de sockets pour select()
	FD_ZERO(&all_sockets);
	FD_ZERO(&read_fds);
	FD_SET(server_fd, &all_sockets); // Ajout de la socket principale à l'ensemble
	fd_max = server_fd; // Le descripteur le plus grand est forcément celui de notre seule socket
	printf("[Server] Set up select fd sets\n");

	//const time_t start_time = time(NULL);
	//const time_t end_wait = start_time + 10;

    while(1) {
    	read_fds = all_sockets;
    	// Timeout de 2 secondes pour select()
    	timer.tv_sec = 2;
    	timer.tv_usec = 0;

	    const int status = select(fd_max + 1, &read_fds, NULL, NULL, &timer);
    	if (status == -1) {
    		fprintf(stderr, "[Server] Select error: %s\n", strerror(errno));
    		exit(1);
    	}
	    if (status == 0) {
		    printf("[Server] Waiting...\n");
		    continue;
	    }

	    for (int i = 0; i <= fd_max; i++) {
    		if (FD_ISSET(i, &read_fds) != 1) {
    			continue ;
    		}
    		printf("[%d] Ready for I/O operation\n", i);
    		if (i == server_fd) {
    			accept_new_connection(server_fd, &all_sockets, &fd_max);
    		}
    		else {
    			if(read_data_from_socket(i, &all_sockets, fd_max, server_fd) != 0) {
    				close(i);
    				FD_CLR(i, &all_sockets);
    			}
    		}
    	}
    }
	return 0;
}

// Renvoie le socket du serveur liée à l'adresse et au port qu'on veut écouter
int create_server_socket(void) {

	struct sockaddr_in serv_addr = {
		.sin_family = AF_INET ,
		.sin_port = htons(PORT),
		.sin_addr = { htonl(INADDR_ANY) },
	};

	// Création du socket
	const int server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd == -1) {
		printf("Socket creation failed: %s...\n", strerror(errno));
		return 1;
	}
	printf("[Server] Created server socket fd: %d\n", server_fd);

	const int reuse = 1;
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
		printf("SO_REUSEADDR failed: %s \n", strerror(errno));
		return 1;
	}

	// Liaison du socket à l'adresse et au port
	if (bind(server_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) != 0) {
		printf("Bind failed: %s \n", strerror(errno));
		return 1;
	}
	printf("[Server] Bound socket to localhost port %d\n", PORT);
	return (server_fd);
}


// Accepte une nouvelle connexion et ajoute le nouvel socket à l'ensemble des sockets
void accept_new_connection(int server_fd, fd_set *all_sockets, int *fd_max)
{
	int client_addr_len;
	struct sockaddr_in client_addr;

	client_addr_len = sizeof(client_addr);
	const int client_fd = accept(server_fd, (struct sockaddr *) &client_addr, &client_addr_len);

	if (client_fd < 0) {
		printf("[Server] Accept error: %s\n", strerror(errno));
		return;
	}
	FD_SET(client_fd, all_sockets); // Ajoute la socket client à l'ensemble
	if (client_fd > *fd_max) {
		*fd_max = client_fd; // Met à jour la plus grande socket
	}
	printf("[Server] Accepted new connection on client socket %d.\n", client_fd);
}

int read_data_from_socket(const int socket, fd_set *all_sockets, int fd_max, int server_socket)
{
    char buffer[BUFFER_SIZE];

	request_t request = {};

	size_t line_receive_length = receive_line(socket, buffer, sizeof(buffer));
	if(line_receive_length == -1) {
		perror("[Server] Error receiving request header");
		return 1;
	}

	char *method = strtok(buffer, " ");
	char *path = strtok(method + strlen(method) + 1, " ");
	char *version = strtok(path + strlen(path) + 1, " ");

	request.method = method_value(method);
	const size_t path_length = strlen(path);
	if(path_length > 1 && path[path_length-1] == '/') {
		path[path_length-1] = '\0';
	}
	strncpy(request.path, path, strlen(path));
	printf("method=`%s`, method=%d, path=`%s`, request.path=`%s` version=`%s`\n", method, request.method, path, request.path, version);

	while (1) {
		line_receive_length = receive_line(socket, buffer, sizeof(buffer));
		if (line_receive_length == -1) {
			perror("[Server] Error receiving request headers");
			return 1;
		}

		if (line_receive_length == 0) {
			break;
		}

		const char *key = strtok(buffer, ": ");
		const char *value = key + strlen(key) + 1;

		while (*value == ' ') {
			++value;
		}

		request.headers = headers_add(request.headers, key, value);
		printf("key=`%s` value=`%s`\n", request.headers->key, request.headers->value);
	}

	if (request.method == POST) {
		const char* content_length_string = headers_get(request.headers, HEADER_KEY_CONTENT_LENGTH);
		char *end_ptr_content_length;
		unsigned long content_length = strtol(content_length_string ? content_length_string : "0", &end_ptr_content_length, 10);
		content_length = content_length < BODY_MAX_SIZE - 1 ? content_length : BODY_MAX_SIZE - 1;

		char *body = malloc(content_length+1);

		if (recv(socket, body, content_length, 0) == -1) {
			perror("[Server] Error receiving request body");
			return 1;
		}

		strncpy(request.body, body, sizeof(request.body));
		request.body[content_length] = '\0';
		request.body_length = content_length;
		free(body);
	}

	response_t response = {};
	response.status = NOT_FOUND;
	response.body_length = 0;
	printf("Endpoints\n");
	printf("Path: %s\n", request.path);
	if(request.method  == GET && strcmp("/", request.path) == 0) {
		response.status = OK;
    }else if(request.method  == GET && strncmp(PATH_ECHO, request.path, strlen(PATH_ECHO)) == 0) {
		const char *body = strdup(request.path + strlen(PATH_ECHO));

    	printf("Body: %s\n", body);

		response.status = OK;
    	response.headers = headers_add(response.headers, "Content-Type", "text/plain");
    	strncpy(response.body, body, strlen(body));
    	response.body_length = strlen(body);
    }else if(request.method  == GET && strcmp(PATH_USER_AGENT, request.path) == 0) {
		const char *body = strdup(headers_get(request.headers, "User-Agent"));

    	printf("User agent: %s\n", body);

    	response.status = OK;
    	response.headers = headers_add(response.headers, "Content-Type", "text/plain");
    	strncpy(response.body, body, sizeof(response.body));
    	response.body_length = strlen(body);
    }else if(strncmp(PATH_FILES, request.path, strlen(PATH_FILES)) == 0){
		const char *request_path = request.path + strlen(PATH_FILES);
    	if(request.method  == GET) {
		    const unsigned char* body = read_file_to_response(request_path, &response.status, &response.body_length);
    		if (body != NULL) {
    			response.status = OK;
    			response.headers = headers_add(response.headers, HEADER_KEY_CONTENT_TYPE, HEADER_VAL_APPLICATION_OCTET_STREAM);
    			memcpy(response.body, body, response.body_length);
			}
    	}
    	else if(request.method == POST) {
		    const int file = open(request_path, O_WRONLY | O_CREAT | O_TRUNC);
    		if (file == -1) {
    			perror("Error opening file");
    		}else {
    			printf("Body : %s\n", (char*)request.body);
    			if(write(file, request.body, request.body_length) == -1) {
    				perror("Error writing file");
    			}else {
    				response.status = CREATED;
    			};
    			close(file);
    		}

    	}
    }

	if(strlen(response.body) > 0) {

		char* encodings = headers_get(request.headers, HEADER_KEY_ACCEPT_ENCODING);
		if(encodings) {
			encoder_t encoder = NULL;

			char* encoding = strtok(encodings, COMMA);
			while(encoding) {
				while (*encoding == ' ') {
					++encoding;
				}
				encoder = get_encoder(encoding);
				if(encoder) {
					response.headers = headers_add(response.headers, HEADER_KEY_CONTENT_ENCODING, encoding);
					break;
				}
				encoding = strtok(NULL, COMMA);
			}

			if(encoder) {
				size_t buffer_size = response.body_length + 128;
				unsigned char* buffer = malloc(buffer_size);
				unsigned char* body = response.body;
				response.body_length = encoder(body, response.body_length, buffer, buffer_size);
				memcpy(response.body, buffer, response.body_length);
			}
		}

		response.headers = headers_add_number(response.headers, HEADER_KEY_CONTENT_LENGTH, response.body_length);
	}

	memset(buffer, 0, sizeof(buffer));
	sprintf(buffer, "%s %d %s\r\n", HTTP_VERSION, status_to_code(response.status), status_to_phrase(response.status));
	//printf("Send 1 line : 1 header : %s -> %s", header->key, header->value);

	if (send(socket, buffer, strlen(buffer), 0) == -1) {
		perror("[Server] Error sending response header");
		return 1;
	}

	printf("Send headers\n");

	header_t *header = response.headers;
	while (header) {
		printf("Header : key=`%s` value=`%s`\n", header->key, header->value);
		memset(buffer, 0, sizeof(buffer));
		sprintf(buffer, "%s: %s\r\n", header->key, header->value);
		if (send(socket, buffer, strlen(buffer), 0) == -1) {
			perror("[Server] Error sending response header");
			return 1;
		}
		header = header->next;
	}

	printf("Send headers end\n");

	if (send(socket, "\r\n", 2, 0) == -1) {
		perror("[Server] Error sending response header end");
		return 1;
	}

	printf("Send body\n");

	if (strlen(response.body) > 0) {
		if (send(socket, response.body, response.body_length, 0) == -1) {
			perror("[Server] Error sending response body");
			return 1;
		}
	}

	//free(request.path);
	clean_headers(request.headers);
	clean_headers(response.headers);
	return 0;
}


//void compress_gzip(const char *input, int input_len, unsigned char **output, int *output_len) {
//	// Estimer une taille maximale pour les données compressées
//	uLongf max_compressed_size = compressBound(input_len);
//
//	// Allouer de la mémoire pour le résultat compressé
//	*output = malloc(max_compressed_size);
//	if (*output == NULL) {
//		fprintf(stderr, "Erreur d'allocation de mémoire pour la compression\n");
//		exit(EXIT_FAILURE);
//	}
//
//	// Compresser les données
//	int result = compress2(*output, &max_compressed_size, (const Bytef *)input, input_len, Z_BEST_COMPRESSION);
//	if (result != Z_OK) {
//		fprintf(stderr, "Erreur lors de la compression : %d\n", result);
//		free(*output);
//		*output = NULL;
//		*output_len = 0;
//		exit(EXIT_FAILURE);
//	}
//
//	// Réduire la taille réelle des données compressées
//	*output_len = max_compressed_size;
//}

/*
int compressToGzip(unsigned char* input, int inputSize, unsigned char** output_pointer)
{
	int output_size = 128 + inputSize;
	unsigned char *output = malloc(output_size);

	z_stream zs;
	zs.zalloc = Z_NULL;
	zs.zfree = Z_NULL;
	zs.opaque = Z_NULL;
	zs.avail_in = inputSize;
	zs.next_in = input;
	zs.avail_out = output_size;
	zs.next_out = output;

	// hard to believe they don't have a macro for gzip encoding, "Add 16" is the best thing zlib can do:
	// "Add 16 to windowBits to write a simple gzip header and trailer around the compressed data instead of a zlib wrapper"
	deflateInit2(&zs, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 15 | 16, 8, Z_DEFAULT_STRATEGY);
	deflate(&zs, Z_FINISH);
	deflateEnd(&zs);

	*output_pointer = output;
	return (zs.total_out);
}
*/