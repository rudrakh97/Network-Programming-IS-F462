#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

int main(int argc, char *argv[])
{
    // Argument error handling
    if(argc < 2)
    {
        printf("Enter a URL to check. Exiting ...\n");
        exit(1);
    }
    if(argc > 2)
    {
        printf("Program expected single argument. Found %d arguments. Exiting ...\n", argc-1);
        exit(1);
    }

    // Confirm URL
    char url[256];
    memset(url, '\0', sizeof(url));
    strncpy(url, argv[1], strlen(argv[1]));
    printf("URL to be checked: %s\nPress [ENTER] to continue...", url);
    getchar();

    // Get IP address from hostname
    struct hostent *HostInfo;
    HostInfo = gethostbyname(url);
    if(!HostInfo)
    {
        printf("Couldn't resolve host name\n");
        exit(1);
    }
    char ip[INET_ADDRSTRLEN];
    char **ptr = HostInfo->h_addr_list;
    printf("Address list: \n");
    while(*ptr != NULL)
    {
        printf("Aliases: %s\n",inet_ntop(HostInfo->h_addrtype, *ptr, ip, sizeof(ip)));
        ++ptr;
    }
    printf("IP considered: %s\n",ip);

    // TO DO: check number of possible TCP connections to ip:80

	return 0;
}
