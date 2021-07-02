#ifndef _ftp_
#define _ftp_

//Standard include
#include <stdlib.h> /* for atoi() and exit() */
#include <string.h> /* for memset() */
#include <stdio.h> /* for printf() and fprintf() */
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/select.h>

//Linux include
#include <sys/socket.h> /* for socket(), bind(), sendto() and recvfrom() */
#include <arpa/inet.h> /* for sockaddr_in and inet_ntoa() */
#include <unistd.h> /* for close() */

//Windows include
//#include <io.h>
//#include <process.h>
//#include <windows.h>
//#include <winsock.h>

//Define error messages
#define BIND_ERROR "bind"
#define SOCKET_ERROR1 "socket"
#define LISTEN_ERROR "listen"
#define SEND_ERROR "send"
#define RECEIVE_ERROR "recv"
#define ACCEPT_ERROR "accept"
#define CLOSE_ERROR "close"
#define CONNECT_ERROR "connect"
#define REUSE_ERR "Reuse socket failed!\r\n"
#define NO_ARG "No argument provided.\r\n"
#define NO_MSG "No message to send.\r\n"
#define NO_NAME "Recieved Name : No such username\r\n"

#define ACC_CLT "Accept client 127.0.0.1 on TCP port 21.\r\n"
#define CLT_LFT "Client left the server.\r\n"

#define MAX 255 /* Longest string to command */

//Define structure of ftp client
struct FtpClient
{
	int 		_client_socket;//Client's socket
    char		_ip[MAX];//Client's ip
	int  		_data_socket;//Data socket
	int 		_dataport;//Data port which is established to send data
};

//Define all functions
int startup(int);
int start_data_port(int, char*);

void recv_msg(int, char**, char**, char**);
void send_msg(int, char*);
int handle_command(struct FtpClient*);

void handle_MKD(int, char*);
void handle_DELE(int, char*);
void handle_RNFR(int, char*);
void handle_RNTO(int, char*, char*);
void handle_PUT(int,char*, int);
int handle_RETR(int,char*);
void handle_PWD(int);
void handle_CWD(int, char*);
void handle_LIST(int);
int handle_PORT(struct FtpClient*, char*);
int handle_PASS(int, char*);
void handle_USER(int, char*);
void handle_TYPE(int, char*);

char* _substring(char*, int, int);
int _find_first_of(char*, char);
void sleep_ms(unsigned int);
int timeDiff(struct timeval*, struct timeval*);

#endif