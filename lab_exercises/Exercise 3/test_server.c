#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/epoll.h>

int main()
{
	char server_msg[256] = "Server connection successful\n";

	int server_socket;
	server_socket = socket(AF_INET, SOCK_STREAM, 0);

	struct sockaddr_in server_address;
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(12345);
	server_address.sin_addr.s_addr = INADDR_ANY;

	bind(server_socket, (struct sockaddr*)(&server_address), sizeof(server_address));

	listen(server_socket, 5);

	printf("[+] Server running on port 9090 ...\n");

	while(1)
	{
		int client_socket;
		struct sockaddr_in client_address;
		int addr_size = sizeof(struct sockaddr_in);
		client_socket = accept(server_socket, (struct sockaddr*)&client_address, &addr_size);
		if(client_socket == -1)
		{
			perror("[-] Socket connection error");
			exit(1);
		}
		printf("[+] Connected to %s on remote port %d\n", inet_ntoa(client_address.sin_addr), (int) ntohs(client_address.sin_port));
	}

	close(server_socket);
	return 0;
}