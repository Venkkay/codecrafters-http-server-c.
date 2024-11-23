#include <stdio.h>
#include <stdlib.h>
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

#define PORT 4221
#define BUFFER_SIZE 1024
#define MAX_READ_SIZE 10  // Taille maximale du tableau
#define CRLF "\r\n"

int create_server_socket(void);
void accept_new_connection(int server_socket, fd_set *all_sockets, int *fd_max);
void read_data_from_socket(int socket, fd_set *all_sockets, int fd_max, int server_socket);
int split_into_array(const char *str, const char *delim, char result[][BUFFER_SIZE]);
void get_value_in_str(const char *input, char *output, int max_len, char *prefix);



int main() {
	// Disable output buffering
	setbuf(stdout, NULL);
 	setbuf(stderr, NULL);

	// You can use print statements as follows for debugging, they'll be visible when running tests.
	printf("Logs from your program will appear here!\n");

	// Uncomment this block to pass the first stage

	 int server_fd, client_addr_len, client_fd, status;
	 struct sockaddr_in client_addr;

	// Pour surveiller les sockets clients :
	fd_set all_sockets; // Ensemble de toutes les sockets du serveur
	fd_set read_fds;    // Ensemble temporaire pour select()
	int fd_max;         // Descripteur de la plus grande socket
	struct timeval timer;


	 server_fd = create_server_socket();
	 if (server_fd == -1) {
		return (1);
	 }

	 int connection_backlog = 5;
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

    time_t start_time = time(NULL);
    time_t end_wait = start_time + 10;

    while(start_time < end_wait) {
    	read_fds = all_sockets;
    	// Timeout de 2 secondes pour select()
    	timer.tv_sec = 2;
    	timer.tv_usec = 0;

    	// Surveille les sockets prêtes à être lues
    	status = select(fd_max + 1, &read_fds, NULL, NULL, &timer);
    	if (status == -1) {
    		fprintf(stderr, "[Server] Select error: %s\n", strerror(errno));
    		exit(1);
    	}
    	else if (status == 0) {
    		// Aucun descipteur de fichier de socket n'est prêt pour la lecture
    		printf("[Server] Waiting...\n");
    		continue;
    	}

    	for (int i = 0; i <= fd_max; i++) {
    		if (FD_ISSET(i, &read_fds) != 1) {
    			// Le fd i n'est pas une socket à surveiller
    			// on s'arrête là et on continue la boucle
    			continue ;
    		}
    		printf("[%d] Ready for I/O operation\n", i);
    		// La socket est prête à être lue !
    		if (i == server_fd) {
    			// La socket est notre socket serveur qui écoute le port
    			accept_new_connection(server_fd, &all_sockets, &fd_max);
    		}
    		else {
    			// La socket est une socket client, on va la lire
    			read_data_from_socket(i, &all_sockets, fd_max, server_fd);
    		}
    	}
    }

}

// Renvoie la socket du serveur liée à l'adresse et au port qu'on veut écouter
int create_server_socket(void) {
	struct sockaddr_in sa;
	//int socket_fd;
	int status;
	int server_fd;

	struct sockaddr_in serv_addr = { .sin_family = AF_INET ,
								 .sin_port = htons(PORT),
								 .sin_addr = { htonl(INADDR_ANY) },
								};

	// Création de la socket
	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd == -1) {
		printf("Socket creation failed: %s...\n", strerror(errno));
		return 1;
	}
	printf("[Server] Created server socket fd: %d\n", server_fd);

	// Since the tester restarts your program quite often, setting SO_REUSEADDR
	// ensures that we don't run into 'Address already in use' errors
	int reuse = 1;
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
		printf("SO_REUSEADDR failed: %s \n", strerror(errno));
		return 1;
	}

	// Liaison de la socket à l'adresse et au port
	if (bind(server_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) != 0) {
		printf("Bind failed: %s \n", strerror(errno));
		return 1;
	}
	printf("[Server] Bound socket to localhost port %d\n", PORT);
	return (server_fd);
}


// Accepte une nouvelle connexion et ajoute la nouvelle socket à l'ensemble des sockets
void accept_new_connection(int server_fd, fd_set *all_sockets, int *fd_max)
{
	int client_fd, client_addr_len;
	struct sockaddr_in client_addr;
	char msg_to_send[BUFSIZ];
	int status;

	client_addr_len = sizeof(client_addr);
	//client_fd = accept(server_fd, NULL, NULL);
	client_fd = accept(server_fd, (struct sockaddr *) &client_addr, &client_addr_len);

	if (client_fd < 0) {
		printf("[Server] Accept error: %s\n", strerror(errno));
		return;
	}
	FD_SET(client_fd, all_sockets); // Ajoute la socket client à l'ensemble
	if (client_fd > *fd_max) {
		*fd_max = client_fd; // Met à jour la plus grande socket
	}
	printf("[Server] Accepted new connection on client socket %d.\n", client_fd);
//	memset(&msg_to_send, '\0', sizeof msg_to_send);
//	sprintf(msg_to_send, "Welcome. You are client fd [%d]\n", client_fd);
//	status = send(client_fd, msg_to_send, strlen(msg_to_send), 0);
//	if (status == -1) {
//		fprintf(stderr, "[Server] Send error to client %d: %s\n", client_fd, strerror(errno));
//	}
}

void read_data_from_socket(int socket, fd_set *all_sockets, int fd_max, int server_socket)
{

	int status;
    char buffer[BUFFER_SIZE];
	memset(&buffer, '\0', sizeof buffer);
	char message[BUFFER_SIZE+200];
    char request_read[7][BUFFER_SIZE];
    char *token;

	// Recevoir le message du serveur
    int bytes_received = recv(socket, buffer, BUFFER_SIZE - 1, 0);
    if (bytes_received <= 0) {
      	perror("Erreur lors de la récepti on du message");
	    if (bytes_received == 0) {
    		printf("[%d] Client socket closed connection.\n", socket);
	    }
	    else {
    		fprintf(stderr, "[Server] Recv error: %s\n", strerror(errno));
	    }
	    close(socket); // Ferme la socket
	    FD_CLR(socket, all_sockets); // Enlève la socket de l'ensemble
    }
	buffer[bytes_received] = '\0'; // Terminer la chaîne reçue
	printf("[%d] Got message: %s", socket, buffer);

	int i = 0;
    split_into_array(buffer, CRLF, request_read);
	for (i = 0; i < 7; i++) {
		printf("%d : %s\n", i, request_read[i]);
	}
    char splitedRequestLine[5][BUFFER_SIZE];
    char *delimiter = " ";
    //str_split(request_read[0], delimiter, path, 2);
    split_into_array(request_read[0], delimiter, splitedRequestLine);
    char *path = splitedRequestLine[1];
    if (strcmp(&path[strlen(path)-1], "/") != 0) {
    	path[strlen(path)] = '/';
    	path[strlen(path) + 1] = '\0';
    }
	printf("Path : %s\n", path);
	printf("Method : %s\n", splitedRequestLine[0]);

    // Envoyer un message au client
	memset(&message, '\0', sizeof message);

    if(strncmp(splitedRequestLine[0], "GET", 3) == 0 && strcmp(path, "/") == 0) {
      strncpy(message, "HTTP/1.1 200 OK\r\n\r\n", sizeof(message));
    }else if(strncmp(splitedRequestLine[0], "GET", 3) == 0 && strncmp(path, "/echo/", strlen("/echo/")) == 0) {
    	char splitedPath[10][BUFFER_SIZE];
    	char *delimiterPath = "/";
    	split_into_array(path, delimiterPath, splitedPath);
    	printf("Path : %s\n", splitedPath[0]);
    	printf("Path : %s\n", splitedPath[1]);
    	printf("Path : %s\n", splitedPath[2]);
    	snprintf(message, BUFFER_SIZE+200, "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: %lu\r\n\r\n%s", strlen(splitedPath[1]), splitedPath[1]);
    }else if(strncmp(splitedRequestLine[0], "GET", 3) == 0 && strcmp(path, "/user-agent/") == 0) {
    	char userAgent[BUFFER_SIZE];
        get_value_in_str(request_read[2], userAgent, BUFFER_SIZE, "User-Agent: ");
    	snprintf(message, BUFFER_SIZE+100, "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: %lu\r\n\r\n%s", strlen(userAgent), userAgent);
    }else if(strncmp(splitedRequestLine[0], "GET", 3) == 0 && strncmp(path, "/files/", strlen("/files/")) == 0){
    	char splitedPath[10][BUFFER_SIZE];
    	char *delimiterPath = "/";
    	split_into_array(path, delimiterPath, splitedPath);
    	printf("Path : %s\n", splitedPath[0]);
    	printf("Path : %s\n", splitedPath[1]);
    	printf("Path : %s\n", splitedPath[2]);
        char filename[BUFFER_SIZE] = "/tmp/data/codecrafters.io/http-server-tester/";
        strcat(filename, splitedPath[1]);
        printf("Filename : %s\n", filename);
        FILE *file;
        if((file = fopen(filename, "r")) != NULL) {
            char fileBuffer[800];
            fgets(fileBuffer, 800, file);
        	snprintf(message, BUFFER_SIZE+100, "HTTP/1.1 200 OK\r\nContent-Type: application/octet-stream\r\nContent-Length: %lu\r\n\r\n%s", strlen(fileBuffer), fileBuffer);
        	fclose(file);
        }
        else{
        	strncpy(message, "HTTP/1.1 404 Not Found\r\n\r\n", sizeof(message)-1);
        }
    }else if(strncmp(splitedRequestLine[0], "POST", 4) == 0 && strncmp(path, "/files/", strlen("/files/")) == 0){
    	char splitedPath[10][BUFFER_SIZE];
    	char *delimiterPath = "/";
    	split_into_array(path, delimiterPath, splitedPath);
    	printf("Path : %s\n", splitedPath[0]);
    	printf("Path : %s\n", splitedPath[1]);
    	printf("Path : %s\n", splitedPath[2]);
    	char filename[BUFFER_SIZE] = "/tmp/data/codecrafters.io/http-server-tester/";
    	strcat(filename, splitedPath[1]);
    	printf("Filename : %s\n", filename);
        int index_data = 0;
        for(int i = 0; i < 7; i++){
        	if(request_read[i][0] != '\0'){
                  index_data = i;
            }
        }
    	FILE *file;
    	file = fopen(filename, "w");
    	printf("Data : %s\n", request_read[index_data]);
    	fprintf(file, "%s", request_read[index_data]);
    	snprintf(message, BUFFER_SIZE+100, "HTTP/1.1 201 Created\r\n\r\n");
    	fclose(file);
     }
    else{
      strncpy(message, "HTTP/1.1 404 Not Found\r\n\r\n", sizeof(message)-1);
    }
    status = send(socket, message, strlen(message), 0);
//	if (status == -1) {
//		fprintf(stderr, "[Server] Send error to client fd %d: %s\n", socket, strerror(errno));
//	}
    printf("Message envoyé au client .\n");


}


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

void get_value_in_str(const char *input, char *output, int max_len, char *prefix) {
	const char *start = strstr(input, prefix); // Trouver "User-Agent: "
	if (start != NULL) {
		start += strlen(prefix); // Avancer au début de la valeur
		const char *end = strstr(start, "\r\n"); // Trouver la fin de la ligne
		if (end != NULL) {
			int len = end - start;
			if (len >= max_len) len = max_len - 1; // Limiter la taille pour éviter un débordement
			strncpy(output, start, len);
			output[len] = '\0'; // Assurez-vous que la chaîne est terminée
		} else {
			// Si "\r\n" est introuvable, copier jusqu'à la fin
			strncpy(output, start, max_len - 1);
			output[max_len - 1] = '\0';
		}
	} else {
		// Si "User-Agent: " est introuvable
		output[0] = '\0';
	}
}

