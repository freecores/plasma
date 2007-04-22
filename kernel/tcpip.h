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
#define RETRANSMIT_TIME       3
#define SOCKET_TIMEOUT        12

typedef enum IPMode_e {
   IP_MODE_UDP,
   IP_MODE_TCP
} IPMode_e;

typedef enum IPState_e {
   IP_LISTEN,
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
   uint16 length, timeout;
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
void EthernetSendPacket(const unsigned char *packet, int length);

//tcpip.c
void IPInit(IPFuncPtr FrameSendFunction);
IPFrame *IPFrameGet(int freeCount);
int IPProcessEthernetPacket(IPFrame *frameIn);
void IPTick(void);

IPSocket *IPOpen(IPMode_e Mode, uint32 IPAddress, uint32 Port, IPFuncPtr funcPtr);
void IPWriteFlush(IPSocket *Socket);
uint32 IPWrite(IPSocket *Socket, const uint8 *Buf, uint32 Length);
uint32 IPRead(IPSocket *Socket, uint8 *Buf, uint32 Length);
void IPClose(IPSocket *Socket);
uint32 IPResolve(char *Name, IPFuncPtr resolvedFunc);

//http.c
#define HTML_LENGTH_CALLBACK  -2
#define HTML_LENGTH_LIST_END  -1
typedef struct PageEntry_s {
   const char *name;
   int length;
   const char *page;
} PageEntry_t;
void HttpInit(const PageEntry_t *Pages, int UseFiles);

#endif //__TCPIP_H__
