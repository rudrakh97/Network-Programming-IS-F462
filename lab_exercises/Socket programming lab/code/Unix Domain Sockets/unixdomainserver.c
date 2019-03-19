#include <signal.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <unistd.h>
#include <strings.h>
#include <sys/un.h>

void str_echo(int sockfd)
{
	ssize_t n;
	char buf[1000];
	while((n=read(sockfd,buf,1000))>0)
	{
	buf[n]='\0';
	write(sockfd,buf,n);
	printf("%s\n",buf);
	}
}

int
main(int argc, char **argv)
{
	int					listenfd, connfd;
	pid_t				childpid;
	socklen_t			clilen;
	struct sockaddr_un	cliaddr, servaddr;
	void				sig_chld(int h)
	{
		return;
	}

	listenfd = socket(AF_LOCAL, SOCK_STREAM, 0);

	unlink(argv[1]);
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sun_family = AF_LOCAL;
	strcpy(servaddr.sun_path, argv[1]);

	bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr));

	listen(listenfd, 10);

	signal(SIGCHLD, sig_chld);

	for ( ; ; ) {
		clilen = sizeof(cliaddr);
		if ( (connfd = accept(listenfd, (struct sockaddr *) &cliaddr, &clilen)) < 0) {
			printf("connection error\n");
				continue;		/* back to for() */
		}

		if ( (childpid = fork()) == 0) {	/* child process */
			close(listenfd);	/* close listening socket */
			str_echo(connfd);	/* process the request */
			exit(0);
		}
		close(connfd);			/* parent closes connected socket */
	}
}
