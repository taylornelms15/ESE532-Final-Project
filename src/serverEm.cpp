// Server side implementation of UDP client-server model
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifdef __SDSCC__
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "sds_lib.h"
#endif

#include "serverEm.h"
#include <assert.h>


#define PORT	8091

#define MAXINPUTFILESIZE 200000000

// basic
int ESE532_Server::setup_server(int avg_chunksize, const char filename[])
{

	printf("setting up sever...\n");

#ifndef __SDSCC__
	//
	int opt = 1;

	// Creating socket file descriptor
	if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 )
	{
		perror("socket creation failed");
		exit(EXIT_FAILURE);
	}


	memset(&servaddr, 0, sizeof(servaddr));

	if(setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
		perror("sockopt");


	// Filling server information
	//
	bzero(&servaddr,sizeof(servaddr));
	servaddr.sin_family = AF_INET; // IPv4
	servaddr.sin_addr.s_addr = htons(INADDR_ANY);
	servaddr.sin_port = htons(PORT);

	// Bind the socket with the server address
	if ( bind(sockfd, (const struct sockaddr *)&servaddr,sizeof(servaddr)) < 0 )
	{
		perror("bind failed");
		exit(EXIT_FAILURE);
	}

	//
	server_len = sizeof(servaddr);
#else
	sockfd = open(filename, O_RDONLY);
	if(sockfd < 0)
	{
		perror("error opening file");
		return -1;
	}
#endif
	//
	chunksize = avg_chunksize;

	//
	packets_read = 0;

	//
	printf("server setup complete!\n");

	return 0;
}

int ESE532_Server::get_packet(unsigned char* buffer)
{
#ifndef __SDSCC__
	int bytes_read = recvfrom(sockfd, (void *)buffer, chunksize+HEADER,0, ( struct sockaddr *) &servaddr,	&server_len);
#else
	// when bytes read is equal to 0 then you have finished the file or if you read less than your chunksize then tag it as done. This will stay consistent with client script
	// you may have to change this read call to something like this. int bytes_read = read(sockfd, (void*)&buffer[HEADER],chunksize);
	// then add your encoding in the first and second spots. You can look at the client code to see how its done if you want to stay consistent
	int bytes_read = read(sockfd, (void*)buffer,chunksize);
#endif
	packets_read++;
	// crash
	if( bytes_read < 0 )
	{
		perror("recvfrom failed!");
		assert(bytes_read > 0);
	}

	return bytes_read;
}
