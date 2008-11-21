/*--------------------------------------------------------------------
 * TITLE: Plasma Real Time Operating System
 * AUTHOR: Steve Rhoads (rhoadss@yahoo.com)
 * DATE CREATED: 12/17/05
 * FILENAME: rtos.c
 * PROJECT: Plasma CPU core
 * COPYRIGHT: Software placed into the public domain by the author.
 *    Software 'as is' without warranty.  Author liable for nothing.
 * DESCRIPTION:
 *    Plasma Real Time Operating System
 *    Fully pre-emptive RTOS with support for:
 *       Heaps, Threads, Semaphores, Mutexes, Message Queues, and Timers.
 *    This file tries to be hardware independent except for calls to:
 *       MemoryRead() and MemoryWrite() for interrupts.
 *    Partial support for multiple CPUs using symmetric multiprocessing.
 *--------------------------------------------------------------------*/
#include "plasma.h"
#include "rtos.h"

#define HEAP_MAGIC 0x1234abcd
#define THREAD_MAGIC 0x4321abcd
#define SEM_RESERVED_COUNT 2
#define INFO_COUNT 4
#define HEAP_COUNT 8


/*************** Structures ***************/
#ifdef WIN32
   #define setjmp _setjmp
   //x86 registers
   typedef struct jmp_buf2 {  
      uint32 Ebp, Ebx, Edi, Esi, sp, pc, extra[10];
   } jmp_buf2;
#elif defined(ARM_CPU)
   //ARM registers
   typedef struct jmp_buf2 {  
      uint32 r[13], sp, lr, pc, cpsr, extra[5];
   } jmp_buf2;
#else  
   //Plasma registers
   typedef struct jmp_buf2 {  
      uint32 s[9], gp, sp, pc;
   } jmp_buf2;
#endif

typedef struct HeapNode_s {
   struct HeapNode_s *next;
   int size;
} HeapNode_t;

struct OS_Heap_s {
   uint32 magic;
   const char *name;
   OS_Semaphore_t *semaphore;
   HeapNode_t *available;
   HeapNode_t base;
   struct OS_Heap_s *alternate;
};
//typedef struct OS_Heap_s OS_Heap_t;

typedef enum {
   THREAD_PEND    = 0,       //Thread in semaphore's linked list
   THREAD_READY   = 1,       //Thread in ThreadHead linked list
   THREAD_RUNNING = 2        //Thread == ThreadCurrent[cpu]
} OS_ThreadState_e;

struct OS_Thread_s {
   const char *name;         //Name of thread
   OS_ThreadState_e state;   //Pending, ready, or running
   int cpuIndex;             //Which CPU is running the thread
   int cpuLock;              //Lock the thread to a specific CPU
   jmp_buf env;              //Registers saved during context swap
   OS_FuncPtr_t funcPtr;     //First function called
   void *arg;                //Argument to first function called
   uint32 priority;          //Priority of thread (0=low, 255=high)
   uint32 ticksTimeout;      //Tick value when semaphore pend times out
   void *info[INFO_COUNT];   //User storage
   OS_Semaphore_t *semaphorePending;  //Semaphore thread is blocked on
   int returnCode;           //Return value from semaphore pend
   uint32 processId;         //Process ID if using MMU
   OS_Heap_t *heap;          //Heap used if no heap specified
   struct OS_Thread_s *next; //Linked list of threads by priority
   struct OS_Thread_s *prev;  
   struct OS_Thread_s *nextTimeout; //Linked list of threads by timeout
   struct OS_Thread_s *prevTimeout; 
   uint32 magic[1];          //Bottom of stack to detect stack overflow
};
//typedef struct OS_Thread_s OS_Thread_t;

struct OS_Semaphore_s {
   const char *name;
   struct OS_Thread_s *threadHead; //threads pending on semaphore
   int count;
};
//typedef struct OS_Semaphore_s OS_Semaphore_t;

struct OS_Mutex_s {
   OS_Semaphore_t *semaphore;
   OS_Thread_t *thread;
   int count;
}; 
//typedef struct OS_Mutex_s OS_Mutex_t;

struct OS_MQueue_s {
   const char *name;
   OS_Semaphore_t *semaphore;
   int count, size, used, read, write;
};
//typedef struct OS_MQueue_s OS_MQueue_t;

struct OS_Timer_s {
   const char *name;
   struct OS_Timer_s *next, *prev;
   uint32 ticksTimeout;
   uint32 ticksRestart;
   int active;
   OS_TimerFuncPtr_t callback;
   OS_MQueue_t *mqueue;
   uint32 info;
}; 
//typedef struct OS_Timer_s OS_Timer_t;


/*************** Globals ******************/
static OS_Heap_t *HeapArray[HEAP_COUNT];
static int InterruptInside[OS_CPU_COUNT];
static int ThreadNeedReschedule[OS_CPU_COUNT];
static OS_Thread_t *ThreadCurrent[OS_CPU_COUNT];  //Currently running thread(s)
static OS_Thread_t *ThreadHead;   //Linked list of threads sorted by priority
static OS_Thread_t *TimeoutHead;  //Linked list of threads sorted by timeout
static int ThreadSwapEnabled;
static uint32 ThreadTime;
static void *NeedToFree;
static OS_Semaphore_t SemaphoreReserved[SEM_RESERVED_COUNT];
static OS_Semaphore_t *SemaphoreSleep;
static OS_Semaphore_t *SemaphoreRelease;
static OS_Semaphore_t *SemaphoreLock;
static OS_Semaphore_t *SemaphoreTimer;
static OS_Timer_t *TimerHead;     //Linked list of timers sorted by timeout
static OS_FuncPtr_t Isr[32];


/***************** Heap *******************/
/******************************************/
OS_Heap_t *OS_HeapCreate(const char *name, void *memory, uint32 size)
{
   OS_Heap_t *heap;

   assert(((uint32)memory & 3) == 0);
   heap = (OS_Heap_t*)memory;
   heap->magic = HEAP_MAGIC;
   heap->name = name;
   heap->semaphore = OS_SemaphoreCreate(name, 1);
   heap->available = (HeapNode_t*)(heap + 1);
   heap->available->next = &heap->base;
   heap->available->size = (size - sizeof(OS_Heap_t)) / sizeof(HeapNode_t);
   heap->base.next = heap->available;
   heap->base.size = 0;
   return heap;
}


/******************************************/
void OS_HeapDestroy(OS_Heap_t *heap)
{
   OS_SemaphoreDelete(heap->semaphore);
}


/******************************************/
//Modified from K&R
void *OS_HeapMalloc(OS_Heap_t *heap, int bytes)
{
   HeapNode_t *node, *prevp;
   int nunits;

   if(heap == NULL && OS_ThreadSelf())
      heap = OS_ThreadSelf()->heap;
   if((uint32)heap < HEAP_COUNT)
      heap = HeapArray[(int)heap];
   nunits = (bytes + sizeof(HeapNode_t) - 1) / sizeof(HeapNode_t) + 1;
   OS_SemaphorePend(heap->semaphore, OS_WAIT_FOREVER);
   prevp = heap->available;
   for(node = prevp->next; ; prevp = node, node = node->next)
   {
      if(node->size >= nunits)       //Big enough?
      {
         if(node->size == nunits)    //Exactly
            prevp->next = node->next;
         else
         {                           //Allocate tail end
            node->size -= nunits;
            node += node->size;
            node->size = nunits;
         }
         heap->available = prevp;
         node->next = (HeapNode_t*)heap;
         OS_SemaphorePost(heap->semaphore);
         return (void*)(node + 1);
      }
      if(node == heap->available)   //Wrapped around free list
      {
         OS_SemaphorePost(heap->semaphore);
         if(heap->alternate)
            return OS_HeapMalloc(heap->alternate, bytes);
         return NULL;
      }
   }
}


/******************************************/
//Modified from K&R
void OS_HeapFree(void *block)
{
   OS_Heap_t *heap;
   HeapNode_t *bp, *node;

   assert(block);
   bp = (HeapNode_t*)block - 1;   //point to block header
   heap = (OS_Heap_t*)bp->next;
   assert(heap->magic == HEAP_MAGIC);
   if(heap->magic != HEAP_MAGIC)
      return;
   OS_SemaphorePend(heap->semaphore, OS_WAIT_FOREVER);
   for(node = heap->available; !(node < bp && bp < node->next); node = node->next)
   {
      if(node >= node->next && (bp > node || bp < node->next))
         break;               //freed block at start or end of area
   }

   if(bp + bp->size == node->next)   //join to upper
   {
      bp->size += node->next->size;
      bp->next = node->next->next;
   }
   else
   {
      bp->next = node->next;
   }

   if(node + node->size == bp)       //join to lower
   {
      node->size += bp->size;
      node->next = bp->next;
   }
   else
      node->next = bp;
   heap->available = node;
   OS_SemaphorePost(heap->semaphore);
}


/******************************************/
void OS_HeapAlternate(OS_Heap_t *heap, OS_Heap_t *alternate)
{
   heap->alternate = alternate;
}


/******************************************/
void OS_HeapRegister(void *index, OS_Heap_t *heap)
{
   if((uint32)index < HEAP_COUNT)
      HeapArray[(int)index] = heap;
}



/***************** Thread *****************/
/******************************************/
//Linked list of threads sorted by priority
//The listed list is either ThreadHead (ready to run threads not including
//the currently running thread) or a list of threads waiting on a semaphore.
//Must be called with interrupts disabled
static void OS_ThreadPriorityInsert(OS_Thread_t **head, OS_Thread_t *thread)
{
   OS_Thread_t *node, *prev;

   prev = NULL;
   for(node = *head; node; node = node->next)
   {
      if(node->priority < thread->priority)
         break;
      prev = node;
   }

   if(prev == NULL)
   {
      thread->next = *head;
      thread->prev = NULL;
      if(*head)
         (*head)->prev = thread;
      *head = thread;
   }
   else
   {
      if(prev->next)
         prev->next->prev = thread;
      thread->next = prev->next;
      thread->prev = prev;
      prev->next = thread;
   }
   assert(ThreadHead);
   thread->state = THREAD_READY;
}


/******************************************/
//Must be called with interrupts disabled
static void OS_ThreadPriorityRemove(OS_Thread_t **head, OS_Thread_t *thread)
{
   assert(thread->magic[0] == THREAD_MAGIC);  //check stack overflow
   if(thread->prev == NULL)
      *head = thread->next;
   else
      thread->prev->next = thread->next;
   if(thread->next)
      thread->next->prev = thread->prev;
   thread->next = NULL;
   thread->prev = NULL;
}


/******************************************/
//Linked list of threads sorted by timeout value
//Must be called with interrupts disabled
static void OS_ThreadTimeoutInsert(OS_Thread_t *thread)
{
   OS_Thread_t *node, *prev;
   int diff;

   prev = NULL;
   for(node = TimeoutHead; node; node = node->nextTimeout)
   {
      diff = thread->ticksTimeout - node->ticksTimeout;
      if(diff <= 0)
         break;
      prev = node;
   }

   if(prev == NULL)
   {
      thread->nextTimeout = TimeoutHead;
      thread->prevTimeout = NULL;
      if(TimeoutHead)
         TimeoutHead->prevTimeout = thread;
      TimeoutHead = thread;
   }
   else
   {
      if(prev->nextTimeout)
         prev->nextTimeout->prevTimeout = thread;
      thread->nextTimeout = prev->nextTimeout;
      thread->prevTimeout = prev;
      prev->nextTimeout = thread;
   }
}


/******************************************/
//Must be called with interrupts disabled
static void OS_ThreadTimeoutRemove(OS_Thread_t *thread)
{
   if(thread->prevTimeout == NULL && TimeoutHead != thread)
      return;         //not in list
   if(thread->prevTimeout == NULL)
      TimeoutHead = thread->nextTimeout;
   else
      thread->prevTimeout->nextTimeout = thread->nextTimeout;
   if(thread->nextTimeout)
      thread->nextTimeout->prevTimeout = thread->prevTimeout;
   thread->nextTimeout = NULL;
   thread->prevTimeout = NULL;
}


/******************************************/
//Loads highest priority thread from the ThreadHead linked list
//The currently running thread isn't in the ThreadHead list
//Must be called with interrupts disabled
static void OS_ThreadReschedule(int roundRobin)
{
   OS_Thread_t *threadNext, *threadCurrent;
   int rc, cpuIndex = OS_CpuIndex();

   if(ThreadSwapEnabled == 0 || InterruptInside[cpuIndex])
   {
      ThreadNeedReschedule[cpuIndex] |= 2 + roundRobin;  //Reschedule later
      return;
   }

   //Determine which thread should run
   threadNext = ThreadHead;
   while(threadNext && threadNext->cpuLock != -1 && 
         threadNext->cpuLock != cpuIndex)
      threadNext = threadNext->next;
   if(threadNext == NULL)
      return;
   threadCurrent = ThreadCurrent[cpuIndex];

   if(threadCurrent == NULL || 
      threadCurrent->state == THREAD_PEND ||
      threadCurrent->priority < threadNext->priority ||
      (roundRobin && threadCurrent->priority == threadNext->priority))
   {
      //Swap threads
      ThreadCurrent[cpuIndex] = threadNext;
      if(threadCurrent)
      {
         assert(threadCurrent->magic[0] == THREAD_MAGIC); //check stack overflow
         if(threadCurrent->state == THREAD_RUNNING)
            OS_ThreadPriorityInsert(&ThreadHead, threadCurrent);
         rc = setjmp(threadCurrent->env);  //ANSI C call to save registers
         if(rc)
            return;  //Returned from longjmp()
      }

      //Remove the new running thread from the ThreadHead linked list
      threadNext = ThreadCurrent[OS_CpuIndex()]; //removed warning
      assert(threadNext->state == THREAD_READY);
      OS_ThreadPriorityRemove(&ThreadHead, threadNext); 
      threadNext->state = THREAD_RUNNING;               
      threadNext->cpuIndex = OS_CpuIndex();
      longjmp(threadNext->env, 1);         //ANSI C call to restore registers
   }
}


/******************************************/
void OS_ThreadCpuLock(OS_Thread_t *thread, int cpuIndex)
{
   thread->cpuLock = cpuIndex;
   if(thread == OS_ThreadSelf() && cpuIndex != (int)OS_CpuIndex())
      OS_ThreadSleep(1);
}


/******************************************/
static void OS_ThreadInit(void *arg)
{
   uint32 cpuIndex = OS_CpuIndex();
   (void)arg;

   OS_CriticalEnd(1);
   ThreadCurrent[cpuIndex]->funcPtr(ThreadCurrent[cpuIndex]->arg);
   OS_ThreadExit();
}


/******************************************/
//Stops warning "argument X might be clobbered by `longjmp'"
static void OS_ThreadRegsInit(jmp_buf env)
{
   setjmp(env); //ANSI C call to save registers
}


/******************************************/
OS_Thread_t *OS_ThreadCreate(const char *name,
                             OS_FuncPtr_t funcPtr, 
                             void *arg, 
                             uint32 priority, 
                             uint32 stackSize)
{
   OS_Thread_t *thread;
   uint8 *stack;
   jmp_buf2 *env;
   uint32 state;

   OS_SemaphorePend(SemaphoreRelease, OS_WAIT_FOREVER);
   if(NeedToFree)
      OS_HeapFree(NeedToFree);
   NeedToFree = NULL;
   OS_SemaphorePost(SemaphoreRelease);

   if(stackSize == 0)
      stackSize = STACK_SIZE_DEFAULT;
   if(stackSize < STACK_SIZE_MINIMUM)
      stackSize = STACK_SIZE_MINIMUM;
   thread = (OS_Thread_t*)OS_HeapMalloc(NULL, sizeof(OS_Thread_t) + stackSize);
   assert(thread);
   if(thread == NULL)
      return NULL;
   memset(thread, 0, sizeof(OS_Thread_t));
   stack = (uint8*)(thread + 1);
   memset(stack, 0xcd, stackSize);

   thread->name = name;
   thread->state = THREAD_READY;
   thread->cpuLock = -1;
   thread->funcPtr = funcPtr;
   thread->arg = arg;
   thread->priority = priority;
   thread->semaphorePending = NULL;
   thread->returnCode = 0;
   if(OS_ThreadSelf())
   {
      thread->processId = OS_ThreadSelf()->processId;
      thread->heap = OS_ThreadSelf()->heap;
   }
   else
   {
      thread->processId = 0;
      thread->heap = NULL;
   }
   thread->next = NULL;
   thread->prev = NULL;
   thread->nextTimeout = NULL;
   thread->prevTimeout = NULL;
   thread->magic[0] = THREAD_MAGIC;

   OS_ThreadRegsInit(thread->env);
   env = (jmp_buf2*)thread->env;
   env->sp = (uint32)stack + stackSize - 24; //minimum stack frame size
   env->pc = (uint32)OS_ThreadInit;

   state = OS_CriticalBegin();
   OS_ThreadPriorityInsert(&ThreadHead, thread);
   OS_ThreadReschedule(0);
   OS_CriticalEnd(state);
   return thread;
}


/******************************************/
void OS_ThreadExit(void)
{
   uint32 state, cpuIndex = OS_CpuIndex();

   for(;;)
   {
      OS_SemaphorePend(SemaphoreRelease, OS_WAIT_FOREVER);
      if(NeedToFree)
         OS_HeapFree(NeedToFree);
      NeedToFree = NULL;
      OS_SemaphorePost(SemaphoreRelease);

      state = OS_CriticalBegin();
      if(NeedToFree)
      {
         OS_CriticalEnd(state);
         continue;
      }
      ThreadCurrent[cpuIndex]->state = THREAD_PEND;
      NeedToFree = ThreadCurrent[cpuIndex];
      OS_ThreadReschedule(0);
      OS_CriticalEnd(state);
   }
}


/******************************************/
OS_Thread_t *OS_ThreadSelf(void)
{
   return ThreadCurrent[OS_CpuIndex()];
}


/******************************************/
void OS_ThreadSleep(int ticks)
{
   OS_SemaphorePend(SemaphoreSleep, ticks);
}


/******************************************/
uint32 OS_ThreadTime(void)
{
   return ThreadTime;
}


/******************************************/
void OS_ThreadInfoSet(OS_Thread_t *thread, uint32 index, void *Info)
{
   if(index < INFO_COUNT)
      thread->info[index] = Info;
}


/******************************************/
void *OS_ThreadInfoGet(OS_Thread_t *thread, uint32 index)
{
   if(index < INFO_COUNT)
      return thread->info[index];
   return NULL;
}


/******************************************/
uint32 OS_ThreadPriorityGet(OS_Thread_t *thread)
{
   return thread->priority;
}


/******************************************/
void OS_ThreadPrioritySet(OS_Thread_t *thread, uint32 priority)
{
   uint32 state;
   state = OS_CriticalBegin();
   thread->priority = priority;
   if(thread->state == THREAD_READY)
   {
      OS_ThreadPriorityRemove(&ThreadHead, thread);
      OS_ThreadPriorityInsert(&ThreadHead, thread);
      OS_ThreadReschedule(0);
   }
   OS_CriticalEnd(state);
}


/******************************************/
void OS_ThreadProcessId(OS_Thread_t *thread, uint32 processId, OS_Heap_t *heap)
{
   thread->processId = processId;
   thread->heap = heap;
}


/******************************************/
//Must be called with interrupts disabled
void OS_ThreadTick(void *Arg)
{
   OS_Thread_t *thread;
   OS_Semaphore_t *semaphore;
   int diff;
   (void)Arg;

   ++ThreadTime;
   while(TimeoutHead)
   {
      thread = TimeoutHead;
      diff = ThreadTime - thread->ticksTimeout;
      if(diff < 0)
         break;
      OS_ThreadTimeoutRemove(thread);
      semaphore = thread->semaphorePending;
      ++semaphore->count;
      thread->semaphorePending = NULL;
      thread->returnCode = -1;
      OS_ThreadPriorityRemove(&semaphore->threadHead, thread);
      OS_ThreadPriorityInsert(&ThreadHead, thread);
   }
   OS_ThreadReschedule(1);
}



/***************** Semaphore **************/
/******************************************/
OS_Semaphore_t *OS_SemaphoreCreate(const char *name, uint32 count)
{
   OS_Semaphore_t *semaphore;
   static int semCount = 0;

   if(semCount < SEM_RESERVED_COUNT)
      semaphore = &SemaphoreReserved[semCount++];  //Heap not ready yet
   else
      semaphore = (OS_Semaphore_t*)OS_HeapMalloc(HEAP_SYSTEM, sizeof(OS_Semaphore_t));
   assert(semaphore);
   if(semaphore == NULL)
      return NULL;

   semaphore->name = name;
   semaphore->threadHead = NULL;
   semaphore->count = count;
   return semaphore;
}


/******************************************/
void OS_SemaphoreDelete(OS_Semaphore_t *semaphore)
{
   while(semaphore->threadHead)
      OS_SemaphorePost(semaphore);
   OS_HeapFree(semaphore);
}


/******************************************/
int OS_SemaphorePend(OS_Semaphore_t *semaphore, int ticks)
{
   uint32 state, cpuIndex;
   OS_Thread_t *thread;
   int returnCode=0;

   assert(semaphore);
   assert(InterruptInside[OS_CpuIndex()] == 0);
   state = OS_CriticalBegin();
   if(--semaphore->count < 0)
   {
      if(ticks == 0)
      {
         ++semaphore->count;
         OS_CriticalEnd(state);
         return -1;
      }
      cpuIndex = OS_CpuIndex();
      thread = ThreadCurrent[cpuIndex];
      assert(thread);
      thread->semaphorePending = semaphore;
      thread->ticksTimeout = ticks + OS_ThreadTime();
      //FYI: The current thread isn't in the ThreadHead linked list
      OS_ThreadPriorityInsert(&semaphore->threadHead, thread);
      thread->state = THREAD_PEND;
      if(ticks != OS_WAIT_FOREVER)
         OS_ThreadTimeoutInsert(thread);
      assert(ThreadHead);
      OS_ThreadReschedule(0);
      returnCode = thread->returnCode;
   }
   OS_CriticalEnd(state);
   return returnCode;
}


/******************************************/
void OS_SemaphorePost(OS_Semaphore_t *semaphore)
{
   uint32 state;
   OS_Thread_t *thread;

   assert(semaphore);
   state = OS_CriticalBegin();
   if(++semaphore->count <= 0)
   {
      thread = semaphore->threadHead;
      OS_ThreadTimeoutRemove(thread);
      OS_ThreadPriorityRemove(&semaphore->threadHead, thread);
      OS_ThreadPriorityInsert(&ThreadHead, thread);
      thread->semaphorePending = NULL;
      thread->returnCode = 0;
      OS_ThreadReschedule(0);
   }
   OS_CriticalEnd(state);
}



/***************** Mutex ******************/
/******************************************/
OS_Mutex_t *OS_MutexCreate(const char *name)
{
   OS_Mutex_t *mutex;

   mutex = (OS_Mutex_t*)OS_HeapMalloc(HEAP_SYSTEM, sizeof(OS_Mutex_t));
   if(mutex == NULL)
      return NULL;
   mutex->semaphore = OS_SemaphoreCreate(name, 1);
   if(mutex->semaphore == NULL)
      return NULL;
   mutex->thread = NULL;
   mutex->count = 0;
   return mutex;
}


/******************************************/
void OS_MutexDelete(OS_Mutex_t *mutex)
{
   OS_SemaphoreDelete(mutex->semaphore);
   OS_HeapFree(mutex);
}


/******************************************/
void OS_MutexPend(OS_Mutex_t *mutex)
{
   OS_Thread_t *thread;

   assert(mutex);
   thread = OS_ThreadSelf();
   if(thread == mutex->thread)
   {
      ++mutex->count;
      return;
   }
   OS_SemaphorePend(mutex->semaphore, OS_WAIT_FOREVER);
   mutex->thread = thread;
   mutex->count = 1;
}


/******************************************/
void OS_MutexPost(OS_Mutex_t *mutex)
{
   assert(mutex);
   assert(mutex->thread == OS_ThreadSelf());
   assert(mutex->count > 0);
   if(--mutex->count <= 0)
   {
      mutex->thread = NULL;
      OS_SemaphorePost(mutex->semaphore);
   }
}



/***************** MQueue *****************/
/******************************************/
OS_MQueue_t *OS_MQueueCreate(const char *name,
                             int messageCount,
                             int messageBytes)
{
   OS_MQueue_t *queue;
   int size;

   size = messageBytes / sizeof(uint32);
   queue = (OS_MQueue_t*)OS_HeapMalloc(HEAP_SYSTEM, sizeof(OS_MQueue_t) + 
      messageCount * size * 4);
   if(queue == NULL)
      return queue;
   queue->name = name;
   queue->semaphore = OS_SemaphoreCreate(name, 0);
   if(queue->semaphore == NULL)
      return NULL;
   queue->count = messageCount;
   queue->size = size;
   queue->used = 0;
   queue->read = 0;
   queue->write = 0;
   return queue;
}


/******************************************/
void OS_MQueueDelete(OS_MQueue_t *mQueue)
{
   OS_SemaphoreDelete(mQueue->semaphore);
   OS_HeapFree(mQueue);
}


/******************************************/
int OS_MQueueSend(OS_MQueue_t *mQueue, void *message)
{
   uint32 state, *dst, *src;
   int i;

   assert(mQueue);
   src = (uint32*)message;
   state = OS_CriticalBegin();
   if(++mQueue->used > mQueue->count)
   {
      --mQueue->used;
      OS_CriticalEnd(state);
      return -1;
   }
   dst = (uint32*)(mQueue + 1) + mQueue->write * mQueue->size;
   for(i = 0; i < mQueue->size; ++i)
      dst[i] = src[i];
   if(++mQueue->write >= mQueue->count)
      mQueue->write = 0;
   OS_CriticalEnd(state);
   OS_SemaphorePost(mQueue->semaphore);
   return 0;
}


/******************************************/
int OS_MQueueGet(OS_MQueue_t *mQueue, void *message, int ticks)
{
   uint32 state, *dst, *src;
   int i, rc;

   assert(mQueue);
   dst = (uint32*)message;
   rc = OS_SemaphorePend(mQueue->semaphore, ticks);
   if(rc)
      return rc;
   state = OS_CriticalBegin();
   --mQueue->used;
   src = (uint32*)(mQueue + 1) + mQueue->read * mQueue->size;
   for(i = 0; i < mQueue->size; ++i)
      dst[i] = src[i];
   if(++mQueue->read >= mQueue->count)
      mQueue->read = 0;
   OS_CriticalEnd(state);
   return 0;
}



/***************** Jobs *******************/
/******************************************/
typedef void (*JobFunc_t)();
static OS_MQueue_t *jobQueue;
static OS_Thread_t *jobThread;

static void JobThread(void *arg)
{
   uint32 message[4];
   JobFunc_t funcPtr;
   (void)arg;
   for(;;)
   {
      OS_MQueueGet(jobQueue, message, OS_WAIT_FOREVER);
      funcPtr = (JobFunc_t)message[0];
      funcPtr(message[1], message[2], message[3]);
   }
}


/******************************************/
void OS_Job(void (*funcPtr)(), void *arg0, void *arg1, void *arg2)
{
   uint32 message[4];
   int rc;

   OS_SemaphorePend(SemaphoreLock, OS_WAIT_FOREVER);
   if(jobThread == NULL)
   {
      jobQueue = OS_MQueueCreate("job", 100, 16);
      jobThread = OS_ThreadCreate("job", JobThread, NULL, 150, 4000);
   }
   OS_SemaphorePost(SemaphoreLock);

   message[0] = (uint32)funcPtr;
   message[1] = (uint32)arg0;
   message[2] = (uint32)arg1;
   message[3] = (uint32)arg2;
   rc = OS_MQueueSend(jobQueue, message);
}


/***************** Timer ******************/
/******************************************/
static void OS_TimerThread(void *arg)
{
   uint32 timeNow;
   int diff, ticks;
   uint32 message[8];
   OS_Timer_t *timer;
   (void)arg;

   timeNow = OS_ThreadTime();
   for(;;)
   {
      //Determine how long to sleep
      OS_SemaphorePend(SemaphoreLock, OS_WAIT_FOREVER);
      if(TimerHead)
         ticks = TimerHead->ticksTimeout - timeNow;
      else
         ticks = OS_WAIT_FOREVER;
      OS_SemaphorePost(SemaphoreLock);
      OS_SemaphorePend(SemaphoreTimer, ticks);

      //Send messages for all timed out timers
      timeNow = OS_ThreadTime();
      for(;;)
      {
         timer = NULL;
         OS_SemaphorePend(SemaphoreLock, OS_WAIT_FOREVER);
         if(TimerHead)
         {
            diff = timeNow - TimerHead->ticksTimeout;
            if(diff >= 0)
               timer = TimerHead;
         }
         OS_SemaphorePost(SemaphoreLock);
         if(timer == NULL)
            break;
         if(timer->ticksRestart)
            OS_TimerStart(timer, timer->ticksRestart, timer->ticksRestart);
         else
            OS_TimerStop(timer);

         if(timer->callback)
            timer->callback(timer, timer->info);
         else
         {
            //Send message
            message[0] = MESSAGE_TYPE_TIMER;
            message[1] = (uint32)timer;
            message[2] = timer->info;
            OS_MQueueSend(timer->mqueue, message);
         }
      }
   }
}


/******************************************/
OS_Timer_t *OS_TimerCreate(const char *name, OS_MQueue_t *mQueue, uint32 info)
{
   OS_Timer_t *timer;

   OS_SemaphorePend(SemaphoreLock, OS_WAIT_FOREVER);
   if(SemaphoreTimer == NULL)
   {
      SemaphoreTimer = OS_SemaphoreCreate("Timer", 0);
      OS_ThreadCreate("Timer", OS_TimerThread, NULL, 250, 2000);
   }
   OS_SemaphorePost(SemaphoreLock);

   timer = (OS_Timer_t*)OS_HeapMalloc(HEAP_SYSTEM, sizeof(OS_Timer_t));
   if(timer == NULL)
      return NULL;
   timer->name = name;
   timer->callback = NULL;
   timer->mqueue = mQueue;
   timer->next = NULL;
   timer->prev = NULL;
   timer->info = info;
   timer->active = 0;
   return timer;
}


/******************************************/
void OS_TimerDelete(OS_Timer_t *timer)
{
   OS_TimerStop(timer);
   OS_HeapFree(timer);
}


/******************************************/
void OS_TimerCallback(OS_Timer_t *timer, OS_TimerFuncPtr_t callback)
{
   timer->callback = callback;
}


/******************************************/
//Must not be called from an ISR
void OS_TimerStart(OS_Timer_t *timer, uint32 ticks, uint32 ticksRestart)
{
   OS_Timer_t *node, *prev;
   int diff, check=0;

   assert(timer);
   assert(InterruptInside[OS_CpuIndex()] == 0);
   ticks += OS_ThreadTime();
   if(timer->active)
      OS_TimerStop(timer);
   OS_SemaphorePend(SemaphoreLock, OS_WAIT_FOREVER);
   if(timer->active)
   {
      //Prevent race condition
      OS_SemaphorePost(SemaphoreLock);
      return;
   }
   timer->ticksTimeout = ticks;
   timer->ticksRestart = ticksRestart;
   timer->active = 1;
   prev = NULL;
   for(node = TimerHead; node; node = node->next)
   {
      diff = ticks - node->ticksTimeout;
      if(diff <= 0)
         break;
      prev = node;
   }
   timer->next = node;
   timer->prev = prev;
   if(node)
      node->prev = timer;
   if(prev == NULL)
   {
      TimerHead = timer;
      check = 1;
   }
   else
      prev->next = timer;
   OS_SemaphorePost(SemaphoreLock);
   if(check)
      OS_SemaphorePost(SemaphoreTimer);
}


/******************************************/
//Must not be called from an ISR
void OS_TimerStop(OS_Timer_t *timer)
{
   assert(timer);
   assert(InterruptInside[OS_CpuIndex()] == 0);
   OS_SemaphorePend(SemaphoreLock, OS_WAIT_FOREVER);
   if(timer->active)
   {
      timer->active = 0;
      if(timer->prev == NULL)
         TimerHead = timer->next;
      else
         timer->prev->next = timer->next;
      if(timer->next)
         timer->next->prev = timer->prev;
   }
   OS_SemaphorePost(SemaphoreLock);
}


/***************** ISR ********************/
/******************************************/
void OS_InterruptServiceRoutine(uint32 status, uint32 *stack)
{
   int i;
   uint32 state, cpuIndex = OS_CpuIndex();

   if(status == 0 && Isr[31])
      Isr[31](stack);                   //SYSCALL or BREAK

   InterruptInside[cpuIndex] = 1;
   i = 0;
   do
   {   
      if(status & 1)
      {
         if(Isr[i])
            Isr[i](stack);
         else
            OS_InterruptMaskClear(1 << i);
      }
      status >>= 1;
      ++i;
   } while(status);
   InterruptInside[cpuIndex] = 0;

   state = OS_SpinLock();
   if(ThreadNeedReschedule[cpuIndex])
      OS_ThreadReschedule(ThreadNeedReschedule[cpuIndex] & 1);
   OS_SpinUnlock(state);
}


/******************************************/
void OS_InterruptRegister(uint32 mask, OS_FuncPtr_t funcPtr)
{
   int i;

   for(i = 0; i < 32; ++i)
   {
      if(mask & (1 << i))
         Isr[i] = funcPtr;
   }
}


/******************************************/
//Plasma hardware dependent
uint32 OS_InterruptStatus(void)
{
   return MemoryRead(IRQ_STATUS);
}


/******************************************/
//Plasma hardware dependent
uint32 OS_InterruptMaskSet(uint32 mask)
{
   uint32 state;
   state = OS_CriticalBegin();
   mask |= MemoryRead(IRQ_MASK);
   MemoryWrite(IRQ_MASK, mask);
   OS_CriticalEnd(state);
   return mask;
}


/******************************************/
//Plasma hardware dependent
uint32 OS_InterruptMaskClear(uint32 mask)
{
   uint32 state;
   state = OS_CriticalBegin();
   mask = MemoryRead(IRQ_MASK) & ~mask;
   MemoryWrite(IRQ_MASK, mask);
   OS_CriticalEnd(state);
   return mask;
}


/**************** Init ********************/
/******************************************/
static volatile uint32 IdleCount;
static void OS_IdleThread(void *arg)
{
   (void)arg;

   //Don't block in the idle thread!
   for(;;)
   {
      ++IdleCount;
   }
}


/******************************************/
#ifndef DISABLE_IRQ_SIM
static void OS_IdleSimulateIsr(void *arg)
{
   uint32 count=0, value;
   (void)arg;

   for(;;)
   {
      MemoryRead(IRQ_MASK + 4);       //calls Sleep(10)
#if WIN32
      while(OS_InterruptMaskSet(0) & IRQ_UART_WRITE_AVAILABLE)
         OS_InterruptServiceRoutine(IRQ_UART_WRITE_AVAILABLE, 0);
#endif
      value = OS_InterruptMaskSet(0) & 0xf;
      if(value)
         OS_InterruptServiceRoutine(value, 0);
      ++count;
   }
}
#endif //DISABLE_IRQ_SIM


/******************************************/
//Plasma hardware dependent
static void OS_ThreadTickToggle(void *arg)
{
   uint32 status, mask, state;

   //Toggle looking for IRQ_COUNTER18 or IRQ_COUNTER18_NOT
   state = OS_SpinLock();
   status = MemoryRead(IRQ_STATUS) & (IRQ_COUNTER18 | IRQ_COUNTER18_NOT);
   mask = MemoryRead(IRQ_MASK) | IRQ_COUNTER18 | IRQ_COUNTER18_NOT;
   mask &= ~status;
   MemoryWrite(IRQ_MASK, mask);
   OS_ThreadTick(arg);
   OS_SpinUnlock(state);
}


/******************************************/
void OS_Init(uint32 *heapStorage, uint32 bytes)
{
   int i;
   OS_AsmInterruptInit();               //Patch interrupt vector
   OS_InterruptMaskClear(0xffffffff);   //Disable interrupts
   HeapArray[0] = OS_HeapCreate("Default", heapStorage, bytes);
   HeapArray[1] = HeapArray[0];
   SemaphoreSleep = OS_SemaphoreCreate("Sleep", 0);
   SemaphoreRelease = OS_SemaphoreCreate("Release", 1);
   SemaphoreLock = OS_SemaphoreCreate("Lock", 1);
   for(i = 0; i < OS_CPU_COUNT; ++i)
      OS_ThreadCreate("Idle", OS_IdleThread, NULL, 0, 256);
#ifndef DISABLE_IRQ_SIM
   if((OS_InterruptStatus() & (IRQ_COUNTER18 | IRQ_COUNTER18_NOT)) == 0)
   {
      //Detected that running in simulator so create SimIsr thread
      UartPrintfCritical("SimIsr\n");
      OS_ThreadCreate("SimIsr", OS_IdleSimulateIsr, NULL, 1, 0);
   }
#endif //DISABLE_IRQ_SIM
   //Plasma hardware dependent
   OS_InterruptRegister(IRQ_COUNTER18 | IRQ_COUNTER18_NOT, OS_ThreadTickToggle);
   OS_InterruptMaskSet(IRQ_COUNTER18 | IRQ_COUNTER18_NOT);
}


/******************************************/
void OS_Start(void)
{
   ThreadSwapEnabled = 1;
   (void)OS_SpinLock();
   OS_ThreadReschedule(1);
}


/******************************************/
//Place breakpoint here
void OS_Assert(void)
{
}


#if OS_CPU_COUNT > 1
static uint8 SpinLockArray[OS_CPU_COUNT];
/******************************************/
uint32 OS_CpuIndex(void)
{
   return 0; //0 to OS_CPU_COUNT-1
}


/******************************************/
//Symmetric Multiprocessing Spin Lock Mutex
uint32 OS_SpinLock(void)
{
   uint32 state, cpuIndex, i, j, ok, delay;

   cpuIndex = OS_CpuIndex();
   delay = cpuIndex + 8;
   state = OS_AsmInterruptEnable(0);
   do
   {
      ok = 1;
      if(++SpinLockArray[cpuIndex] == 1)
      {
         for(i = 0; i < OS_CPU_COUNT; ++i)
         {
            if(i != cpuIndex && SpinLockArray[i])
               ok = 0;
         }
         if(ok == 0)
         {
            SpinLockArray[cpuIndex] = 0;
            for(j = 0; j < delay; ++j)  //wait a bit
               ++i;
            if(delay < 128)
               delay <<= 1;
         }
      }
   } while(ok == 0);
   return state;
}


/******************************************/
void OS_SpinUnlock(uint32 state)
{
   uint32 cpuIndex;
   cpuIndex = OS_CpuIndex();
   if(--SpinLockArray[cpuIndex] == 0)
      OS_AsmInterruptEnable(state);

   assert(SpinLockArray[cpuIndex] < 10);
}
#endif  //OS_CPU_COUNT > 1


/************** WIN32/Linux Support *************/
#ifdef WIN32
#ifdef LINUX
#define putch putchar
#undef _LIBC
#undef kbhit
#undef getch
#define UartPrintf UartPrintf2
#define UartScanf UartScanf2
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
void Sleep(unsigned int value)
{ 
   usleep(value * 1000);
}

int kbhit(void)
{
   struct termios oldt, newt;
   struct timeval tv;
   fd_set read_fd;

   tcgetattr(STDIN_FILENO, &oldt);
   newt = oldt;
   newt.c_lflag &= ~(ICANON | ECHO);
   tcsetattr(STDIN_FILENO, TCSANOW, &newt);
   tv.tv_sec=0;
   tv.tv_usec=0;
   FD_ZERO(&read_fd);
   FD_SET(0,&read_fd);
   if(select(1, &read_fd, NULL, NULL, &tv) == -1)
      return 0;
   if(FD_ISSET(0,&read_fd))
      return 1;
   return 0;
}

int getch(void)
{
   struct termios oldt, newt;
   int ch;

   tcgetattr(STDIN_FILENO, &oldt);
   newt = oldt;
   newt.c_lflag &= ~(ICANON | ECHO);
   tcsetattr(STDIN_FILENO, TCSANOW, &newt);
   ch = getchar();
   return ch;
}
#else
//Support RTOS inside Windows
#undef kbhit
#undef getch
#undef putch
extern int kbhit();
extern int getch(void);
extern int putch(int);
extern void __stdcall Sleep(unsigned long value);
#endif

static uint32 Memory[8];

uint32 MemoryRead(uint32 address)
{
   Memory[2] |= IRQ_UART_WRITE_AVAILABLE;    //IRQ_STATUS
   switch(address)
   {
   case UART_READ: 
      if(kbhit())
         Memory[0] = getch();                //UART_READ
      Memory[2] &= ~IRQ_UART_READ_AVAILABLE; //clear bit
      return Memory[0];
   case IRQ_MASK: 
      return Memory[1];                      //IRQ_MASK
   case IRQ_MASK + 4:
      Sleep(10);
      return 0;
   case IRQ_STATUS: 
      if(kbhit())
         Memory[2] |= IRQ_UART_READ_AVAILABLE;
      return Memory[2];
   }
   return 0;
}

void MemoryWrite(uint32 address, uint32 value)
{
   switch(address)
   {
   case UART_WRITE: 
      putch(value); 
      break;
   case IRQ_MASK:   
      Memory[1] = value; 
      break;
   case IRQ_STATUS: 
      Memory[2] = value; 
      break;
   }
}

uint32 OS_AsmInterruptEnable(uint32 enableInterrupt)
{
   return enableInterrupt;
}

void OS_AsmInterruptInit(void)
{
}
#endif  //WIN32


/**************** Example *****************/
#ifndef NO_MAIN
#ifdef WIN32
static uint8 HeapSpace[1024*512];
#endif

int main(int programEnd, char *argv[])
{
   (void)programEnd;  //Pointer to end of used memory
   (void)argv;

   UartPrintfCritical("Starting RTOS\n");
#ifdef WIN32
   OS_Init((uint32*)HeapSpace, sizeof(HeapSpace));
#else
   //Remaining space after program in 1MB external RAM
   OS_Init((uint32*)programEnd, 
           RAM_EXTERNAL_BASE + RAM_EXTERNAL_SIZE - programEnd); 
#endif
   UartInit();
   OS_ThreadCreate("Main", MainThread, NULL, 100, 4000);
   OS_Start();
   return 0;
}
#endif  //NO_MAIN

