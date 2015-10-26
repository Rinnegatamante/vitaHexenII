// net_adhoc.c

#include "quakedef.h"
#include <psptypes.h>
#include <pspwlan.h>
#include <pspnet.h>
#include <pspdebug.h>
#include <pspdisplay.h>
#include <pspthreadman.h>
#include <sys/types.h>
#include "pspnet_adhoc.h"
#include "pspnet_adhocctl.h"
#include "adhochelper.h"

static int net_acceptsocket = -1;		// socket for fielding new connections
static int net_controlsocket;
static int net_broadcastsocket = 0;
struct qsockaddr broadcastaddr;

unsigned long myAddr;

#define MAXHOSTNAMELEN 50

#include "net_adhoc.h"

int flags = 1;
int connectSocket = -1;
int maxWrite = 0;

void Datagram_Shutdown (void);
int Datagram_Init (void);

// Used to enable sockaddr structures to be used with adhoc connections
typedef struct sockaddr_adhoc
{
        char   len;
        short family;
        u16	port;
        char mac[6];
		char zero[6];
} sockaddr_adhoc;

#define ADHOC_NET 29

//=============================================================================
char buf[1024];

static pdpStatStruct gPdpStat;

pdpStatStruct *findPdpStat(int socket, pdpStatStruct *pdpStat)
{
	if(socket == pdpStat->pdpId)
	{
		memcpy(&gPdpStat, pdpStat, sizeof(pdpStatStruct));
		return &gPdpStat;
	}

	if(pdpStat->next)
		return findPdpStat(socket, pdpStat->next);

	return (pdpStatStruct *)-1;
}


char debug[1000];

// Used to connect to adhoc via scripts
void NET_Adhoc_f(void)
{
	if (cmd_source != src_command)
		return;

	Datagram_Shutdown();

	adhocAvailable = 1;
	tcpipAvailable = 0;

	Con_Printf ("Starting adhoc connections\n");
	Datagram_Init();
}

int Adhoc_Init (void)
{
	if(!host_initialized)
	{
		Cmd_AddCommand("net_adhoc", NET_Adhoc_f);
	}

	if(!adhocAvailable)
		return -1;

	int i,rc;
	unsigned char mac[6];
	char tempStr[20];

	S_ClearBuffer ();		// so dma doesn't loop current sound

	sceWlanGetEtherAddr((char *)mac);
	sceNetEtherNtostr(mac, tempStr);

	NET_GetHostname(buf, MAXHOSTNAMELEN);

	// if the quake hostname isn't set, set it to the machine name
	if (Q_strcmp(buf, "UNNAMED") == 0)
	{
		Cvar_Set ("hostname", tempStr);
	}
	else
	{
		Cvar_Set ("hostname", buf);
	}

	Q_strcpy(my_tcpip_address,  tempStr);

    	int stateLast = -1;
	// Product ID needs to be passed in (this should be changed for
	// other homebrew apps)
	rc = pspSdkAdhocInit("ULUS90000");
	if(rc < 0)
	{
		Sys_Error("Error calling pspSdkAdhocInit - %x\n", rc);
		return rc;
	}

	rc = sceNetAdhocctlConnect("QUAKE");
	if(rc < 0)
	{
		Sys_Error("AdhocctlConnect failed: rc=%x\n", rc);
		return rc;
	}

    while (1)
    {
        int state;
        rc = sceNetAdhocctlGetState(&state);
        if (rc != 0)
        {
            Sys_Error("sceNetApctlGetState returns $%x\n", rc);
            sceKernelDelayThread(10*1000000); // 10sec to read before exit
			return -1;
        }
        if (state > stateLast)
        {
            Con_Printf("  connection state %d of 1\n", state);
            stateLast = state;
        }
        if (state == 1)
            break;  // connected

        // wait a little before polling again
        sceKernelDelayThread(50*1000); // 50ms
    }

	if ((net_controlsocket = Adhoc_OpenSocket (0)) == -1)
		Sys_Error("Adhoc_Init: Unable to open control socket\n");

	((struct sockaddr_adhoc *)&broadcastaddr)->family = ADHOC_NET;
	for(i=0; i<6; i++)
		((struct sockaddr_adhoc *)&broadcastaddr)->mac[i] = 0xFF;
	((struct sockaddr_adhoc *)&broadcastaddr)->port = net_hostport;

	Con_Printf("Adhoc Initialized\n");
	adhocAvailable = true;

	return net_controlsocket;
}

//=============================================================================

void Adhoc_Shutdown (void)
{
	Adhoc_Listen (false);
	Adhoc_CloseSocket (net_controlsocket);

	pspSdkAdhocTerm();
}

//=============================================================================

void Adhoc_Listen (qboolean state)
{
	// enable listening
	if (state == true)
	{
		if (net_acceptsocket != -1)
			return;
		if ((net_acceptsocket = Adhoc_OpenSocket (net_hostport)) == -1)
			Sys_Error ("Adhoc_Listen: Unable to open accept socket\n");

		return;
	}

	// disable listening
	if (net_acceptsocket == -1)
		return;
	Adhoc_CloseSocket (net_acceptsocket);

	net_acceptsocket = -1;
}

//=============================================================================

int Adhoc_OpenSocket (int port)
{
	char mac[8];
	sceWlanGetEtherAddr(mac);

	int rc = sceNetAdhocPdpCreate(mac,
							 port,
							 0x2000,
							 0);

	return rc;
}

//=============================================================================

int Adhoc_CloseSocket (int socket)
{
	if (socket == net_broadcastsocket)
		net_broadcastsocket = 0;
	int rc = sceNetAdhocPdpDelete(socket, 0);

	return rc;
}


int Adhoc_Connect (int socket, struct qsockaddr *addr)
{
	// When the propper method of scanning for connections is used
	// then this will need to connect to the adhoc connection in here

	flags = 0;
	connectSocket = socket;
	
	return 0;
}

//=============================================================================

int Adhoc_CheckNewConnections (void)
{
	if (net_acceptsocket == -1)
		return -1;

	pdpStatStruct pdpStat[20];
	int length = sizeof(pdpStatStruct) * 20;

	// We only want to know about the accept socket
	// Not sure that this will work or not
	int err = sceNetAdhocGetPdpStat(&length, pdpStat);
	if(err<0)
	{
		Con_Printf("error calling GetPdpStat, err=%x\n", err);
		return err;
	}

	pdpStatStruct *tempPdp = findPdpStat(net_acceptsocket, pdpStat);

	if(tempPdp < 0)
		return -1;

	if(tempPdp->rcvdData > 0)
		return net_acceptsocket;

	return -1;
}

//=============================================================================
int read = 1;
int Adhoc_Read (int socket, byte *buf, int len, struct qsockaddr *addr)
{
	int port;
	int datalength = len;
	int ret;

	sceKernelDelayThread(1);
	ret = sceNetAdhocPdpRecv(socket,
				((struct sockaddr_adhoc *)addr)->mac,
				&port,
				buf,
				&datalength,	// Pass in the value of the buffer size
				0,
				1);

	// Seems to indicate that the socket would be blocked
	// We could either do it this way, or check to see if
	// there are any packets, but this seems easier
	if(ret == 0x80410709)
	{
		return 0;
	}

	if(ret < -1)
	{
		return ret;
	}

	((struct sockaddr_adhoc *)addr)->port = port;

	return datalength;
}

//=============================================================================

int Adhoc_MakeSocketBroadcastCapable (int socket)
{
	// Dont think anything is required to make it broadcast capable

	net_broadcastsocket = socket;

	return 0;
}

//=============================================================================

int Adhoc_Broadcast (int socket, byte *buf, int len)
{
	int ret;
	if (socket != net_broadcastsocket)
	{
		if (net_broadcastsocket != 0)
			Sys_Error("Attempted to use multiple broadcasts sockets\n");
		ret = Adhoc_MakeSocketBroadcastCapable (socket);
		if (ret == -1)
		{
			Con_Printf("Unable to make socket broadcast capable\n");
			return ret;
		}
	}

	return Adhoc_Write (socket, buf, len, &broadcastaddr);
}

//=============================================================================

int Adhoc_Write (int socket, byte *buf, int len, struct qsockaddr *addr)
{
	int ret = -1;

	if(len > maxWrite)
		maxWrite = len;

	ret = sceNetAdhocPdpSend(socket,
				((struct sockaddr_adhoc *)addr)->mac,
				((struct sockaddr_adhoc *)addr)->port,
				buf,
				len,
				0,
				1);

	if(ret < 0)
	{
		Con_Printf("pdpSend err = %x\n", ret);
	}

	return ret;
}

//=============================================================================

char *Adhoc_AddrToString (struct qsockaddr *addr)
{
	static char buffer[22];

	sceNetEtherNtostr((unsigned char *)((struct sockaddr_adhoc *)addr)->mac, buffer);

	sprintf(buffer + strlen(buffer), ":%d", ((struct sockaddr_adhoc *)addr)->port);
	
	return buffer;
}

//=============================================================================

int Adhoc_StringToAddr (char *string, struct qsockaddr *addr)
{
	int ha1, ha2, ha3, ha4, ha5, ha6, hp;

	sscanf(string, "%x:%x:%x:%x:%x:%x:%d", &ha1, &ha2, &ha3, &ha4, &ha5, &ha6, &hp);

	addr->sa_family = ADHOC_NET;
	((struct sockaddr_adhoc *)addr)->mac[0] = ha1 & 0xFF;
	((struct sockaddr_adhoc *)addr)->mac[1] = ha2 & 0xFF;
	((struct sockaddr_adhoc *)addr)->mac[2] = ha3 & 0xFF;
	((struct sockaddr_adhoc *)addr)->mac[3] = ha4 & 0xFF;
	((struct sockaddr_adhoc *)addr)->mac[4] = ha5 & 0xFF;
	((struct sockaddr_adhoc *)addr)->mac[5] = ha6 & 0xFF;

	((struct sockaddr_adhoc *)addr)->port = hp & 0xFFFF;
	return 0;
}

//=============================================================================

void printPdpStat(pdpStatStruct *stat)
{
	pspDebugScreenPrintf("PdpStat - pdpId=%x, mac=%x%x%x%x%x%x, port=%x, rcvdData=%x\n", stat->pdpId, 
		stat->mac[0], stat->mac[1], stat->mac[2], stat->mac[3], stat->mac[4], stat->mac[5],
		stat->port, stat->rcvdData);

	if(stat->next)
		printPdpStat(stat->next);
}

int Adhoc_GetSocketAddr (int socket, struct qsockaddr *addr)
{
	pdpStatStruct pdpStat[20];
	int length = sizeof(pdpStatStruct) * 20;

	// We only want to know about the accept socket
	// Not sure that this will work or not
	int err = sceNetAdhocGetPdpStat(&length, pdpStat);
	if(err<0)
	{
		Con_Printf("error calling GetPdpStat, err=%x\n", err);
		return err;
	}

	pdpStatStruct *tempPdp = findPdpStat(socket, pdpStat);
	if(tempPdp < 0)
		return -1;

	memcpy(((struct sockaddr_adhoc *)addr)->mac, tempPdp->mac, 6);
	((struct sockaddr_adhoc *)addr)->port = tempPdp->port;
	addr->sa_family = ADHOC_NET;

	return 0;
}

//=============================================================================

int Adhoc_GetNameFromAddr (struct qsockaddr *addr, char *name)
{
#if 0
	struct hostent *hostentry;

	//Todo
	hostentry = gethostbyaddr ((char *)&((struct sockaddr_in *)addr)->sin_addr, sizeof(struct in_addr), AF_INET);
	if (hostentry)
	{
		Q_strncpy (name, (char *)hostentry->h_name, NET_NAMELEN - 1);
		return 0;
	}
#endif
	Q_strcpy (name, Adhoc_AddrToString (addr));
	return 0;
}

//=============================================================================

int Adhoc_GetAddrFromName(char *name, struct qsockaddr *addr)
{
	if (name[0] >= '0' && name[0] <= '9')
		return Adhoc_StringToAddr(name, addr);

#if 0
	// CSWINDLE todo: Need to add the resolver code here
	pspDebugScreenPrintf("name=%s\n", name);
	for(;;)
		sceDisplayWaitVblankStart();
	
	hostentry = gethostbyname (name);
	if (!hostentry)
		return -1;

	addr->sa_family = ADHOC_NET;
	((struct sockaddr_in *)addr)->sin_port = htons(net_hostport);	
	((struct sockaddr_in *)addr)->sin_addr.s_addr = *(int *)hostentry->h_addr_list[0];
#endif

	return 0;
}

//=============================================================================

int Adhoc_AddrCompare (struct qsockaddr *addr1, struct qsockaddr *addr2)
{
//	if (addr1->sa_family != addr2->sa_family)
//		return -1;

	if (memcmp(((struct sockaddr_adhoc *)addr1)->mac, ((struct sockaddr_adhoc *)addr2)->mac, 6) != 0)
		return -1;

	if (((struct sockaddr_adhoc *)addr1)->port != ((struct sockaddr_adhoc *)addr2)->port)
		return -1;

	return 0;
}

//=============================================================================

int Adhoc_GetSocketPort (struct qsockaddr *addr)
{
	return ((struct sockaddr_adhoc *)addr)->port;
}


int Adhoc_SetSocketPort (struct qsockaddr *addr, int port)
{
	((struct sockaddr_adhoc *)addr)->port = port;
	return 0;
}

//=============================================================================
