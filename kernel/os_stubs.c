/*--------------------------------------------------------------------
 * TITLE: OS stubs
 * AUTHOR: Steve Rhoads (rhoadss@yahoo.com)
 * DATE CREATED: 2/18/08
 * FILENAME: os_stubs.c
 * PROJECT: Plasma CPU core
 * COPYRIGHT: Software placed into the public domain by the author.
 *    Software 'as is' without warranty.  Author liable for nothing.
 * DESCRIPTION:
 *--------------------------------------------------------------------*/
#include <stdlib.h>
#include "plasma.h"
#include "rtos.h"

unsigned char *flash;
int beenInit;


void FlashRead(uint16 *dst, uint32 byteOffset, int bytes)
{
   if(beenInit == 0)
   {
      beenInit = 1;
      flash = (unsigned char*)malloc(1024*1024*16);
      memset(flash, 0xff, sizeof(flash));
   }
   memcpy(dst, flash+byteOffset, bytes);
}


void FlashWrite(uint16 *src, uint32 byteOffset, int bytes)
{
   memcpy(flash+byteOffset, src, bytes);
}


void FlashErase(uint32 byteOffset)
{
   memset(flash+byteOffset, 0xff, 1024*128);
}


//Stub out RTOS functions
void UartPrintfCritical(const char *format, ...) {(void)format;}
uint32 OS_AsmInterruptEnable(uint32 state)   {(void)state; return 0;}
void OS_Assert(void)                         {}
OS_Thread_t *OS_ThreadSelf(void)             {return NULL;}
void OS_ThreadSleep(int ticks)               {(void)ticks;}
uint32 OS_ThreadTime(void)                   {return 0;}
OS_Mutex_t *OS_MutexCreate(const char *name) {(void)name; return NULL; }
void OS_MutexDelete(OS_Mutex_t *semaphore)   {(void)semaphore;}
void OS_MutexPend(OS_Mutex_t *semaphore)     {(void)semaphore;}
void OS_MutexPost(OS_Mutex_t *semaphore)     {(void)semaphore;}

OS_MQueue_t *OS_MQueueCreate(const char *name,
                             int messageCount,
                             int messageBytes)
{(void)name;(void)messageCount;(void)messageBytes; return NULL;}

void OS_MQueueDelete(OS_MQueue_t *mQueue)    {(void)mQueue;}

int OS_MQueueSend(OS_MQueue_t *mQueue, void *message) 
{(void)mQueue;(void)message; return 0;}

int OS_MQueueGet(OS_MQueue_t *mQueue, void *message, int ticks)
{(void)mQueue;(void)message;(void)ticks; return 0;}

void OS_Job(void (*funcPtr)(), void *arg0, void *arg1, void *arg2)
{funcPtr(arg0, arg1, arg2);}


