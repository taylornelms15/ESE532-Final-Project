#ifndef SERVER_H_
#define SERVER_H_


#include <sys/types.h>

#ifdef __SDSCC__
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#endif

#include <unistd.h>
#define HEADER 2

class ESE532_Server{
public:

    //
	int setup_server(int avg_chunksize, const char filename[]);

	//
	int get_packet(unsigned char* buffer);

	int packets_read;

	int offset;

protected:

    //
    int sockfd;

    // chunksize passed in from cli
    int chunksize;

#ifdef __SDSCC__
    // addresss information
    struct sockaddr_in servaddr;

    //
    socklen_t server_len = sizeof(servaddr);
#endif

};

#endif
