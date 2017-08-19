#include "LNC.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>


#ifndef WIN32 //linux includes

#include <sys/socket.h> //for socket() function
#include <netinet/in.h> //for sockaddr_in
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h> 

#else
	#include <windows.h>
	#include <winsock2.h>
	#include <ws2tcpip.h>
#endif

static void error_out(char * msg)
{
	printf("error in LNC.cpp: %s\n", msg);
	exit(1);
}

int asyncRead(int totalSockets, int numSeconds, int numMicroSeconds, LL_Socket_Type * sockets, int * readyBools)
{
	int i = 0;
	struct fd_set readList;
	FD_ZERO(&readList);
	int maxFD = sockets[0];

	struct timeval timeout;
	timeout.tv_sec = numSeconds;
	timeout.tv_usec = numMicroSeconds;

	for(i = 0; i < totalSockets; i++)
	{
			FD_SET(sockets[i], &readList);
			maxFD = sockets[i] > maxFD ? sockets[i] : maxFD;
	}
	int selectValue = select(maxFD + 1, &readList, (fd_set*) 0, (fd_set*) 0, &timeout);
	if(selectValue == -1)
	{
		char error_msg [] = "error in calling select";
		error_out(error_msg);
	}
	for(i = 0; i < totalSockets; i++)
	{
		readyBools[i] = FD_ISSET(sockets[i], &readList) != 0;
	}
	return selectValue;
}

static void LServer__close(LServer * self)
{
	self->socket.close(&self->socket);
}

static int LSocket__read(LSocket * self, char * buf, int maxSize)
{
	return recv(self->socket, buf, maxSize, 0);
} 
static int LSocket__write(LSocket * self, char * buf, int maxSize)
{
	return send(self->socket, buf, maxSize, 0);
}

static int LSocket__canRead(struct LSocket_struct * self, int seconds, int microSeconds){
	int ready = 0;
	asyncRead(1, seconds, microSeconds, &self->socket, &ready);
	return ready;
}

static int LServer__canAccept(struct LServer_struct * self, int seconds, int microSeconds){
	int ready = 0;
	asyncRead(1, seconds, microSeconds, &self->socket.socket, &ready);
	return ready;
}

//platform dependent ones
#ifdef WIN32

void init_LNC()
{
	WSADATA wsaData;
	int result = WSAStartup(MAKEWORD(2,2), &wsaData);
	if (result != 0)
		error_out("failure on WSAStartup() in ll_socket_windows_init()");
}


static void LSocket__close(LSocket * self)
{
	closesocket(self->socket);
}



void del_LNC()
{
	WSACleanup();
}

static void __setup(LServer * self)
{
	char buf[100];
	self->socket.socket = INVALID_SOCKET;
	struct addrinfo * result = NULL;
	struct addrinfo hints;
	int iResult;
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;
	_itoa(self->port, buf, 10);
	// Resolve the server address and port
	
	iResult = getaddrinfo(NULL, buf, &hints, &result);
	if(iResult != 0)
	{
		sprintf(buf, "getaddrinfo failed with error: %i\n", iResult);
		error_out(buf);
		del_LNC();
	}

	// Create a SOCKET for connecting to server
	self->socket.socket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (self->socket.socket == INVALID_SOCKET)
	{
		sprintf(buf, "getaddrinfo failed with error: %i\n", WSAGetLastError());
		freeaddrinfo(result);
		del_LNC();
		error_out(buf);
	}

	// Setup the TCP listening socket
	iResult = bind(self->socket.socket, result->ai_addr, (int)result->ai_addrlen);
	if(iResult == SOCKET_ERROR)
	{
		
		sprintf(buf, "bind failed with error: %i\n", WSAGetLastError());
		freeaddrinfo(result);
		self->socket.close(&self->socket);
		del_LNC();
		error_out(buf);
	}

	freeaddrinfo(result);

	
	iResult = listen(self->socket.socket, SOMAXCONN);
	if (iResult == SOCKET_ERROR)
	{
		sprintf(buf, "listen failed with error: %i\n", WSAGetLastError());
		self->socket.close(&self->socket);
		del_LNC();
		error_out(buf);
	}
}

static LSocket __accept(LServer * self)
{
	LSocket ret = newLSocket();
	ret.socket = accept(self->socket.socket, NULL, NULL);
	if (ret.socket == INVALID_SOCKET)
	{
		char buf[100];
		sprintf(buf, "accept failed with error: %i\n",WSAGetLastError());
		self->socket.close(&self->socket);
		del_LNC();
		error_out(buf);
	}
	return ret;
}

LSocket newLClient(char * address, int port, int * error)
{
	char buf[100];
	*error = 0;
	LSocket ret = newLSocket(); 
	ret.socket = INVALID_SOCKET;
	int iResult;
	struct addrinfo *result = NULL, *ptr = NULL, hints;
	ZeroMemory( &hints, sizeof(hints) );
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	_itoa(port, buf, 10);
	
	// Resolve the server address and port
	iResult = getaddrinfo(address, buf, &hints, &result);
	if(iResult != 0)
	{
		sprintf(buf, "failed at getaddrinfo(), with error code: %i\n", iResult);
		error_out(buf);
	}

	for(ptr=result; ptr != NULL ;ptr=ptr->ai_next)
	{
		// Create a SOCKET for connecting to server
		ret.socket = socket(ptr->ai_family, ptr->ai_socktype, 
			ptr->ai_protocol);
		if (ret.socket == INVALID_SOCKET) {
			printf("socket failed with error: %ld\n", WSAGetLastError());
			*error = 1;
			return ret;
		}

		// Connect to server.
		iResult = connect(ret.socket, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (iResult == SOCKET_ERROR) {
			ret.close(&ret);
			ret.socket = INVALID_SOCKET;
			continue;
		}
		break;
	}
		
	freeaddrinfo(result);

	if (ret.socket == INVALID_SOCKET) {
		puts("Unable to connect to server!\n");
		*error = 1;
		return ret;
	}

	*error = 0;
	return ret;
}

#else //linux
void init_LNC()
{
	//nothign to be done
}
void del_LNC()
{
	//nothing to be done
}

static void __setup(LServer * self)
{
	struct sockaddr_in name;
	self->socket.socket = socket(AF_INET, SOCK_STREAM, 0);
	if(self->socket.socket < 0)
	{
		char error_msg [] = "error creating socket in __setup()";
		error_out(error_msg);
	}
		
	name.sin_family = AF_INET;
	name.sin_addr.s_addr = (INADDR_ANY);
	name.sin_port = htons(self->port);
	if(bind(self->socket.socket, (sockaddr*) &name, sizeof(name)) < 0)
	{
		char error_msg [] = "__setup() -> error binding";
		error_out (error_msg);
	}
	listen(self->socket.socket, 5);
		
}

static LSocket __accept(LServer * self)
{
	LSocket ret = newLSocket();
	struct sockaddr_in cli_name;
	socklen_t clilen = sizeof(cli_name);
	ret.socket = accept(self->socket.socket, (sockaddr*) &cli_name, &clilen);
	return ret;
}

static void LSocket__close(LSocket * self)
{
	close(self->socket);
}

LSocket newLClient(char * address, int port, int * error)
{
	LSocket ret = newLSocket();
	struct sockaddr_in serv_addr;
	struct hostent *server_name;

	ret.socket = socket(AF_INET, SOCK_STREAM, 0);
	if (ret.socket < 0)
	{
		char error_msg [] = "newLClient -> ERROR opening socket";
		error_out(error_msg);
	}
	server_name = gethostbyname(address);
	if (server_name == NULL)
	{
		char error_msg [] = "newLClient -> ERROR opening socket";
		error_out(error_msg);
	}	
	memset((char *) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	memcpy((char *)&serv_addr.sin_addr.s_addr,
			(char *)server_name->h_addr, 
			server_name->h_length);
	serv_addr.sin_port = htons(port);
	if (connect(ret.socket, (struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0)
		*error = 1;
	else *error = 0;
	return ret;
}
	

#endif


//platform independent functions

LServer newLServer(int portNumber)
{
	LServer ret;
	//setup socket
	ret.socket = newLSocket();
	//setup port number
	ret.port = portNumber;
	//set functions:
	ret.setup = __setup;
	ret.accept = __accept;
	ret.close = LServer__close;
	ret.canAccept = LServer__canAccept;
	//run setup:
	ret.setup(&ret);
	return ret;
}


//luke's sockets and related functions

LSocket newLSocket()
{
	LSocket ret;
	//init functions
	ret.close = LSocket__close;
	ret.read = LSocket__read;
	ret.write = LSocket__write;
	ret.canRead = LSocket__canRead;
	return ret;
}



