#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

int main()
{
	int server_socket;
	server_socket = socket(AF_INET, SOCK_STREAM, 0);

	struct sockaddr_in server_address;
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(9002);
	server_address.sin_addr.s_addr = INADDR_ANY;

	int connection_status = connect(server_socket, (struct sockaddr*)(&server_address), sizeof(server_address));

	if(connection_status < 0)
	{
		perror("Connection error");
	}

	char buffer[256];
	read(server_socket, &buffer, sizeof(buffer));

	printf("Server message: %s\n",buffer);

	close(server_socket);
	return 0;
}