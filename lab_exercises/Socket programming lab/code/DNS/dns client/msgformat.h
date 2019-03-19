#ifndef _MSGFORMAT_H
#define _MSGFORMAT_H	1

#include<sys/types.h>
#include<stdint.h>
#include<unistd.h>

typedef struct{
	uint8_t length;
	char label[64];//max length of label
}question;

typedef struct{
	char name[255];
	uint16_t type;
	uint16_t class;
	int32_t ttl;
	uint16_t rdlength;
	char rsdata[64];
}record;
	

typedef struct{
	uint16_t id, param,noq,noa,noau,noad;
	//question ques;
	question ques[20];
	uint16_t qtype;//one qtype and qclass is enough
	uint16_t qclass;
	//record answer;
	record answer[20];
	//record authority;
	record authority[20];
	//record additional_info;	
	record additional_info[20];	
}message;

enum {
	BIT_15 = 0x0001,
	BIT_14 = 0x0002,
	BIT_13 = 0x0004,
	BIT_12 = 0x0008,
	BIT_11 = 0x0010,
	BIT_10 = 0x0020,
	BIT_9 = 0x0040,
	BIT_8 = 0x0080,
	BIT_7 = 0x0100,
	BIT_6 = 0x0200,
	BIT_5 = 0x0400,
	BIT_4 = 0x0800,
	BIT_3 = 0x1000,
	BIT_2 = 0x2000,
	BIT_1 = 0x4000,
	BIT_0 = 0x8000
};

#endif
