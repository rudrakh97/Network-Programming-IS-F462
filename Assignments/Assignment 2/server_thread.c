#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <pthread.h>

#define MAXMSGSIZE 256
#define MAXTHREADS 10
#define MAXUSERNAMESIZE 50
#define TCPLIMIT 5
#define SERVER_ADDRESS INADDR_ANY
#define SERVER_PORT 9090

struct sockaddr_in server_address;
int server_socket;

int server_init()
{
	server_socket = socket(AF_INET, SOCK_STREAM, 0);

	if(server_socket == -1)
	{
		return -1;
	}

	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(SERVER_PORT);
	server_address.sin_addr.s_addr = SERVER_ADDRESS;

	if(bind(server_socket, (struct sockaddr*)&server_address, sizeof(server_address)) == -1)
	{
		return -1;
	}

	if(listen(server_socket, TCPLIMIT) == -1)
	{
		return -1;
	}

	return 0;
}

void process(char in_buffer[], char out_buffer[])
{
	//TO DO
	sprintf(out_buffer, "Message recieved: %s", in_buffer);
}

void *runnable(void *arg)
{
	printf(">> Thread initialized ...\n");
	int client_socket;
	struct sockaddr_in client_address;
	int address_size = sizeof(struct sockaddr_in), hasStarted = 0;
	char uname[MAXUSERNAMESIZE], out_buffer[MAXMSGSIZE], in_buffer[MAXMSGSIZE];
	memset(uname, '\0', sizeof(uname));
	while(1)
	{
		client_socket = accept(server_socket, (struct sockaddr*)&client_address, &address_size);
		if(client_socket == -1)
		{
			printf(">> Connection accept error ...\n");
			continue;
		}
		printf(">> Connected to %s on remote port %d\n", inet_ntoa(client_address.sin_addr), (int) ntohs(client_address.sin_port));
		memset(out_buffer, '\0',sizeof(out_buffer));
		sprintf(out_buffer,"You are now connected to the server at %s running on port %d", inet_ntoa(server_address.sin_addr), (int) ntohs(server_address.sin_port));
		send(client_socket, out_buffer, sizeof(out_buffer), 0);
		// TO DO
		hasStarted = 0;
		while(!hasStarted)
		{
			memset(in_buffer, '\0', sizeof(in_buffer));
			memset(out_buffer, '\0', sizeof(out_buffer));
			recv(client_socket, in_buffer, sizeof(in_buffer), 0);
			printf(">> Processing: %s\n", in_buffer);
			int name = 4;
			if(in_buffer[0] == 'J' && in_buffer[1] == 'O' && in_buffer[2] == 'I' && in_buffer[3] == 'N') // JOIN
			{
				while(isspace(in_buffer[name]) && name < MAXMSGSIZE)
					name++;
				if(name == MAXMSGSIZE || isspace(in_buffer[name]))
				{
					sprintf(out_buffer, "Invalid username: JOIN <ASCII character(s) with no spaces>");
					send(client_socket, out_buffer, sizeof(out_buffer), 0);
					continue;
				}
				else
				{
					for(int i=name;!isspace(in_buffer[i]) && in_buffer[i]!='\0' && i-name < MAXUSERNAMESIZE; ++i)
						uname[i-name] = in_buffer[i];
					sprintf(out_buffer, "Registering as %s. Use the command JOIN <username> again to change.", uname);
					printf(">> %s registered.\n", uname);
					hasStarted = 1;
					send(client_socket, out_buffer, sizeof(out_buffer), 0);
					break;
				}
			}
			else
			{
				sprintf(out_buffer, "Register username to send messages. >> JOIN <username>");
				send(client_socket, out_buffer, sizeof(out_buffer), 0);
			}
		}
		hasStarted = 1;
		while(hasStarted)
		{
			memset(in_buffer, '\0', sizeof(in_buffer));
			memset(out_buffer, '\0', sizeof(out_buffer));
			if(recv(client_socket, in_buffer, sizeof(in_buffer), 0) == 0)
			{
				printf(">> %s left\n", uname);
				break;
			}
			printf(">> Processing: %s\n", in_buffer);
			process(in_buffer, out_buffer);
			send(client_socket, out_buffer, sizeof(out_buffer), 0);
		}
		close(client_socket);
	}
}

void exit_handler(int sig_no)
{
	close(server_socket);
	printf("\n>> Server closed\n");
	signal(sig_no, SIG_DFL);
	raise(sig_no);
}

int main()
{
	// Handle closing of server
	signal(SIGINT, exit_handler);
	signal(SIGQUIT, exit_handler);
	signal(SIGTSTP, exit_handler);

	// Initialise server socket
	if(server_init() == -1)
	{
		perror(">> Server initialisation error");
		exit(1);
	}

	// Set client list to default values 0.0.0.0, localhost
	// client_list_init();

	pthread_t thread_pool[MAXTHREADS];

	for(int i=0;i<MAXTHREADS;++i)
	{
		pthread_create(&thread_pool[i], NULL, runnable, NULL);
	}

	for(int i=0;i<MAXTHREADS;++i)
	{
		pthread_join(thread_pool[i], NULL);
	}

	close(server_socket);

	return 0;
}