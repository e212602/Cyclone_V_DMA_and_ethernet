/*
 ============================================================================
 Name        : fpga_dma.c
 Author      : Mahmoud Alasmar
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */


#define __USE_GNU
#define _GNU_SOURCE
#include <sched.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <stdint.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include<arpa/inet.h>
#define DMA_0_BASE 0xc0000000
#define DMA_0_SPAN 48
#define MM_BRIDGE_1_BASE 0xc0000020
#define MM_BRIDGE_1_SPAN 16

#define OnChipRAM	0xFFFF0000
#define RAMSPAN		65536
#define IPADDR		"10.144.166.6"
#define PORTNUM		1234
unsigned int buff[4096] = {0};

int* dma_addr;
int* onchip_virt;
int flag = 0;





void *f1(void)
{
	int fd = -1;
	if ((fd = open("/dev/mem", O_RDWR|O_SYNC)) < 0 )
	{
		printf("ARM CORE: Error opening file. \n");
		return (-1);
	}
	dma_addr = mmap(NULL,DMA_0_SPAN,(PROT_READ|PROT_WRITE),MAP_SHARED,fd,DMA_0_BASE);
	onchip_virt = mmap(NULL,RAMSPAN,(PROT_READ|PROT_WRITE),MAP_SHARED,fd,OnChipRAM);

	volatile  int* readWord;
	volatile  int* count;
	readWord = dma_addr;
	*(readWord+8) = 0;

	while(1)
	{
	*(readWord+3) = (1 << 14);
	*(readWord) = 0x00000000;
	*(readWord+1) = 0x00000000;
	*(readWord+2) = 0xffff0000;
	while(*(readWord+8) != 4096 ){};
	//Enable DMA
	*(readWord+6) = 0x0000008C;
	//Poll DMA FLAG
	while((*(readWord) & 0x1) != 0x1 ){};
	*(readWord+6) = 0x00000084;
	*(readWord+8) = 0;
	while(flag == 1){};
	memcpy(buff, onchip_virt,16384);
	flag = 1;
	}
	if ((fd = close(fd)) < 0 )
	{
		printf("ARM CORE: Error closing Test file. \n");
		return (-1);
	}

}

void *f2(void)
{
	 int sockfd, newsockfd, portno;
	 struct sockaddr_in serv_addr, cli_addr;
	 sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	 if (sockfd < 0)
	 {
	   puts("ERROR opening socket");
	   exit(-1);
	 }
	 bzero((char *) &serv_addr, sizeof(serv_addr));
	 portno = PORTNUM;
	 serv_addr.sin_family = AF_INET;
	 serv_addr.sin_addr.s_addr = inet_addr(IPADDR);;
	 serv_addr.sin_port = htons(portno);


	 if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
	 {
		 printf("\nConnection Failed \n");
		 exit(-1);
	 }
	 int k;
	 unsigned int val;
	 unsigned int rev_val;
	 while(1)
	 {
	 while(flag == 0){puts("12");};
	 for(k=0;k<4096;k++)
	 {
		 val = buff[k];
		 asm("REV %1,%0":"=r"(rev_val):"r"(val));
		 buff[k] = rev_val;
	 }

	 if( (send(sockfd , buff ,sizeof(buff), 0 )) < 0)
	 {
		 puts("error in send");
		 exit(-1);
	 }
	 flag = 0;
	 }


}


long int gTime(void)
{
     struct timeval rtclk;
     gettimeofday(&rtclk, NULL);
	 return rtclk.tv_usec;

}

int main(int argc, char *argv[]) {
	long int start2,stop2,stop3;

	char* ip_add;

	if(argc < 2)
	{
		puts("Please Enter Target IP Address");
		return (-1);
	}
	else
		ip_add = argv[1];


	cpu_set_t core0,core1;
	CPU_ZERO(&core0);
	CPU_SET(0, &core0);
	//CPU_ZERO(&core0);
	CPU_SET(1, &core0);

	struct sched_param p_params;
	p_params.sched_priority = 99;
	sched_setaffinity(getpid(), sizeof(cpu_set_t),&core0);
	sched_setscheduler(getpid(), SCHED_FIFO,&p_params);

/*	pthread_t th1, th2;


	if (pthread_create (&th1, NULL, f1, NULL) < 0)
	{
			fprintf (stderr, "pthread_create error for thread 1\n");
			exit (1);
	}
	pthread_setschedparam(th1, SCHED_FIFO, &p_params);
	pthread_setaffinity_np(th1, sizeof(cpu_set_t), &core0);

	if (pthread_create (&th2, NULL, f2, NULL) < 0)
	{
			fprintf (stderr, "pthread_create error for thread 1\n");
			exit (1);
	}
	pthread_setschedparam(th2, SCHED_FIFO, &p_params);
	pthread_setaffinity_np(th2, sizeof(cpu_set_t), &core1);*/
	int sockfd, newsockfd, portno;
	struct sockaddr_in serv_addr, cli_addr;
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0)
	{
	puts("ERROR opening socket");
	exit(-1);
	}
	bzero((char *) &serv_addr, sizeof(serv_addr));
	portno = PORTNUM;
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(ip_add);
	serv_addr.sin_port = htons(portno);


	if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
	{
	printf("\nConnection Failed \n");
	exit(-1);
	}

	int fd = -1;
	if ((fd = open("/dev/mem", O_RDWR|O_SYNC)) < 0 )
	{
	printf("ARM CORE: Error opening file. \n");
	return (-1);
	}
	dma_addr = mmap(NULL,DMA_0_SPAN,(PROT_READ|PROT_WRITE),MAP_SHARED,fd,DMA_0_BASE);
	onchip_virt = mmap(NULL,RAMSPAN,(PROT_READ|PROT_WRITE),MAP_SHARED,fd,OnChipRAM);

	volatile  int* readWord;
	volatile  int* count;
	readWord = dma_addr;
	*(readWord+8) = 0;
	int k;
	unsigned int val;
	unsigned int rev_val;

	while(1)
	{
	*(readWord+3) = (1 << 14);
	*(readWord) = 0x00000000;
	*(readWord+1) = 0x00000000;
	*(readWord+2) = 0xffff0000;
	while(*(readWord+8) != 4096 ){};
	//Enable DMA
	*(readWord+6) = 0x0000008C;
	//Poll DMA FLAG
	while((*(readWord) & 0x1) != 0x1 ){};
	*(readWord+6) = 0x00000084;
	*(readWord+8) = 0;
	//while(flag == 1){};
	memcpy(buff, onchip_virt,16384);
	//flag = 1;


/*	for(k=0;k<4096;k++)
	{
	val = buff[k];
	printf("before = 0x%x\n",val);
	//asm("REV %1,%0":"=r"(rev_val):"r"(val));
	buff[k] = rev_val;
	printf("after = 0x%x\n",rev_val);
	}
	return 0;*/

	while( (send(sockfd , buff ,sizeof(buff), 0 )) < 0)
	{
	puts("error in send");
	//exit(-1);
	if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
	{
	printf("\nConnection Failed \n");
	//exit(-1);
	}
	sleep(0.001);
	}

	}




	if ((fd = close(fd)) < 0 )
	{
	printf("ARM CORE: Error closing Test file. \n");
	return (-1);
	}

/*  (void) pthread_join (th1, NULL);
   (void) pthread_join (th2, NULL);*/

	return EXIT_SUCCESS;
}
