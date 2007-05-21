/*--------------------------------------------------------------------
 * TITLE: Plasma TCP/IP Network Utilities
 * AUTHOR: Steve Rhoads (rhoadss@yahoo.com)
 * DATE CREATED: 4/20/07
 * FILENAME: netutil.c
 * PROJECT: Plasma CPU core
 * COPYRIGHT: Software placed into the public domain by the author.
 *    Software 'as is' without warranty.  Author liable for nothing.
 * DESCRIPTION:
 *    Plasma FTP server and FTP client and TFTP server and client 
 *    and Telnet server.
 *--------------------------------------------------------------------*/
#ifdef WIN32
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#define _LIBC
#define INCLUDE_FILESYS
#else
#define INCLUDE_FILESYS
#endif
#include "rtos.h"
#include "tcpip.h"
#ifdef WIN32
#define UartPrintf printf
#define OS_MQueueCreate(A,B,C) 0
#define OS_MQueueGet(A,B,C) 0
#define OS_ThreadCreate(A,B,C,D,E) 0
#endif


//******************* FTP Server ************************
typedef struct {
   IPSocket *socket;
   int ip, port, bytes, done, canReceive;
   FILE *file;
} FtpdInfo;

static void FtpdSender(IPSocket *socket)
{
   unsigned char buf[600];
   int i, bytes, bytes2;
   FtpdInfo *info = (FtpdInfo*)socket->userPtr;

   if(info == NULL || info->done)
      return;
   fseek(info->file, info->bytes, 0);
   for(i = 0; i < 5; ++i)
   {
      bytes = fread(buf, 1, 512, info->file);
      bytes2 = IPWrite(socket, buf, bytes);
      info->bytes += bytes2;
      if(bytes != bytes2)
         return;
      if(bytes < 512)
      {
         fclose(info->file);
         IPClose(socket);
         info->done = 1;
         IPPrintf(info->socket, "226 Done\r\n");
         return;
      }
   }
}


static void FtpdReceiver(IPSocket *socket)
{
   unsigned char buf[600];
   int bytes, state = socket->state;
   FtpdInfo *info = (FtpdInfo*)socket->userPtr;

   if(info == NULL || info->done)
      return;
   do
   {
      bytes = IPRead(socket, buf, sizeof(buf));
      fwrite(buf, 1, bytes, info->file);
   } while(bytes);
   if(state > IP_TCP)
   {
      fclose(info->file);
      IPClose(socket);
      info->done = 1;
      IPPrintf(info->socket, "226 Done\r\n");
      return;
   }
}


static void FtpdServer(IPSocket *socket)
{
   uint8 buf[600];
   int bytes;
   int ip0, ip1, ip2, ip3, port0, port1;
   IPSocket *socketOut;
   FtpdInfo *info = (FtpdInfo*)socket->userPtr;

   if(socket == NULL)
      return;
   bytes = IPRead(socket, buf, sizeof(buf)-1);
   buf[bytes] = 0;
   //printf("(%s)\n", buf);
   if(socket->userPtr == NULL)
   {
      info = (FtpdInfo*)malloc(sizeof(FtpdInfo));
      if(info == NULL)
         return;
      memset(info, 0, sizeof(FtpdInfo));
      socket->userPtr = info;
      info->socket = socket;
      socket->timeout = 0;
      IPPrintf(socket, "220 Connected to Plasma\r\n");
   }
   else if(strstr((char*)buf, "USER"))
   {
      if(strstr((char*)buf, "PlasmaSend"))
         info->canReceive = 1;
      socket->timeout = 0;
      IPPrintf(socket, "331 Password?\r\n");
   }
   else if(strstr((char*)buf, "PASS"))
   {
      IPPrintf(socket, "230 Logged in\r\n");
   }
   else if(strstr((char*)buf, "PORT"))
   {
      if(info == NULL)
         return;
      socket->timeout = 0;
      sscanf((char*)buf + 5, "%d,%d,%d,%d,%d,%d", &ip0, &ip1, &ip2, &ip3, &port0, &port1);
      info->ip = (ip0 << 24) | (ip1 << 16) | (ip2 << 8) | ip3;
      info->port = (port0 << 8) | port1;
      //printf("ip=0x%x port=%d\n", info->ip, info->port);
      IPPrintf(socket, "200 OK\r\n");
   }
   else if(strstr((char*)buf, "RETR") || strstr((char*)buf, "STOR"))
   {
      char *ptr = strstr((char*)buf, "\r");
      if(ptr)
         *ptr = 0;
      if(info == NULL)
         return;
      info->file = NULL;
      info->bytes = 0;
      info->done = 0;
      if(strstr((char*)buf, "RETR"))
         info->file = fopen((char*)buf + 5, "rb");
      else if(info->canReceive)
         info->file = fopen((char*)buf + 5, "wb");
      if(info->file)
      {
         IPPrintf(socket, "150 File ready\r\n");
         if(strstr((char*)buf, "RETR"))
            socketOut = IPOpen(IP_MODE_TCP, info->ip, info->port, FtpdSender);
         else
            socketOut = IPOpen(IP_MODE_TCP, info->ip, info->port, FtpdReceiver);
         if(socketOut)
            socketOut->userPtr = info;
      }
      else
      {
         IPPrintf(socket, "500 Error\r\n");
      }
   }
   else if(strstr((char*)buf, "QUIT"))
   {
      if(socket->userPtr)
         free(socket->userPtr);
      IPPrintf(socket, "221 Bye\r\n");
      IPClose(socket);
   }
   else if(bytes)
   {
      IPPrintf(socket, "500 Error\r\n");
   }
}


void FtpdInit(int UseFiles)
{
   (void)UseFiles;
   IPOpen(IP_MODE_TCP, 0, 21, FtpdServer);
}


//******************* FTP Client ************************

typedef struct {
   uint32 ip, port;
   char user[80], passwd[80], filename[80];
   uint8 *buf;
   int size, bytes, send, state;
} FtpInfo;


static void FtpCallbackTransfer(IPSocket *socket)
{
   int bytes, state = socket->state;
   FtpInfo *info = (FtpInfo*)socket->userPtr;

   //printf("FtpCallbackTransfer\n");
   if(info == NULL)
      return;
   bytes = info->size - info->bytes;
   if(info->send == 0)
      bytes = IPRead(socket, info->buf + info->bytes, bytes);
   else
      bytes = IPWrite(socket, info->buf + info->bytes, bytes);
   info->bytes += bytes;
   if(info->bytes == info->size || (bytes == 0 && state > IP_TCP))
   {
      socket->userFunc(info->buf, info->bytes);
      free(info);
      socket->userPtr = NULL;
      IPClose(socket);
   }
}


static void FtpCallback(IPSocket *socket)
{
   char buf[600];
   FtpInfo *info = (FtpInfo*)socket->userPtr;
   int bytes, value;

   bytes = IPRead(socket, (uint8*)buf, sizeof(buf)-1);
   if(bytes == 0)
      return;
   buf[bytes] = 0;
   sscanf(buf, "%d", &value);
   if(bytes > 2)
      buf[bytes-2] = 0;
   //printf("FtpCallback(%d:%s)\n", socket->userData, buf);
   if(value / 100 != 2 && value / 100 != 3)
      return;
   buf[0] = 0;
   switch(socket->userData) {
   case 0:
      sprintf(buf, "USER %s\r\n", info->user);
      socket->userData = 1;
      break;
   case 1:
      sprintf(buf, "PASS %s\r\n", info->passwd);
      socket->userData = 2;
      if(value == 331)
         break;  //possible fall-through
   case 2:
      sprintf(buf, "PORT %d,%d,%d,%d,%d,%d\r\n",
         info->ip >> 24, (uint8)(info->ip >> 16),
         (uint8)(info->ip >> 8), (uint8)info->ip,
         (uint8)(info->port >> 8), (uint8)info->port);
      socket->userData = 3;
      break;
   case 3:
      if(info->send == 0)
         sprintf(buf, "RETR %s\r\n", info->filename);
      else
         sprintf(buf, "STOR %s\r\n", info->filename);
      socket->userData = 4;
      break;
   case 4:
      sprintf(buf, "QUIT\r\n");
      socket->userData = 9;
      break;
   }
   IPWrite(socket, (uint8*)buf, strlen(buf));
   IPWriteFlush(socket);
   if(socket->userData == 9)
      IPClose(socket);
}


IPSocket *FtpTransfer(uint32 ip, char *user, char *passwd, 
                      char *filename, uint8 *buf, int size, 
                      int send, void (*callback)(uint8 *data, int size))
{
   IPSocket *socket, *socketTransfer;
   FtpInfo *info;
   uint8 *ptr;
   info = (FtpInfo*)malloc(sizeof(FtpInfo));
   if(info == NULL)
      return NULL;
   strncpy(info->user, user, 80);
   strncpy(info->passwd, passwd, 80);
   strncpy(info->filename, filename, 80);
   info->buf = buf;
   info->size = size;
   info->send = send;
   info->bytes = 0;
   info->state = 0;
   info->port = 2000;
   socketTransfer = IPOpen(IP_MODE_TCP, 0, info->port, FtpCallbackTransfer);
   socketTransfer->userPtr = info;
   socketTransfer->userFunc = callback;
   socket = IPOpen(IP_MODE_TCP, ip, 21, FtpCallback);
   socket->userPtr = info;
   socket->userFunc = callback;
   ptr = socket->headerSend;
   info->ip = IPAddressSelf();
   return socket;
}


//******************* TFTP Server ************************


static void TftpdCallback(IPSocket *socket)
{
   unsigned char buf[512+4];
   int bytes, blockNum;
   FILE *file = (FILE*)socket->userPtr;
   bytes = IPRead(socket, buf, sizeof(buf));
   //printf("TfptdCallback bytes=%d\n", bytes);
   if(bytes < 4 || buf[0])
      return;
   if(buf[1] == 1)  //RRQ = Read Request
   {
      if(file)
         fclose(file);
      file = fopen((char*)buf+2, "rb");
      socket->userPtr = file;
      if(file == NULL)
      {
         buf[0] = 0;
         buf[1] = 5;   //ERROR
         buf[2] = 0;
         buf[3] = 0;
         buf[4] = 'X'; //Error string
         buf[5] = 0;
         IPWrite(socket, buf, 6);
         return;
      }
   }
   if(buf[1] == 1 || buf[1] == 4) //ACK
   {
      if(file == NULL)
         return;
      if(buf[1] == 1)
         blockNum = 0;
      else
         blockNum = (buf[2] << 8) | buf[3];
      ++blockNum;
      buf[0] = 0;
      buf[1] = 3;  //DATA
      buf[2] = (uint8)(blockNum >> 8);
      buf[3] = (uint8)blockNum;
      fseek(file, (blockNum-1)*512, 0);
      bytes = fread(buf+4, 1, 512, file);
      IPWrite(socket, buf, bytes+4);
   }
}


void TftpdInit(void)
{
   IPSocket *socket;
   socket = IPOpen(IP_MODE_UDP, 0, 69, TftpdCallback);
}


//******************* TFTP Client ************************


static void TftpCallback(IPSocket *socket)
{
   unsigned char buf[512+4];
   int bytes, blockNum, length;

   bytes = IPRead(socket, buf, sizeof(buf));
   if(bytes < 4 || buf[0])
      return;
   blockNum = (buf[2] << 8) | buf[3];
   length = blockNum * 512 - 512 + bytes - 4;
   //printf("TftpCallback(%d,%d)\n", buf[1], blockNum);
   if(length > (int)socket->userData)
   {
      bytes -= length - (int)socket->userData;
      length = (int)socket->userData;
   }
   if(buf[1] == 3) //DATA
   {
      memcpy((uint8*)socket->userPtr + blockNum * 512 - 512, buf+4, bytes-4);
      buf[1] = 4; //ACK
      IPWrite(socket, buf, 4);
      if(bytes-4 < 512)
      {
         socket->userFunc(socket->userPtr, length);
         IPClose(socket);
      }
   }
}


IPSocket *TftpTransfer(uint32 ip, char *filename, uint8 *buffer, int size,
                       void (*callback)(uint8 *data, int bytes))
{
   IPSocket *socket;
   uint8 buf[512+4];
   int bytes;
   socket = IPOpen(IP_MODE_UDP, ip, 69, TftpCallback);
   socket->userPtr = buffer;
   socket->userData = size;
   socket->userFunc = callback;
   buf[0] = 0;
   buf[1] = 1; //read
   strcpy((char*)buf+2, filename);
   bytes = strlen(filename);
   strcpy((char*)buf+bytes+3, "octet");
   IPWrite(socket, buf, bytes+9);
   return socket;
}


//******************* Telnet Server ************************

#define COMMAND_BUFFER_SIZE 80
#define COMMAND_BUFFER_COUNT 10
static char CommandHistory[400];
static char *CommandPtr[COMMAND_BUFFER_COUNT];
static int CommandIndex;
static TelnetFunc_t *TelnetFuncList;


static void TelnetServer(IPSocket *socket)
{
   uint8 buf[COMMAND_BUFFER_SIZE+4];
   char bufOut[COMMAND_BUFFER_SIZE+32];
   int bytes, i, j, length;
   char *ptr, *command = socket->userPtr;
   bytes = IPRead(socket, buf, sizeof(buf)-1);
   if(bytes == 0)
   {
      if(socket->userData)
         return;
      socket->userData = 1;
      socket->userPtr = command = (char*)malloc(COMMAND_BUFFER_SIZE);
      if(command == NULL)
      {
         IPClose(socket);
         return;
      }
      socket->timeout = 0;
      buf[0] = 255; //IAC
      buf[1] = 251; //WILL
      buf[2] = 3;   //suppress go ahead
      buf[3] = 255; //IAC
      buf[4] = 251; //WILL
      buf[5] = 1;   //echo
      strcpy((char*)buf+6, " Welcome to Plasma.\r\n-> ");
      IPWrite(socket, buf, 6+23);
      IPWriteFlush(socket);
      command[0] = 0;
      return;
   }
   if(command == NULL)
      return;
   buf[bytes] = 0;
   length = (int)strlen(command);
   for(j = 0; j < bytes; ++j)
   {
      if(buf[j] == 255)
         return;
      if(buf[j] == 8 || (buf[j] == 27 && buf[j+2] == 'D'))
      {
         if(buf[j] == 27)
            j += 2;
         if(length)
         {
            // Backspace
            command[--length] = 0;
            bufOut[0] = 8;
            bufOut[1] = ' ';
            bufOut[2] = 8;
            IPWrite(socket, (uint8*)bufOut, 3);
         }
      }
      else if(buf[j] == 27)
      {
         // Command History
         if(buf[j+2] == 'A')
         {
            if(++CommandIndex > COMMAND_BUFFER_COUNT)
               CommandIndex = COMMAND_BUFFER_COUNT;
         }
         else if(buf[j+2] == 'B')
         {
            if(--CommandIndex < 0)
               CommandIndex = 0;
         }
         else 
            return;
         bufOut[0] = 8;
         bufOut[1] = ' ';
         bufOut[2] = 8;
         for(i = 0; i < length; ++i)
            IPWrite(socket, (uint8*)bufOut, 3);
         if(CommandIndex && CommandPtr[CommandIndex-1])
            strcpy(command, CommandPtr[CommandIndex-1]);
         else
            command[0] = 0;
         length = (int)strlen(command);
         IPWrite(socket, (uint8*)command, length);
         j += 2;
      }
      else
      {
         if(length < COMMAND_BUFFER_SIZE-2)
         {
            IPWrite(socket, buf+j, 1);
            command[length] = buf[j];
            command[++length] = 0;
         }
      }
      ptr = strstr(command, "\r\n");
      if(ptr)
      {
         ptr[0] = 0;
         bufOut[0] = 0;
         if(command[0])
         {
            // Save command in CommandHistory
            memcpy(CommandHistory + length + 1, CommandHistory,
               sizeof(CommandHistory) - length - 1);
            for(i = COMMAND_BUFFER_COUNT-2; i >= 0; --i)
            {
               if(CommandPtr[i+1] && CommandPtr[i+1] + length + 1 >= 
                  CommandHistory + sizeof(CommandHistory))
                  CommandPtr[i+1] = NULL;
               else if(CommandPtr[i])
                  CommandPtr[i+1] = CommandPtr[i] + length + 1;
            }
            CommandPtr[CommandIndex] = CommandHistory;
            strcpy(CommandHistory, command);
         }
         if(strncmp(command, "help", 4) == 0)
         {
            sprintf((char*)bufOut, "Commands: help, exit");
            for(i = 0; TelnetFuncList[i].name; ++i)
            {
               strcat((char*)bufOut, ", ");
               strcat((char*)bufOut, TelnetFuncList[i].name);
            }
            strcat((char*)bufOut, "\r\n");
         }
         else if(strncmp(command, "exit", 4) == 0)
         {
            free(command);
            socket->userPtr = NULL;
            IPClose(socket);
            return;
         }
         else if(command[0])
         {
            strcpy(bufOut, command);
            ptr = strstr(bufOut, " ");
            if(ptr)
               ptr[0] = 0;
            for(i = 0; TelnetFuncList[i].name; ++i)
            {
               if(strcmp(bufOut, TelnetFuncList[i].name) == 0)
               {
                  TelnetFuncList[i].func(socket, command);
                  break;
               }
            }
            bufOut[0] = 0;
            if(TelnetFuncList[i].name == NULL)
               sprintf((char*)bufOut, "Unknown command (%s)\r\n", command);
         }
         strcat((char*)bufOut, "-> ");
         IPPrintf(socket, (char*)bufOut);
         command[0] = 0;
         length = 0;
         CommandIndex = 0;
      }
   }
   IPWriteFlush(socket);
}


void TelnetInit(TelnetFunc_t *funcList)
{
   IPSocket *socket;
   TelnetFuncList = funcList;
   socket = IPOpen(IP_MODE_TCP, 0, 23, TelnetServer);
}


//******************* Console ************************

static uint8 myBuffer[1024*3];
static IPSocket *socketTelnet;


void TransferDone(uint8 *data, int bytes)
{
   printf("TransferDone(0x%x, %d)\n", data, bytes);
   data[bytes] = 0;
   if(bytes > 500)
      data[500] = 0;
   printf("%s\n", data);
}


static void ConsoleInfo(IPSocket *socket, char *command)
{
   (void)command;
   IPPrintf(socket, "Steve was here!\r\n");
}


static void ConsoleMath(IPSocket *socket, char *command)
{
   int v1, v2;
   char buf[20], buf2[20];
   (void)socket;
   if(strlen(command) < 6)
   {
      IPPrintf(socket, "Usage: math <number> <operator> <value>\r\n");
      return;
   }
   sscanf(command, "%s%d%s%d", buf, &v1, buf2, &v2);
   if(buf2[0] == '+')
      v1 += v2;
   else if(buf2[0] == '-')
      v1 -= v2;
   else if(buf2[0] == '*')
      v1 *= v2;
   else if(buf2[0] == '/')
   {
      if(v2 != 0)
         v1 /= v2;
   }
   sprintf((char*)myBuffer, "%d\r\n", v1);
   IPPrintf(socket, (char*)myBuffer);
}


void PingCallback(IPSocket *socket)
{
   IPSocket *socket2 = socket->userPtr;
   //printf("Ping Reply\n");
   IPClose(socket);
   if(socket2)
      IPPrintf(socket2, "Ping Reply\r\n");
   else
      printf("Ping Reply\n");
}


static void DnsResultCallback(IPSocket *socket, uint32 ip, void *arg)
{
   char buf[COMMAND_BUFFER_SIZE];
   IPSocket *socketTelnet = arg;
   IPSocket *socketPing;
   (void)socket;
   sprintf(buf,  "ip=%d.%d.%d.%d\r\n", 
      (uint8)(ip >> 24), (uint8)(ip >> 16), (uint8)(ip >> 8), (uint8)ip);
   IPPrintf(socketTelnet, buf);
   socketPing = IPOpen(IP_MODE_PING, ip, 0, PingCallback);
   socketPing->userPtr = socketTelnet;
   myBuffer[0] = 'A';
   IPWrite(socketPing, myBuffer, 1);
}


static void ConsolePing(IPSocket *socket, char *command)
{
   int ip0, ip1, ip2, ip3;
   char buf[20];
   IPSocket *socketPing;
   if('0' <= command[5] && command[5] <= '9')
   {
      sscanf(command, "%s %d.%d.%d.%d", buf, &ip0, &ip1, &ip2, &ip3);
      ip0 = (ip0 << 24) | (ip1 << 16) | (ip2 << 8) | ip3;
      socketPing = IPOpen(IP_MODE_PING, ip0, 0, PingCallback);
      socketPing->userPtr = socket;
      myBuffer[0] = 'A';
      IPWrite(socketPing, myBuffer, 1);
      IPPrintf(socket, "Sent ping\r\n");
   }
   else
   {
      IPResolve(command+5, DnsResultCallback, socket);
      IPPrintf(socket, "Sent DNS request\r\n");
   }
}


void ConsoleTransferDone(uint8 *data, int length)
{
   data[length] = 0;
   IPPrintf(socketTelnet, "Transfer Done\r\n");
}


static void ConsoleFtp(IPSocket *socket, char *command)
{
   char buf[40], user[40], passwd[40], name[COMMAND_BUFFER_SIZE];
   int ip0, ip1, ip2, ip3;
   if(strlen(command) < 10)
   {
      IPPrintf(socket, "ftp #.#.#.# User Password File\r\n");
      return;
   }
   sscanf(command, "%s %d.%d.%d.%d %s %s %s", 
      buf, &ip0, &ip1, &ip2, &ip3, user, passwd, name);
   ip0 = (ip0 << 24) | (ip1 << 16) | (ip2 << 8) | ip3;
   socketTelnet = socket;
   FtpTransfer(ip0, user, passwd, name, myBuffer, sizeof(myBuffer)-1, 
      0, ConsoleTransferDone);
}


static void ConsoleTftp(IPSocket *socket, char *command)
{
   char buf[COMMAND_BUFFER_SIZE], name[80];
   int ip0, ip1, ip2, ip3;
   if(strlen(command) < 10)
   {
      IPPrintf(socket, "tftp #.#.#.# File\r\n");
      return;
   }
   sscanf(command, "%s %d.%d.%d.%d %s", 
      buf, &ip0, &ip1, &ip2, &ip3, name);
   ip0 = (ip0 << 24) | (ip1 << 16) | (ip2 << 8) | ip3;
   socketTelnet = socket;
   TftpTransfer(ip0, name, myBuffer, sizeof(myBuffer)-1, ConsoleTransferDone);
}


static void ConsoleShow(IPSocket *socket, char *command)
{
   (void)command;
   IPPrintf(socket, (char*)myBuffer);
   IPPrintf(socket, "\r\n");
}


static void ConsoleClear(IPSocket *socket, char *command)
{
   (void)socket;
   (void)command;
   memset(myBuffer, 0, sizeof(myBuffer));
}


static void ConsoleCat(IPSocket *socket, char *command)
{
   char buf[COMMAND_BUFFER_SIZE], name[80];
   int bytes, bytes2;
   FILE *file;
   sscanf(command, "%s %s\n", buf, name);
   file = fopen(name, "r");
   if(file)
   {
      for(;;)
      {
         bytes = fread(buf, 1, sizeof(buf), file);
         if(bytes == 0)
            break;
         bytes2 = IPWrite(socket, (uint8*)buf, bytes);
         if(bytes != bytes2)
            printf("(%d %d) ", bytes, bytes2);
      }
      fclose(file);
   }
   IPPrintf(socket, "\r\n");
}


static void ConsoleMkfile(IPSocket *socket, char *command)
{
   OS_FILE *file;
   (void)command;
   file = fopen("myfile.txt", "w");
   fwrite("Hello World!", 1, 12, file);
   fclose(file);
   IPPrintf(socket, "Created myfile.txt\r\n");
}


static TelnetFunc_t MyFuncs[] = { 
   {"info", 0, ConsoleInfo},
   {"math", 0, ConsoleMath},
   {"ping", 0, ConsolePing},
   {"ftp", 0, ConsoleFtp},
   {"tftp", 0, ConsoleTftp},
   {"show", 0, ConsoleShow},
   {"clear", 0, ConsoleClear},
   {"cat", 0, ConsoleCat},
   {"mkfile", 0, ConsoleMkfile},
   {NULL, 0, NULL}
};


void ConsoleInit(void)
{
   FtpdInit(1);
   TftpdInit();
   TelnetInit(MyFuncs);
}
