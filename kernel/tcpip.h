/*--------------------------------------------------------------------
 * TITLE: Plasma TCP/IP Protocol Stack
 * AUTHOR: Steve Rhoads (rhoadss@yahoo.com)
 * DATE CREATED: 4/22/06
 * FILENAME: tcpip.h
 * PROJECT: Plasma CPU core
 * COPYRIGHT: Software placed into the public domain by the author.
 *    Software 'as is' without warranty.  Author liable for nothing.
 * DESCRIPTION:
 *    Plasma TCP/IP Protocol Stack
 *--------------------------------------------------------------------*/
#ifndef __TCPIP_H__
#define __TCPIP_H__
#define PACKET_SIZE           600
#define FRAME_COUNT           100
#define FRAME_COUNT_WINDOW    50
#define FRAME_COUNT_SYNC      50
#define FRAME_COUNT_SEND      10
#define FRAME_COUNT_RCV       5
#define RETRANSMIT_TIME       110
#define SOCKET_TIMEOUT        12

typedef enum IPMode_e {
   IP_MODE_UDP,
   IP_MODE_TCP,
   IP_MODE_PING
} IPMode_e;

typedef enum IPState_e {
   IP_LISTEN,
   IP_PING,
   IP_UDP,
   IP_SYN,
   IP_TCP,
   IP_FIN_CLIENT,
   IP_FIN_SERVER,
   IP_CLOSED
} IPState_e;

typedef void (*IPFuncPtr)();

typedef struct IPFrame {
   uint8 packet[PACKET_SIZE];
   struct IPFrame *next, *prev;
   struct IPSocket *socket;
   uint32 seqEnd;
   uint16 length;
   short  timeout;
   uint8 state, retryCnt;
} IPFrame;

typedef struct IPSocket {
   struct IPSocket *next, *prev;
   IPState_e state;
   uint32 seq;
   uint32 seqReceived;
   uint32 seqWindow;
   uint32 ack;
   uint32 timeout;
   uint32 timeoutReset;
   uint8 headerSend[38];
   uint8 headerRcv[38];
   struct IPFrame *frameReadHead;
   struct IPFrame *frameReadTail;
   int readOffset;
   struct IPFrame *frameSend;
   int sendOffset;
   IPFuncPtr funcPtr;
   IPFuncPtr userFunc;
   void *userPtr;
   uint32 userData;
} IPSocket;

//ethernet.c
void EthernetSendPacket(const unsigned char *packet, int length); //Windows
void EthernetInit(unsigned char MacAddress[6]);
int EthernetReceive(unsigned char *buffer, int length);
void EthernetTransmit(unsigned char *buffer, int length);

//tcpip.c
void IPInit(IPFuncPtr frameSendFunction);
IPFrame *IPFrameGet(int freeCount);
int IPProcessEthernetPacket(IPFrame *frameIn, int length);
void IPTick(void);

IPSocket *IPOpen(IPMode_e mode, uint32 ipAddress, uint32 port, IPFuncPtr funcPtr);
void IPWriteFlush(IPSocket *socket);
uint32 IPWrite(IPSocket *socket, const uint8 *buf, uint32 length);
uint32 IPRead(IPSocket *socket, uint8 *buf, uint32 length);
void IPClose(IPSocket *socket);
void IPPrintf(IPSocket *socket, char *message);
void IPResolve(char *name, IPFuncPtr resolvedFunc, void *arg);
uint32 IPAddressSelf(void);

//http.c
#define HTML_LENGTH_CALLBACK  -2
#define HTML_LENGTH_LIST_END  -1
typedef struct PageEntry_s {
   const char *name;
   int length;
   const char *page;
} PageEntry_t;
void HttpInit(const PageEntry_t *Pages, int UseFiles);

//html.c
void HtmlInit(int UseFiles);

//netutil.c
typedef struct {
   char *name;
   int mode;
   void (*func)();
} TelnetFunc_t;
void FtpdInit(int UseFiles);
IPSocket *FtpTransfer(uint32 ip, char *user, char *passwd, 
                      char *filename, uint8 *buf, int size, 
                      int send, void (*callback)(uint8 *data, int size));
void TftpdInit(void);
IPSocket *TftpTransfer(uint32 ip, char *filename, uint8 *buffer, int size,
                       void (*callback)(uint8 *data, int bytes));
void TelnetInit(TelnetFunc_t *funcList);
void ConsoleInit(void);

#endif //__TCPIP_H__
