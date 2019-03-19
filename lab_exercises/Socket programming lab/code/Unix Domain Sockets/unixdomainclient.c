#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>
#include <sys/un.h>
#define MAXLINE 1000

void
str_cli(FILE *fp, int sockfd)
{
int n;
	char	sendline[MAXLINE], recvline[MAXLINE];

	while (fgets(sendline, MAXLINE, fp) != NULL) {

		write(sockfd, sendline, strlen(sendline));

		if ((n=read(sockfd, recvline, MAXLINE)) == 0)
			{
			printf("str_cli: server terminated prematurely\n");
			exit(0);
			}		
		recvline[n]='\0';
		fputs(recvline, stdout);
	}
}


int
main(int argc, char **argv)
{
	int					sockfd;
	struct sockaddr_un	servaddr;

	sockfd = socket(AF_LOCAL, SOCK_STREAM, 0);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sun_family = AF_LOCAL;
	strcpy(servaddr.sun_path, argv[1]);

	connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr));

	str_cli(stdin, sockfd);		/* do it all */

	exit(0);
}
