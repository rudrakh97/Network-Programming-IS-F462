#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <strings.h>
#include <sys/socket.h>

int main()
{
	char *ptr,**pptr,url[50],address[100];
	char str[INET_ADDRSTRLEN];
	struct hostent *hptr;
	printf("Enter Url:");
	while((gets(url)),!feof(stdin))
	{
		ptr=url;
		if((hptr=gethostbyname(ptr))==NULL)
		{
			continue;
		}
		strcpy(url,hptr->h_name);
		printf("official hostname:%s\n",hptr->h_name);
		for(pptr=hptr->h_aliases;*pptr!=NULL;pptr++)
		printf("alias:%s\n",*pptr);
		switch(hptr->h_addrtype)
		{
			case AF_INET:
					pptr=hptr->h_addr_list;
					for(;*pptr!=NULL;pptr++){
					strcpy(address,inet_ntop(hptr->h_addrtype,*pptr,str,sizeof(str)));
					printf("address:%s\n",inet_ntop(hptr->h_addrtype,*pptr,str,sizeof(str)));
					}
					break;
			default:
					break;

		}
		printf("\n Enter url:");
	}
	exit(0);

}
