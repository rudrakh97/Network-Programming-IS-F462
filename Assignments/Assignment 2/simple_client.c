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
#define MAXUSERNAMESIZE 50

int server_socket;

void exit_handler(int sig_no)
{
	// close(server_socket);
	printf("\n>> Client closed\n");
	signal(sig_no, SIG_DFL);
	raise(sig_no);
}

int main()
{
	// Handle closing of server
	signal(SIGINT, exit_handler);
	signal(SIGQUIT, exit_handler);
	signal(SIGTSTP, exit_handler);

	server_socket = socket(AF_INET, SOCK_STREAM, 0);

	struct sockaddr_in server_address;
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(9090);
	server_address.sin_addr.s_addr = INADDR_ANY;

	int connection_status = connect(server_socket, (struct sockaddr*)(&server_address), sizeof(server_address));

	char buffer[MAXMSGSIZE], uname[MAXUSERNAMESIZE];
	memset(buffer, '\0', sizeof(buffer));

	if(connection_status < 0)
	{
		perror(">> Connection error");
	}

	else
	{
		recv(server_socket, buffer, sizeof(buffer), 0);
		printf(">> Server: %s\n", buffer);
	}
	
	int registered = 0;

	while(1)
	{
		if(!registered)
			printf(">> User: ");
		else
			printf(">> %s: ", uname);
		memset(buffer, '\0', sizeof(buffer));
		gets(buffer);
		send(server_socket, buffer, sizeof(buffer), 0);
		memset(buffer, '\0', sizeof(buffer));
		recv(server_socket, buffer, sizeof(buffer), 0);
		puts(buffer);
		if(strstr(buffer, "Registering as")) // Registration confirmation
		{
			registered = 1;
			int i = 15;
			while(i-15 < MAXUSERNAMESIZE && buffer[i]!='.')
			{
				uname[i-15] = buffer[i];
				i++;
			}
			printf("Updating username as %s\n", uname);
		}
	}
	return 0;
}