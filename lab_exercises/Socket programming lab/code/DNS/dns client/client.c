#include<stdio.h>
#include<stdlib.h>
#include<strings.h>
#include<string.h>
#include<netdb.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<arpa/nameser.h>
#include<sys/types.h>
#include<resolv.h>
#include"msgformat.h"
#define SERV_PORT 53
#define STAN_PORT 53

char *rcodestr[] = {"No error condition","Format error","Server failure",
		    "Name Error", "Not Implemented", "Refused" };

void DieWithError(char* error){
	perror(error);
	exit(1);
}

void printfunc(char *offstr,char *start) {
	unsigned int p;char *newstr;
	while(*offstr != 0) {
		if( ((*offstr) & ((uint8_t)0xC0)) ) {
			newstr = start + ((ntohs(*((uint16_t*)(offstr)))) & (uint16_t)0x3fff) ;
			printfunc(newstr,start);
			return;
		}
		for(p=0;p<(uint8_t)*offstr;p++)
			printf("%c",*(offstr+p+1));
		printf(".");
		offstr+=*offstr+1;
	}
}
int recflag;
char recip[5][25];
int recipnum;
//Usage ./resolver 172.24.26465 mit.edu
void dg_cli(char*, int,  struct sockaddr_in*,socklen_t);
int main(int argc, char* argv[]){
	if(argc != 2){
		printf("hostname required\n");
		exit(0);
	}
	//recflag = atoi(argv[2]);
	FILE* fp;
	//put error condtn below
	fp = fopen("resolv.conf", "r");
	char str[30],serv[30];
	int flag = 0;
	while(fgets(str,29,fp) != NULL){
		if(strncmp("nameserver",str,10) == 0){
			flag = 1;
			break;
		}
	}
	if(!flag)
		DieWithError("No nameserver\n");
	int sockfd;
	struct sockaddr_in servaddr;
	char addr[30];
	char* addr_temp = str;
	addr_temp += 11;
	int i = 0;
	while(*addr_temp != '\n')
		addr[i++] = *addr_temp++;
	addr[i] = '\0';
	//printf("Address:%s\n",addr);
	if((sockfd = socket(AF_INET,SOCK_DGRAM,0)) < 0)
		DieWithError("socket");
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(SERV_PORT);
	/*if(inet_pton(AF_INET,addr,&servaddr.sin_addr) < 0)
		DieWithError("inet_pton");*/
	servaddr.sin_addr.s_addr = inet_addr(addr);
	dg_cli(argv[1],sockfd,&servaddr,sizeof(servaddr));
	exit(0);
}

int parse(char *recvline, int answer_offset){
	/*int w;char c ;
	for(w=0;w<512;w++) {
		c = recvline[w];
		printf("%d_",c);
	}
	printf("\n");*/
	//Printing each field
	int authans,addans,i;
	char *strtmp = recvline;
	strtmp+=2;
	printf("Authoritative answer:%s\n",(*strtmp & ((uint8_t)0x04))?"Yes":"No");
	printf("Truncated:%s\n",(*strtmp & ((uint8_t)0x02))?"Yes":"No");
	printf("Recursion desired:%s\n",(*strtmp & ((uint8_t)0x01))?"Yes":"No");
	strtmp++;
	printf("Recursion available:%s\n",(*strtmp & ((uint8_t)0x80))?"Yes":"No");
	printf("Response code:%s\n",rcodestr[(*strtmp & ((uint16_t)0xf))]);
	strtmp++;
	printf("No of Questions:%d\n",ntohs(*((uint16_t*)(strtmp))));
	strtmp+=2;
	printf("No of answers:%d\n",ntohs(*((uint16_t*)(strtmp))));
	strtmp+=2;
	printf("No of authority records:%d\n",authans=ntohs(*((uint16_t*)(strtmp))));
	strtmp+=2;
	printf("No of additional records:%d\n",addans=ntohs(*((uint16_t*)(strtmp))));
	#define OFFSET_ANCOUNT 6
	#define HEADER_LENGTH 12
	int ans=ntohs(*((uint16_t*)(recvline+OFFSET_ANCOUNT)));
	printf("No of answers:%d\n",ans);
	strtmp=recvline+answer_offset;
	int flag;
	struct in_addr inaddr;
	int numip = 0;
	for(i=0;i<ans;i++,numip++) {
		flag=1;
		while(*strtmp != 0) {
			if( ((*strtmp) & ((uint8_t)0xC0)) ) {
				strtmp+=2;
				flag=0;
				//printf("--------------\n");
				break;
			}
			strtmp+=*strtmp+1;
		}
		if(flag)
			strtmp++;
		if(ntohs(*((uint16_t*)strtmp)) != 1) {
			strtmp+=8;
			strtmp+=ntohs(*((uint16_t*)strtmp))+2;
			numip--;
			continue;
		}
		strtmp+=10;//go to rdata section
		inaddr.s_addr = *((int32_t*)strtmp);
		printf("IP address %d: %s\n",numip+1,inet_ntoa(inaddr));
		strtmp+=4;
	}
	//return;
	//if(ans==0)
	//	return;
	//Now at authority section
	char *offstr;
	unsigned int p,q;
	int rtype,rdlength;
	for(i=0;i<authans;i++) {
		printf("Authority record %d: ",i+1);
		printf("Owner name:");
		flag=1;
		while(*strtmp != 0) {
			if( ((*strtmp) & ((uint8_t)0xC0)) ) {
				flag=0;
				offstr = recvline + ((ntohs(*((uint16_t*)(strtmp)))) & (uint16_t)0x3fff) ;
				printfunc(offstr,recvline);
				strtmp+=2;
				break;
			}
			for(p=0;p<(uint8_t)*strtmp;p++)
				printf("%c",*(strtmp+p+1));

			printf(".");
			strtmp+=*strtmp+1;
		}
		printf("., ");
		if(flag)
			strtmp++;
		printf("Type:%d, ",rtype=ntohs(*((uint16_t*)(strtmp))));
		if(rtype != 2) {
			printf("error: type other than 2\n");
			//exit(0);
			strtmp+=8;
			rdlength=ntohs(*((uint16_t*)(strtmp)));
			strtmp+=rdlength+2;
			//printf("\n");
			continue;
		}
		strtmp+=8;
		rdlength=ntohs(*((uint16_t*)(strtmp)));
		printf("NS RDATA:");
		strtmp+=2;
		printfunc(strtmp,recvline);
		strtmp+=rdlength;
		printf("\n");
	}
	//Now at additional records
	recipnum = 0;
	for(i=0;i<addans;i++) {
		printf("Additional record %d: ",i+1);
		printf("Owner name:");
		flag=1;
		while(*strtmp != 0) {
			if( ((*strtmp) & ((uint8_t)0xC0)) ) {
				flag=0;
				offstr = recvline + ((ntohs(*((uint16_t*)(strtmp)))) & (uint16_t)0x3fff) ;
				printfunc(offstr,recvline);
				strtmp+=2;
				break;
			}
			for(p=0;p<(uint8_t)*strtmp;p++)
				printf("%c",*(strtmp+p+1));

			printf(".");
			strtmp+=*strtmp+1;
		}
		printf(", ");
		if(flag)
			strtmp++;
		printf("Type:%d, ",rtype=ntohs(*((uint16_t*)(strtmp))));
		if(rtype == 2) {
			strtmp+=8;
			rdlength=ntohs(*((uint16_t*)(strtmp)));
			printf("NS RDATA:");
			strtmp+=2;
			printfunc(strtmp,recvline);
			strtmp+=rdlength;
		}
		else if(rtype==1) {
			strtmp+=10;
			inaddr.s_addr = *((int32_t*)strtmp);
			printf("IP address %d: %s",i+1,inet_ntoa(inaddr));
			strcpy(recip[recipnum++],inet_ntoa(inaddr));
			strtmp+=4;
		}
		else {
			printf("error: type other than 1 or 2\n");
			strtmp+=8;
			rdlength=ntohs(*((uint16_t*)(strtmp)));
			strtmp+=rdlength+2;
			continue;
			//break;
			//exit(0);
		}
		printf("\n");
	}
	return ans;
}

void dg_cli(char* hostname, int sockfd, struct sockaddr_in* pservaddr, socklen_t servlen){
	int h,m,n,i,j,k,total_length,num_labels,len;
	char* host,temp[64];
	i=0;
	message msg;
	bzero(&msg, sizeof(msg));
	msg.id = htons(1);
	recflag = 0;
	//msg.param = htons(BIT_7);//for recursion
	//printf("recflag = %d  host:%s\n",recflag,hostname);
	msg.param = htons(recflag?BIT_7:0);
	//Start setting appropriate fields
	//While sending to our server, recursion is not desired
	//Nothing to set
	msg.noq = htons(1);
	msg.noa = msg.noau = msg.noad = htons(0);
	bzero(&msg.ques, 20*sizeof(question));
	j = total_length = k = num_labels = 0;
	host = hostname;
	for(;*host != '\0';host++){
		if(*host != '.'){
			j++;total_length++;
			temp[k++] = *host;
		}
		if(*host == '.'){
			temp[k] = '\0';//will be reqd if we remove attribute packed
			msg.ques[num_labels].length = j;
			//printf("Name is %s_",temp);
			memcpy(msg.ques[num_labels].label,temp,strlen(temp));//should be memmove()??
			num_labels++;
			j = k = 0;
		}

	}
	temp[k] = '\0';
	msg.ques[num_labels].length = j;
	memcpy(msg.ques[num_labels++].label,temp,strlen(temp));
	//printf("Total length =%d_",total_length);
	//printf("Number of labels = %d_",num_labels);
	msg.ques[num_labels].length = 0; //last label = 0
	bzero(msg.ques[num_labels].label,64);
	//printf("%d_",msg.ques[num_labels].length);
	msg.qtype = htons(1);//host address
	/*
	* QTYPE values can be found at http://www.iana.org/assignments/dns-parameters/dns-parameters.xhtml
	*/
	msg.qclass = htons(1);//Internet
	bzero(&msg.answer, 20*sizeof(record));
	bzero(&msg.authority, 20*sizeof(record));
	bzero(&msg.additional_info, 20*sizeof(record));
	char* udp_msg = (char*)malloc(512*sizeof(char));
	char* udp_temp = udp_msg;
	bzero(udp_msg, 512);
	/* Filling character array with msg */
	//printf("message que = %d_",msg.noq);
	memcpy(udp_temp,&msg.id,2);
	udp_temp += 2;
	/*uint16_t q=0x01;
	printf("\n\n%d\n\n",q);
	printf("\nmsg_id: %d\n",*((uint16_t*)(udp_temp)));*/
	memcpy(udp_temp,(char*)&msg.param,2);
	udp_temp += 2;
	memcpy(udp_temp,(char*)&msg.noq,2);
	udp_temp += 2;
	memcpy(udp_temp,(char*)&msg.noa,2);
	udp_temp += 2;
	memcpy(udp_temp,(char*)&msg.noau,2);
	udp_temp += 2;
	memcpy(udp_temp,(char*)&msg.noad,2);
	udp_temp += 2;
	m = 0;
	while(msg.ques[m].length != 0){
		//printf("lenght = %d_",msg.ques[m].length);
		memcpy(udp_temp,(char*)&msg.ques[m].length,1);
		udp_temp += 1;
		memcpy(udp_temp,msg.ques[m].label,strlen(msg.ques[m].label));
		udp_temp += strlen(msg.ques[m].label);
		//printf("hehe!...%s_",msg.ques[m].label);
		m++;
	}
	*udp_temp = 0;
	udp_temp += 1;
	memcpy(udp_temp,(char*)&msg.qtype,2);
	udp_temp += 2;
	memcpy(udp_temp,(char*)&msg.qclass,2);
	udp_temp += 2;
	*udp_temp = '\0';
	int answer_offset=udp_temp-udp_msg;
	char recvline[513];

	int ans = 0;

	if(ans==0){
		char *newtmp = udp_msg+2;
		*newtmp|=(uint8_t)(0x01);
		printf("\nSending to BITS name servers \n");
		printf("Trying %s.....\n",inet_ntoa(pservaddr->sin_addr));
	//	struct sockaddr_in servaddr;
	//	bzero(&servaddr, sizeof(servaddr));
	//	servaddr.sin_family = AF_INET;
	//	servaddr.sin_port = htons(STAN_PORT);
	//	servaddr.sin_addr.s_addr = inet_addr(recip[0]);
	//	close(sockfd);
	//	if((sockfd = socket(AF_INET,SOCK_DGRAM,0)) < 0)
	//		DieWithError("socket");
		if(sendto(sockfd,udp_msg,512,0,(struct sockaddr*)pservaddr,servlen) < 0)
			DieWithError("sendto");
		if((n = recvfrom(sockfd,recvline,512,0,NULL,NULL)) < 0)
			DieWithError("recvfrom");
		recvline[n] = '\0';
		ans = parse(recvline,answer_offset);
	}

}

