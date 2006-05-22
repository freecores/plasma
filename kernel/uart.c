/*--------------------------------------------------------------------
 * TITLE: Plasma Uart Driver
 * AUTHOR: Steve Rhoads (rhoadss@yahoo.com)
 * DATE CREATED: 12/31/05
 * FILENAME: uart.c
 * PROJECT: Plasma CPU core
 * COPYRIGHT: Software placed into the public domain by the author.
 *    Software 'as is' without warranty.  Author liable for nothing.
 * DESCRIPTION:
 *    Plasma Uart Driver
 *--------------------------------------------------------------------*/
#define NO_ELLIPSIS2
#include "plasma.h"
#include "rtos.h"

#define SUPPORT_DATA_PACKETS

#define BUFFER_WRITE_SIZE 128
#define BUFFER_READ_SIZE 128
#define BUFFER_PRINTF_SIZE 1024
#undef UartPrintf

typedef struct Buffer_s {
   uint8 *data;
   int size;
   volatile int read, write;
   volatile int pendingRead, pendingWrite;
   OS_Semaphore_t *semaphoreRead, *semaphoreWrite;
} Buffer_t;

static Buffer_t *WriteBuffer, *ReadBuffer;
static OS_Semaphore_t *SemaphoreUart;
static char PrintfString[BUFFER_PRINTF_SIZE];  //Used in UartPrintf

#ifdef SUPPORT_DATA_PACKETS
//For packet processing [0xff lengthMSB lengthLSB checksum data]
static PacketGetFunc_t UartPacketGet;
static uint8 *PacketCurrent;
static uint32 UartPacketSize;
static uint32 UartPacketChecksum, Checksum;
static OS_MQueue_t *UartPacketMQueue;
static uint32 PacketBytes, PacketLength;
static uint32 UartPacketOutLength, UartPacketOutByte;
int CountOk, CountError;
#endif
static uint8 *UartPacketOut;


/******************************************/
Buffer_t *BufferCreate(int size)
{
   Buffer_t *buffer;
   buffer = (Buffer_t*)OS_HeapMalloc(NULL, sizeof(Buffer_t) + size);
   if(buffer == NULL)
      return NULL;
   buffer->data = (uint8*)(buffer + 1);
   buffer->read = 0;
   buffer->write = 0;
   buffer->size = size;
   buffer->pendingRead = 0;
   buffer->pendingWrite = 0;
   buffer->semaphoreRead = OS_SemaphoreCreate("BufferRead", 0);
   buffer->semaphoreWrite = OS_SemaphoreCreate("BufferWrite", 0);
   return buffer;
}


void BufferWrite(Buffer_t *Buffer, int Value, int Pend)
{
   int writeNext;

   writeNext = Buffer->write + 1;
   if(writeNext >= Buffer->size)
      writeNext = 0;

   //Check if room for value
   if(writeNext == Buffer->read)
   {
      if(Pend == 0)
         return;
      ++Buffer->pendingWrite;
      OS_SemaphorePend(Buffer->semaphoreWrite, OS_WAIT_FOREVER);
   }

   Buffer->data[Buffer->write] = (uint8)Value;
   Buffer->write = writeNext;
   if(Buffer->pendingRead)
   {
      --Buffer->pendingRead;
      OS_SemaphorePost(Buffer->semaphoreRead);
   }
}


int BufferRead(Buffer_t *Buffer, int Pend)
{
   int value;

   //Check if empty buffer
   if(Buffer->read == Buffer->write)
   {
      if(Pend == 0)
         return 0;
      ++Buffer->pendingRead;
      OS_SemaphorePend(Buffer->semaphoreRead, OS_WAIT_FOREVER);
   }

   value = Buffer->data[Buffer->read];
   if(++Buffer->read >= Buffer->size)
      Buffer->read = 0;
   if(Buffer->pendingWrite)
   {
      --Buffer->pendingWrite;
      OS_SemaphorePost(Buffer->semaphoreWrite);
   }
   return value;
}


/******************************************/
#ifdef SUPPORT_DATA_PACKETS
static void UartPacketRead(uint32 value)
{
   uint32 message[4];
   if(PacketBytes == 0 && value == 0xff)
   {
      ++PacketBytes;
   }
   else if(PacketBytes == 1)
   {
      ++PacketBytes;
      PacketLength = value << 8;
   }
   else if(PacketBytes == 2)
   {
      ++PacketBytes;
      PacketLength |= value;
      if(PacketLength <= UartPacketSize)
         PacketCurrent = UartPacketGet();
      else
      {
         PacketCurrent = NULL;
         PacketBytes = 0;
      }
   }
   else if(PacketBytes == 3)
   {
      ++PacketBytes;
      UartPacketChecksum = value;
      Checksum = 0;
   }
   else if(PacketBytes >= 4)
   {
      if(PacketCurrent)
         PacketCurrent[PacketBytes - 4] = (uint8)value;
      Checksum += value;
      ++PacketBytes;
      if(PacketBytes - 4 >= PacketLength)
      {
         if((uint8)Checksum == UartPacketChecksum)
         {
            //Notify thread that a packet have been received
            ++CountOk;
            message[0] = 0;
            message[1] = (uint32)PacketCurrent;
            message[2] = PacketLength;
            if(PacketCurrent)
               OS_MQueueSend(UartPacketMQueue, message);
         }
         else
         {
            ++CountError;
            //printf("E");
         }
         PacketBytes = 0;
      }
   }
}


static int UartPacketWrite(void)
{
   int value=0, i;
   uint32 message[4];
   if(UartPacketOut)
   {
      if(UartPacketOutByte == 0)
      {
         value = 0xff;
         ++UartPacketOutByte;
      }
      else if(UartPacketOutByte == 1)
      {
         value = UartPacketOutLength >> 8;
         ++UartPacketOutByte;
      }
      else if(UartPacketOutByte == 2)
      {
         value = (uint8)UartPacketOutLength;
         ++UartPacketOutByte;
      }
      else if(UartPacketOutByte == 3)
      {
         value = 0;
         for(i = 0; i < (int)UartPacketOutLength; ++i)
            value += UartPacketOut[i];
         value = (uint8)value;
         ++UartPacketOutByte;
      }
      else 
      {
         value = UartPacketOut[UartPacketOutByte - 4];
         ++UartPacketOutByte;
         if(UartPacketOutByte - 4 >= UartPacketOutLength)
         {
            //Notify thread that a packet has been sent
            message[0] = 1;
            message[1] = (uint32)UartPacketOut;
            UartPacketOut = 0;
            OS_MQueueSend(UartPacketMQueue, message);
         }
      }
   }
   return value;
}
#endif


static void UartInterrupt(void *Arg)
{
   uint32 status, value, count=0;
   (void)Arg;

   status = OS_InterruptStatus();
   while(status & IRQ_UART_READ_AVAILABLE)
   {
      value = MemoryRead(UART_READ);
#ifdef SUPPORT_DATA_PACKETS
      if(UartPacketGet && (value == 0xff || PacketBytes))
         UartPacketRead(value);
      else
#endif
      BufferWrite(ReadBuffer, value, 0);
      status = OS_InterruptStatus();
      if(++count >= 16)
         break;
   }
   while(status & IRQ_UART_WRITE_AVAILABLE)
   {
#ifdef SUPPORT_DATA_PACKETS
      if(UartPacketOut)
      {
         value = UartPacketWrite();
         MemoryWrite(UART_WRITE, value);
      } else 
#endif
      if(WriteBuffer->read != WriteBuffer->write)
      {
         value = BufferRead(WriteBuffer, 0);
         MemoryWrite(UART_WRITE, value);
      }
      else
      {
         OS_InterruptMaskClear(IRQ_UART_WRITE_AVAILABLE);
         break;
      }
      status = OS_InterruptStatus();
   }
}


void UartInit(void)
{
   uint32 mask;

   SemaphoreUart = OS_SemaphoreCreate("Uart", 1);
   WriteBuffer = BufferCreate(BUFFER_WRITE_SIZE);
   ReadBuffer = BufferCreate(BUFFER_READ_SIZE);

   mask = IRQ_UART_READ_AVAILABLE | IRQ_UART_WRITE_AVAILABLE;
   OS_InterruptRegister(mask, UartInterrupt);
   OS_InterruptMaskSet(IRQ_UART_READ_AVAILABLE);
}


void UartWrite(int C)
{
   BufferWrite(WriteBuffer, C, 1);
   OS_InterruptMaskSet(IRQ_UART_WRITE_AVAILABLE);
}


uint8 UartRead(void)
{
   return (uint8)BufferRead(ReadBuffer, 1);
}


void UartWriteData(uint8 *Data, int Length)
{
   OS_SemaphorePend(SemaphoreUart, OS_WAIT_FOREVER);
   while(Length--)
      UartWrite(*Data++);
   OS_SemaphorePost(SemaphoreUart);
}


void UartReadData(uint8 *Data, int Length)
{
   OS_SemaphorePend(SemaphoreUart, OS_WAIT_FOREVER);
   while(Length--)
      *Data++ = UartRead();
   OS_SemaphorePost(SemaphoreUart);
}


void UartPrintf(const char *format,
                int arg0, int arg1, int arg2, int arg3,
                int arg4, int arg5, int arg6, int arg7)
{
   uint8 *ptr;
   OS_SemaphorePend(SemaphoreUart, OS_WAIT_FOREVER);
   sprintf(PrintfString, format, arg0, arg1, arg2, arg3,
           arg4, arg5, arg6, arg7);
   ptr = (uint8*)PrintfString;
   while(*ptr)
   {
      if(*ptr == '\n')
         UartWrite('\r');
#ifdef SUPPORT_DATA_PACKETS
      if(*ptr == 0xff)
         *ptr = '@';
#endif
      UartWrite(*ptr++);
   }
   OS_SemaphorePost(SemaphoreUart);
}


void UartPrintfPoll(const char *format,
                    int arg0, int arg1, int arg2, int arg3,
                    int arg4, int arg5, int arg6, int arg7)
{
   uint8 *ptr;
   uint32 state;

   if(SemaphoreUart)
      OS_SemaphorePend(SemaphoreUart, OS_WAIT_FOREVER);
   sprintf(PrintfString, format, arg0, arg1, arg2, arg3,
           arg4, arg5, arg6, arg7);
   ptr = (uint8*)PrintfString;
   while(*ptr)
   {
      while((MemoryRead(IRQ_STATUS) & IRQ_UART_WRITE_AVAILABLE) == 0)
         ;
      state = OS_CriticalBegin();
      if((MemoryRead(IRQ_STATUS) & IRQ_UART_WRITE_AVAILABLE) &&
         UartPacketOut == NULL)
      {
         MemoryWrite(UART_WRITE, *ptr++);
      }
      OS_CriticalEnd(state);
   }
   if(SemaphoreUart)
      OS_SemaphorePost(SemaphoreUart);
}


void UartPrintfCritical(const char *format,
                        int arg0, int arg1, int arg2, int arg3,
                        int arg4, int arg5, int arg6, int arg7)
{
   uint8 *ptr;
   uint32 state;

   state = OS_CriticalBegin();
   sprintf(PrintfString, format, arg0, arg1, arg2, arg3,
           arg4, arg5, arg6, arg7);
   ptr = (uint8*)PrintfString;
   while(*ptr)
   {
      while((MemoryRead(IRQ_STATUS) & IRQ_UART_WRITE_AVAILABLE) == 0)
         ;
      MemoryWrite(UART_WRITE, *ptr++);
#ifdef SUPPORT_DATA_PACKETS
      if(UartPacketOut && UartPacketOutByte - 4 < UartPacketOutLength)
      {
         ++UartPacketOutByte;
         --ptr;
      }
#endif
   }
   memset(PrintfString, 0, sizeof(PrintfString));
   OS_CriticalEnd(state);
}


void UartScanf(const char *format,
               int arg0, int arg1, int arg2, int arg3,
               int arg4, int arg5, int arg6, int arg7)
{
   int index = 0, ch;
   OS_SemaphorePend(SemaphoreUart, OS_WAIT_FOREVER);
   for(;;)
   {
      ch = UartRead();
      if(ch != '\b' || index)
         UartWrite(ch);
      if(ch == '\n' || ch == '\r')
         break;
      else if(ch == '\b')
      {
         if(index)
         {
            UartWrite(' ');
            UartWrite(ch);
            --index;
         }
      }
      else if(index < sizeof(PrintfString))
         PrintfString[index++] = (uint8)ch;
   }
   UartWrite('\n');
   PrintfString[index] = 0;
   sscanf(PrintfString, format, arg0, arg1, arg2, arg3,
          arg4, arg5, arg6, arg7);
   OS_SemaphorePost(SemaphoreUart);
}


#ifdef SUPPORT_DATA_PACKETS
void UartPacketConfig(PacketGetFunc_t PacketGetFunc, 
                      int PacketSize, 
                      OS_MQueue_t *mQueue)
{
   UartPacketGet = PacketGetFunc;
   UartPacketSize = PacketSize;
   UartPacketMQueue = mQueue;
}


void UartPacketSend(uint8 *Data, int Bytes)
{
   UartPacketOutByte = 0;
   UartPacketOutLength = Bytes;
   UartPacketOut = Data;
   OS_InterruptMaskSet(IRQ_UART_WRITE_AVAILABLE);
}
#endif


void Led(int value)
{
   value |= 0xffffff00;
   MemoryWrite(GPIO0_OUT, value);  //Change LEDs
}


/******************************************/
#ifndef WIN32
int puts(const char *string)
{
   uint8 *ptr;
   OS_SemaphorePend(SemaphoreUart, OS_WAIT_FOREVER);
   ptr = (uint8*)string;
   while(*ptr)
   {
      if(*ptr == '\n')
         UartWrite('\r');
      UartWrite(*ptr++);
   }
   OS_SemaphorePost(SemaphoreUart);
   return 0;
}


int getch(void)
{
   return BufferRead(ReadBuffer, 1);
}


int kbhit(void)
{
   return ReadBuffer->read != ReadBuffer->write;
}
#endif


/******************************************/
#if 0
int LogArray[100], LogIndex;
void LogWrite(int a)
{
   if(LogIndex < sizeof(LogArray)/4)
      LogArray[LogIndex++] = a;
}

void LogDump(void)
{
   int i;
   for(i = 0; i < LogIndex; ++i)
   {
      if(LogArray[i] > 0xfff)
         UartPrintfCritical("\n", 0,0,0,0,0,0,0,0);
      UartPrintfCritical("0x%x ", LogArray[i], 0,0,0,0,0,0,0);
   }
   LogIndex = 0;
}
#endif

