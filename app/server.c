#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#define BUFFER_SIZE 1024
#define MAX_READ_SIZE 10  // Taille maximale du tableau
#define CRLF "\r\n"

//void str_split(const char* str, char delim, char* path, int index_delimiter) {
//	int delimiter_count = 0;
//	int j = 0;
//	int k = 0;
//	while (str[k] != '\0') {
//		if (str[k] == delim) {
//            delimiter_count++;
//            if(delimiter_count == index_delimiter) {
//              path[j] = '\0';
//              break;
//            }
//            path = "";
//			j = 0;
//		}else{
//			path[j] = str[k];
//			j++;
//        }
//		k++;
//	}
//}

int split_into_array(const char *str, const char *delim, char result[][BUFFER_SIZE]) {
	char buffer[BUFFER_SIZE]; // Copie de la chaîne originale pour ne pas la modifier
	strncpy(buffer, str, sizeof(buffer));
	buffer[sizeof(buffer) - 1] = '\0'; // Sécurité contre débordement

	int count = 0; // Compteur de tokens
	char *part = strtok(buffer, delim);

	while (part != NULL) {
		if (count >= MAX_READ_SIZE) { // Empêcher de dépasser la capacité
			fprintf(stderr, "Erreur : trop de tokens\n");
			return -1;
		}
		strncpy(result[count], part, BUFFER_SIZE); // Copier le token dans le tableau
		result[count][BUFFER_SIZE - 1] = '\0';     // Sécurité de fin de chaîne
		count++;
		part = strtok(NULL, delim);
	}

	return count; // Retourner le nombre de tokens
}

int main() {
	// Disable output buffering
	setbuf(stdout, NULL);
 	setbuf(stderr, NULL);

	// You can use print statements as follows for debugging, they'll be visible when running tests.
	printf("Logs from your program will appear here!\n");

	// Uncomment this block to pass the first stage

	 int server_fd, client_addr_len, client_fd;
	 struct sockaddr_in client_addr;


	 server_fd = socket(AF_INET, SOCK_STREAM, 0);
	 if (server_fd == -1) {
	 	printf("Socket creation failed: %s...\n", strerror(errno));
	 	return 1;
	 }

	 // Since the tester restarts your program quite often, setting SO_REUSEADDR
	 // ensures that we don't run into 'Address already in use' errors
	 int reuse = 1;
	 if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
	 	printf("SO_REUSEADDR failed: %s \n", strerror(errno));
	 	return 1;
	 }

	 struct sockaddr_in serv_addr = { .sin_family = AF_INET ,
	 								 .sin_port = htons(4221),
	 								 .sin_addr = { htonl(INADDR_ANY) },
	 								};

	 if (bind(server_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) != 0) {
	 	printf("Bind failed: %s \n", strerror(errno));
	 	return 1;
	 }

	 int connection_backlog = 5;
	 if (listen(server_fd, connection_backlog) != 0) {
	 	printf("Listen failed: %s \n", strerror(errno));
	 	return 1;
	 }

	 printf("Waiting for a client to connect...\n");
	 client_addr_len = sizeof(client_addr);

	 client_fd = accept(server_fd, (struct sockaddr *) &client_addr, &client_addr_len);

	if (client_fd < 0) {
          perror("Erreur lors de l'acceptation de la connexion");
          close(server_fd); return 1;
    }
	 printf("Client connected\n");

	char buffer[BUFFER_SIZE];
    char request_read[7][BUFFER_SIZE];
    char *token;
	// Recevoir le message du serveur
    int bytes_received = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);
    if (bytes_received < 0) {
      perror("Erreur lors de la récepti on du message");
    }
	buffer[bytes_received] = '\0'; // Terminer la chaîne reçue
	printf("Message du serveur : %s\n\n", buffer);
//	token = strtok(buffer, CRLF);
//	int i = 0;
//	while (token != NULL) {
//		strcpy(request_read[i], token);
//		token = strtok(NULL, CRLF);
//		i++;
//	}
	int i = 0;
    split_into_array(buffer, CRLF, request_read);
	for (i = 0; i < 7; i++) {
		printf("%d : %s\n", i, request_read[i]);
	}
    char splitedLine[5][BUFFER_SIZE];
    char *delimiter = " ";
    //str_split(request_read[0], delimiter, path, 2);
    split_into_array(request_read[0], delimiter, splitedLine);
    char *path = splitedLine[1];
	printf("Path : %s\n", path);
    // Envoyer un message au client
    char message[100];
    message[0] = '\0';
    if(strcmp(path, "/") == 0) {
      strncpy(message, "HTTP/1.1 200 OK\r\n\r\n", sizeof(message));
    }
    else{
      strncpy(message, "HTTP/1.1 404 Not Found\r\n\r\n", sizeof(message)-1);
    }
    send(client_fd, message, strlen(message), 0);
    printf("Message envoyé au client .\n");

	 close(server_fd);

	return 0;
}

