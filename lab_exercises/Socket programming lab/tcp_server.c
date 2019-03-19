#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

int main()
{
	char server_msg[256] = "Server connection successful\n";

	int server_socket;
	server_socket = socket(AF_INET, SOCK_STREAM, 0);

	struct sockaddr_in server_address;
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(9002);
	server_address.sin_addr.s_addr = INADDR_ANY;

	bind(server_socket, (struct sockaddr*)(&server_address), sizeof(server_address));

	listen(server_socket, 5);

	int client_socket;
	client_socket = accept(server_socket, NULL, NULL);

	write(client_socket, server_msg, sizeof(server_msg));

	close(server_socket);
	return 0;
}