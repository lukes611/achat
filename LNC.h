#ifndef LNC_H
#define LNC_H

#ifdef WIN32
#undef UNICODE
#define WIN32_LEAN_AND_MEAN
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")
#include <winsock2.h>
typedef SOCKET LL_Socket_Type;
#else
typedef int LL_Socket_Type;
#endif

typedef struct LSocket_struct{
	LL_Socket_Type socket;
	void (*close)(struct LSocket_struct * self);
	int (*read)(struct LSocket_struct * self, char * buf, int size);
	int (*write)(struct LSocket_struct * self, char * buf, int size);
	int (*canRead)(struct LSocket_struct * self, int seconds, int microSeconds);
} LSocket;

typedef struct LSocket_struct LClient;

typedef struct LServer_struct{
	int port;
	LSocket socket;
	void (*setup)(struct LServer_struct * self);
	LSocket (*accept)(struct LServer_struct * self);
	void (*close)(struct LServer_struct * self);
	int (*canAccept)(struct LServer_struct * self, int seconds, int microSeconds);
} LServer;


LServer newLServer(int portNumber);
LSocket newLSocket();
LSocket newLClient(char * address, int port, int * error);
void init_LNC();
void del_LNC();

int asyncRead(int totalSockets, int numSeconds, int numMicroSeconds, LL_Socket_Type * sockets, int * readyBools);

#endif
