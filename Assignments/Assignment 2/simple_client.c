#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <pthread.h>

#define MAXMSGSIZE 512
#define MAXUSERNAMESIZE 50
#define SERVER_ADDRESS INADDR_ANY
#define SERVER_PORT 9090

int server_socket, registered = 0;
char uname[MAXUSERNAMESIZE];
pthread_mutex_t lock;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

void exit_handler(int sig_no)
{
	// close(server_socket);
	printf("\n>> Client closed\n");
	signal(sig_no, SIG_DFL);
	raise(sig_no);
}

void *to_server(void *args)
{
	char buffer[MAXMSGSIZE];
	while(1)
	{
		pthread_mutex_lock(&lock);
		if(!registered)
			printf(">> User: ");
		else
			printf(">> %s: ", uname);
		memset(buffer, '\0', sizeof(buffer));
		gets(buffer);
		if(strlen(buffer) < 4)
		{
			pthread_cond_wait(&cond, &lock);
			pthread_mutex_unlock(&lock);
			continue;
		}
		send(server_socket, buffer, sizeof(buffer), 0);
		if(strcmp(buffer, "LEAVE") == 0)
		{
			printf(">> Exiting ...\n");
			exit(0);
		}
		pthread_cond_wait(&cond, &lock);
		pthread_mutex_unlock(&lock);
	}
}

void *from_server(void *args)
{
	while(1)
	{
		char buffer[MAXMSGSIZE];
		memset(buffer, '\0', sizeof(buffer));
		recv(server_socket, buffer, sizeof(buffer), 0);
		pthread_mutex_lock(&lock);
		printf(">> ");
		puts(buffer);
		if(strstr(buffer, "Server: Registering as")) // Registration confirmation
		{
			registered = 1;
			int i = 23;
			while(i-23 < MAXUSERNAMESIZE && buffer[i]!='.')
			{
				uname[i-23] = buffer[i];
				i++;
			}
			printf(">> Updating username as %s\n", uname);
		}
		else if(strstr(buffer, "Server: Username updated"))
		{
			int i = 28;
			while(i-28 < MAXUSERNAMESIZE && buffer[i]!='.')
			{
				uname[i-28] = buffer[i];
				i++;
			}
			printf(">> Updating username as %s\n", uname);
		}
		pthread_cond_signal(&cond);
		pthread_mutex_unlock(&lock);
	}
}

int main()
{
	// Handle closing of server
	signal(SIGINT, exit_handler);
	signal(SIGQUIT, exit_handler);
	signal(SIGTSTP, exit_handler);

	server_socket = socket(AF_INET, SOCK_STREAM, 0);
	// fcntl(server_socket, F_SETFL, fcntl(server_socket, F_GETFL) | O_NONBLOCK);

	struct sockaddr_in server_address;
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(SERVER_PORT);
	server_address.sin_addr.s_addr = SERVER_ADDRESS;

	int connection_status = connect(server_socket, (struct sockaddr*)(&server_address), sizeof(server_address));

	memset(uname, '\0', sizeof(uname));

	if(connection_status < 0)
	{
		perror(">> Connection error");
	}

	else
	{
		int buffer[MAXMSGSIZE];
		recv(server_socket, buffer, sizeof(buffer), 0);
		printf(">> Server: %s\n", buffer);
	}

	pthread_t thread_pool[2];
	pthread_mutex_init(&lock, NULL);

	pthread_create(&thread_pool[0], NULL, to_server, NULL);
	pthread_create(&thread_pool[1], NULL, from_server, NULL);

	pthread_join(thread_pool[0], NULL);
	pthread_join(thread_pool[1], NULL);

	return 0;
}