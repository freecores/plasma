/*--------------------------------------------------------------------
 * TITLE: Test Plasma Real Time Operating System
 * AUTHOR: Steve Rhoads (rhoadss@yahoo.com)
 * DATE CREATED: 1/1/06
 * FILENAME: rtos_test.c
 * PROJECT: Plasma CPU core
 * COPYRIGHT: Software placed into the public domain by the author.
 *    Software 'as is' without warranty.  Author liable for nothing.
 * DESCRIPTION:
 *    Test Plasma Real Time Operating System
 *--------------------------------------------------------------------*/
#include "plasma.h"
#include "rtos.h"

extern void TestMathFull(void);
OS_FuncPtr_t FuncPtr;

//******************************************************************
static void TestCLib(void)
{
   char s1[80], s2[80], *ptr;
   int rc, v1, v2, v3;

   printf("TestCLib\n");
   strcpy(s1, "Hello ");
   strncpy(s2, "World wide", 5);
   strcat(s1, s2);
   strncat(s1, "!\nthing", 14);
   printf("%s", s1);
   rc = strcmp(s1, "Hello World!\n");
   assert(rc == 0);
   rc = strcmp(s1, "Hello WOrld!\n");
   assert(rc > 0);
   rc = strcmp(s1, "Hello world!\n");
   assert(rc < 0);
   rc = strncmp(s1, "Hellx", 4);
   assert(rc == 0);
   ptr = strstr(s1, "orl");
   assert(ptr[0] = 'o');
   rc = strlen(s1);
   assert(rc == 13);
   memcpy(s2, s1, rc+1);
   rc = memcmp(s1, s2, 8);
   assert(rc == 0);
   s2[5] = 'z';
   rc = memcmp(s1, s2, 8);
   assert(rc != 0);
   memset(s2, 0, 5);
   memset(s2, 'a', 3);
   rc = abs(-5);
   itoa(s1, 1234, 10, 8);
   itoa(s1, 1234, 10, 20);
   itoa(s1, 0, 10, 8);
   itoa(s1, 0, 10, 0);
   itoa(s1, -1234, 10, 8);
   itoa(s1, -1234, 10, 20);
   itoa(s1, 0xabcd, 16, 8);
   itoa(s1, 0x12ab, 16, 20);
   sprintf(s1, "test c%c d%d x%x s%s End\n", 'C', 1234, 0xabcd, "String");
   printf("%s", s1);
   sprintf(s1, "test c%c d%6d x%6x s%8s End\n", 'C', 1234, 0xabcd, "String");
   printf("%s", s1);
   sscanf("1234 -1234 0xabcd text h", "%d %d %x %s", &v1, &v2, &v3, s1);
   assert(v1 == 1234 && v2 == -1234 && v3 == 0xabcd);
   assert(strcmp(s1, "text") == 0);
   //UartScanf("%d %d", &v1, &v2);
   //printf("v1 = %d v2 = %d\n", v1, v2);
   printf("Done.\n");
}

//******************************************************************
static void TestHeap(void)
{
   uint8 *ptrs[256], size[256], *ptr;
   int i, j, k, value;

   printf("TestHeap\n");
   memset(ptrs, 0, sizeof(ptrs));
   for(i = 0; i < 1000; ++i)
   {
      j = rand() & 255;
      if(ptrs[j])
      {
         ptr = ptrs[j];
         value = size[j];
         for(k = 0; k < value; ++k)
         {
            if(ptr[k] != value)
               printf("Error\n");
         }
         OS_HeapFree(ptrs[j]);
      }
      size[j] = (uint8)(rand() & 255);
      ptrs[j] = OS_HeapMalloc(NULL, size[j]);
      memset(ptrs[j], size[j], size[j]);
   }
   for(i = 0; i < 256; ++i)
   {
      if(ptrs[i])
         OS_HeapFree(ptrs[i]);
   }
   printf("Done.\n");
}

//******************************************************************
static void MyThreadMain(void *Arg)
{
   OS_Thread_t *thread;
   int priority;

   thread = OS_ThreadSelf();
   priority = OS_ThreadPriorityGet(thread);
   OS_ThreadSleep(10);
   printf("Arg=%d thread=0x%x info=0x%x priority=%d\n", 
      (uint32)Arg, thread, OS_ThreadInfoGet(thread), priority);
}

static void TestThread(void)
{
   OS_Thread_t *thread;
   int i, priority;

   printf("TestThread\n");
   for(i = 0; i < 32; ++i)
   {
      priority = 50 + i;
      thread = OS_ThreadCreate("MyThread", MyThreadMain, (uint32*)i, priority, 0);
      OS_ThreadInfoSet(thread, (void*)(0xabcd + i));
      //printf("Created thread 0x%x\n", thread);
   }

   thread = OS_ThreadSelf();
   priority = OS_ThreadPriorityGet(thread);
   printf("Priority = %d\n", priority);
   OS_ThreadPrioritySet(thread, 200);
   printf("Priority = %d\n", OS_ThreadPriorityGet(thread));
   OS_ThreadPrioritySet(thread, priority);

   printf("Thread time = %d\n", OS_ThreadTime());
   OS_ThreadSleep(100);
   printf("Thread time = %d\n", OS_ThreadTime());
}

//******************************************************************
static OS_Semaphore_t *MySemaphore[100];
static void TestSemThread(void *Arg)
{
   int i;
   (void)Arg;

   for(i = 0; i < 50; ++i)
   {
      printf("s");
      OS_SemaphorePend(MySemaphore[i], OS_WAIT_FOREVER);
      OS_SemaphorePost(MySemaphore[i + 50]);
   }
}

static void TestSemaphore(void)
{
   int i, rc;
   printf("TestSemaphore\n");
   for(i = 0; i < 100; ++i)
   {
      MySemaphore[i] = OS_SemaphoreCreate("MySem", 0);
      //printf("sem[%d]=0x%x\n", i, MySemaphore[i]);
   }

   OS_ThreadCreate("TestSem", TestSemThread, NULL, 50, 0);

   for(i = 0; i < 50; ++i)
   {
      printf("S");
      OS_SemaphorePost(MySemaphore[i]);
      rc = OS_SemaphorePend(MySemaphore[i + 50], 500);
      assert(rc == 0);
   }

   printf(":");
   rc = OS_SemaphorePend(MySemaphore[0], 10);
   assert(rc != 0);
   printf(":");
   OS_SemaphorePend(MySemaphore[0], 100);
   printf(":");

   for(i = 0; i < 100; ++i)
      OS_SemaphoreDelete(MySemaphore[i]);

   printf("\nDone.\n");
}

//******************************************************************
static OS_Mutex_t *MyMutex;
static void TestMutexThread(void *Arg)
{
   (void)Arg;

   printf("Waiting for mutex\n");
   OS_MutexPend(MyMutex);
   printf("Have Mutex1\n");
   OS_MutexPend(MyMutex);
   printf("Have Mutex2\n");
   OS_MutexPend(MyMutex);
   printf("Have Mutex3\n");

   OS_ThreadSleep(100);

   OS_MutexPost(MyMutex);
   OS_MutexPost(MyMutex);
   OS_MutexPost(MyMutex);
}

static void TestMutex(void)
{
   printf("TestMutex\n");
   MyMutex = OS_MutexCreate("MyMutex");
   OS_MutexPend(MyMutex);
   OS_MutexPend(MyMutex);
   OS_MutexPend(MyMutex);

   OS_ThreadCreate("TestMutex", TestMutexThread, NULL, 50, 0);

   OS_ThreadSleep(50);
   OS_MutexPost(MyMutex);
   OS_MutexPost(MyMutex);
   OS_MutexPost(MyMutex);

   printf("Try get mutex\n");
   OS_MutexPend(MyMutex);
   printf("Gotit\n");

   OS_MutexDelete(MyMutex);
   printf("Done.\n");
}

//******************************************************************
static void TestMQueue(void)
{
   OS_MQueue_t *mqueue;
   char data[16];
   int i, rc;

   printf("TestMQueue\n");
   mqueue = OS_MQueueCreate("MyMQueue", 10, 16);
   strcpy(data, "Test0");
   for(i = 0; i < 16; ++i)
   {
      data[4] = (char)('0' + i);
      OS_MQueueSend(mqueue, data);
   }
   for(i = 0; i < 16; ++i)
   {
      memset(data, 0, sizeof(data));
      rc = OS_MQueueGet(mqueue, data, 20);
      if(rc == 0)
         printf("message=(%s)\n", data);
      else
         printf("timeout\n");
   }

   OS_MQueueDelete(mqueue);
   printf("Done.\n");
}

//******************************************************************
#define TIMER_COUNT 10
static OS_Timer_t *MyTimer[TIMER_COUNT];
static OS_MQueue_t *MyQueue[TIMER_COUNT];
static int TimerDone;
static void TestTimerThread(void *Arg)
{
   int index = (int)Arg;
   uint32 data[4];
   OS_Timer_t *timer;

   OS_MQueueGet(MyQueue[index], data, 1000);
   timer = (OS_Timer_t*)data[1];
   printf("%d ", data[2]);
   OS_MQueueGet(MyQueue[index], data, 1000);
   printf("%d ", data[2]);
   ++TimerDone;
}

static void TestTimer(void)
{
   int i;

   printf("TestTimer\n");
   TimerDone = 0;
   for(i = 0; i < TIMER_COUNT; ++i)
   {
      MyQueue[i] = OS_MQueueCreate("MyQueue", 10, 16);
      MyTimer[i] = OS_TimerCreate("MyTimer", MyQueue[i], i);
      OS_ThreadCreate("TimerTest", TestTimerThread, (uint32*)i, 50, 0);
      OS_TimerStart(MyTimer[i], 10+i*2, 220+i);
   }

   while(TimerDone < TIMER_COUNT)
      OS_ThreadSleep(10);

   for(i = 0; i < TIMER_COUNT; ++i)
   {
      OS_MQueueDelete(MyQueue[i]);
      OS_TimerDelete(MyTimer[i]);
   }

   printf("Done.\n");
}

//******************************************************************
#if 1
void TestMath(void)
{
   int i;
   float a, b, sum, diff, mult, div;
   uint32 compare;

   //Check add subtract multiply and divide
   for(i = -4; i < 4; ++i)
   {
      a = (float)(i * 10 + (float)63.2);
      b = (float)(-i * 5 + (float)3.5);
      sum = a + b;
      diff = a - b;
      mult = a * b;
      div = a / b;
      printf("a=%dE-3 b=%dE-3 sum=%dE-3 diff=%dE-3 mult=%dE-3 div=%dE-3\n",
         (int)(a*(float)1000), (int)(b*(float)1000), 
         (int)(sum*(float)1000), (int)(diff*(float)1000),
         (int)(mult*(float)1000), (int)(div*(float)1000));
   }

   //Comparisons
   b = (float)2.0;
   compare = 0;
   for(i = 1; i < 4; ++i)
   {
      a = (float)i;
      compare = (compare << 1) | (a == b);
      compare = (compare << 1) | (a != b);
      compare = (compare << 1) | (a <  b);
      compare = (compare << 1) | (a <= b);
      compare = (compare << 1) | (a >  b);
      compare = (compare << 1) | (a >= b);
   }
   printf("Compare = %8x %s\n", compare, 
      compare==0x1c953 ? "OK" : "ERROR");

   //Cosine
   for(a = (float)0.0; a <= (float)(3.1415); a += (float)(3.1415/16.0))
   {
      b = FP_Cos(a);
      printf("cos(%4dE-3) = %4dE-3\n", 
         (int)(a*(float)1000.0), (int)(b*(float)1000.0));
   }
}
#endif


//******************************************************************
void MainThread(void *Arg)
{
   int ch;
   (void)Arg;

   for(;;)
   {
      printf("\n");
      printf("0 Exit\n");
      printf("1 CLib\n");
      printf("2 Heap\n");
      printf("3 Thread\n");
      printf("4 Semaphore\n");
      printf("5 Mutex\n");
      printf("6 MQueue\n");
      printf("7 Timer\n");
      printf("8 Math\n");
      printf("> ");
      ch = UartRead();
      printf("%c\n", ch);
      switch(ch)
      {
      case '0': 
#ifndef WIN32
         OS_CriticalBegin();
         FuncPtr(NULL);
#endif
         return;
      case '1': TestCLib(); break;
      case '2': TestHeap(); break;
      case '3': TestThread(); break;
      case '4': TestSemaphore(); break;
      case '5': TestMutex(); break;
      case '6': TestMQueue(); break;
      case '7': TestTimer(); break;
      case '8': TestMath(); break;
#ifdef WIN32
      case 'm': TestMathFull(); break;
#endif
      }
   }
}


