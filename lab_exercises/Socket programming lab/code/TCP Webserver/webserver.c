/*webserve.c*/
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

int
main (int argc, char **argv)
{
  int connfd, lfd, listenfd, i = 0;
  pid_t pid, ret;
  int p[2];
  FILE *fp;
  socklen_t clilen;
  char buff[20000], *buf, ch;
  struct sockaddr_in cliaddr, servaddr;
  if ((lfd = socket (AF_INET, SOCK_STREAM, 0)) < 0)
    {
      perror ("sockfd error");
    }

  buf =
    "HTTP/1.1 200 OK\nDate: Mon, 19 Oct 2009 01:26:17 GMT\nServer: Apache/1.2.6 Red Hat\nContent-Length: 18\nAccept-Ranges: bytes\nKeep-Alive: timeout=15, max=100\nConnection: Keep-Alive\nContent-Type: text/html\n\n<html>Hello</html>";

  bzero (&servaddr, sizeof (servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl (INADDR_ANY);
  servaddr.sin_port = htons (atoi (argv[1]));
  if (bind (lfd, (struct sockaddr *) &servaddr, sizeof (servaddr)))
    {
      perror ("Bind error");
    }
  listen (lfd, 10);
  for (;;)
    {
      clilen = sizeof (cliaddr);
      if ((connfd = accept (lfd, (struct sockaddr *) &cliaddr, &clilen)) < 0)
	{
	  perror ("connection error");
	}
      if ((pid = fork ()) < 0)
	{
	  perror ("fork:");
	}
      if (pid == 0)
	{
	  close (listenfd);
	  recv (connfd, buff, sizeof (buff), 0);
	  printf ("%s\n", buff);
	  send (connfd, buf, strlen (buf), 0);
	  close (connfd);
	  exit (0);
	}

      close (connfd);

    }
