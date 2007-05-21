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
   } jmp_buf 2;
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
   THREAD_PEND    = 0,
   THREAD_READY   = 1,
   THREAD_RUNNING = 2
} OS_ThreadState_e;

struct OS_Thread_s {
   const char *name;
   OS_ThreadState_e state;
   int cpuIndex;
   int cpuLock;
   jmp_buf env;
   OS_FuncPtr_t funcPtr;
   void *arg;
   uint32 priority;
   uint32 ticksTimeout;
   void *info;
   OS_Semaphore_t *semaphorePending;
   int returnCode;
   uint32 spinLocks;
   uint32 processId;
   OS_Heap_t *heap;
   struct OS_Thread_s *next, *prev;
   struct OS_Thread_s *nextTimeout, *prevTimeout;
   uint32 magic[1];
};
//typedef struct OS_Thread_s OS_Thread_t;

struct OS_Semaphore_s {
   const char *name;
   struct OS_Thread_s *threadHead;
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
static OS_Thread_t *ThreadCurrent[OS_CPU_COUNT];
static OS_Thread_t *ThreadHead;   //Linked list of threads sorted by priority
static OS_Thread_t *TimeoutHead;  //Linked list of threads sorted by timeout
static int ThreadSwapEnabled;
static int ThreadNeedReschedule;
static uint32 ThreadTime;
static void *NeedToFree;
static OS_Semaphore_t SemaphoreReserved[SEM_RESERVED_COUNT];
static OS_Semaphore_t *SemaphoreSleep;
static OS_Semaphore_t *SemaphoreLock;
static OS_Semaphore_t *SemaphoreTimer;
static OS_Timer_t *TimerHead;     //Linked list of timers sorted by timeout
static OS_FuncPtr_t Isr[32];
static int InterruptInside;
int InitStack[128];               //Used by boot.asm


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
//Must be called with interrupts disabled
static void OS_ThreadPriorityInsert(OS_Thread_t **head, OS_Thread_t *thread)
{
   OS_Thread_t *node, *prev;

   prev = NULL;
   for(node = *head; node; node = node->next)
   {
      if(node->priority <= thread->priority)
         break;
      prev = node;
   }

   if(prev == NULL)
   {
      thread->next = *head;
      thread->prev = NULL;
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
   if(*head == ThreadHead)
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
   thread->state = THREAD_PEND;
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


#if OS_CPU_COUNT <= 1
/******************************************/
//Loads a new thread 
//Must be called with interrupts disabled
static void OS_ThreadReschedule(int roundRobin)
{
   OS_Thread_t *threadNext, *threadCurrent, *threadTry;
   int rc;

   if(ThreadSwapEnabled == 0 || InterruptInside)
   {
      ThreadNeedReschedule |= 2 + roundRobin;  //Reschedule later
      return;
   }

   //Determine which thread should run
   threadCurrent = ThreadCurrent[0];
   threadNext = threadCurrent;
   if(threadCurrent == NULL || threadCurrent->state == THREAD_PEND)
      threadNext = ThreadHead;
   else if(threadCurrent->priority < ThreadHead->priority)
      threadNext = ThreadHead;
   else if(roundRobin)
   {
      //Determine next ready thread with same priority
      threadTry = threadCurrent->next;
      if(threadTry && threadTry->priority == threadCurrent->priority)
         threadNext = threadTry;
      else
         threadNext = ThreadHead;
   }

   if(threadNext != threadCurrent)
   {
      //Swap threads
      ThreadCurrent[0] = threadNext;
      assert(threadNext);
      if(threadCurrent)
      {
         assert(threadCurrent->magic[0] == THREAD_MAGIC); //check stack overflow
         rc = setjmp(threadCurrent->env);  //ANSI C call to save registers
         if(rc)
         {
            //Returned from longjmp()
            return;
         }
      }

      threadNext = ThreadCurrent[0];       //removed warning
      longjmp(threadNext->env, 1);         //ANSI C call to restore registers
   }
}

#else //OS_CPU_COUNT > 1

/******************************************/
//Check if a different CPU needs to swap threads
static void OS_ThreadRescheduleCheck(void)
{
   OS_Thread_t *threadBest;
   uint32 i, priorityLow, cpuIndex = OS_CpuIndex();
   int cpuLow;

   //Find the CPU running the lowest priority thread
   cpuLow = 0;
   priorityLow = 0xffffffff;
   for(i = 0; i < OS_CPU_COUNT; ++i)
   {
      if(i != cpuIndex && (ThreadCurrent[i] == NULL || 
         ThreadCurrent[i]->priority < priorityLow))
      {
         cpuLow = i;
         if(ThreadCurrent[i])
            priorityLow = ThreadCurrent[i]->priority;
         else
            priorityLow = 0;
      }
   }

   //Determine highest priority ready thread for other CPUs
   for(threadBest = ThreadHead; threadBest && threadBest->priority > priorityLow; 
      threadBest = threadBest->next)
   {
      if(threadBest->state == THREAD_READY)
      { 
         if(threadBest->cpuLock == -1)
         {
            OS_CpuInterrupt(cpuLow, 1);  //Reschedule on the other CPU
            break;
         }
         else if(threadBest->priority > ThreadCurrent[threadBest->cpuLock]->priority)
         {
            OS_CpuInterrupt(threadBest->cpuLock, 1);  //Reschedule on the other CPU
            break;
         }
      }
   }
}


/******************************************/
//Loads a new thread in a multiprocessor environment
//Must be called with interrupts disabled
static void OS_ThreadReschedule(int roundRobin)
{
   OS_Thread_t *threadNext, *threadCurrent, *threadBest, *threadAlt;
   uint32 cpuIndex = OS_CpuIndex();
   int rc;

   if(ThreadSwapEnabled == 0 || InterruptInside)
   {
      ThreadNeedReschedule |= 2 + roundRobin;  //Reschedule later
      return;
   }

   //Determine highest priority ready thread
   for(threadBest = ThreadHead; threadBest; threadBest = threadBest->next)
   {
      if(threadBest->state == THREAD_READY && 
         (threadBest->cpuLock == -1 || threadBest->cpuLock == (int)cpuIndex))
         break;
   }

   //Determine which thread should run
   threadCurrent = ThreadCurrent[cpuIndex];
   threadNext = threadCurrent;
   if(threadCurrent == NULL || threadCurrent->state == THREAD_PEND)
   {
      threadNext = threadBest;
   }
   else if(threadBest && threadCurrent->priority < threadBest->priority)
   {
      threadNext = threadBest;
   }
   else if(roundRobin)
   {
      //Find the next ready thread
      for(threadAlt = threadCurrent->next; threadAlt; threadAlt = threadAlt->next)
      {
         if(threadAlt->state == THREAD_READY &&
            (threadAlt->cpuLock == -1 || threadAlt->cpuLock == (int)cpuIndex))
            break;
      }
      if(threadAlt && threadAlt->priority == threadCurrent->priority)
         threadNext = threadAlt;
      else if(threadBest && threadBest->priority >= threadCurrent->priority)
         threadNext = threadBest;
   }

   if(threadNext != threadCurrent)
   {
      //Swap threads
      ThreadCurrent[cpuIndex] = threadNext;
      assert(threadNext);
      if(threadCurrent)
      {
         assert(threadCurrent->magic[0] == THREAD_MAGIC); //check stack overflow
         threadCurrent->state = THREAD_READY;
         threadCurrent->spinLocks = OS_SpinCountGet();
         threadCurrent->cpuIndex = -1;
         rc = setjmp(threadCurrent->env);   //ANSI C call to save registers
         if(rc)
         {
            //Returned from longjmp()
            return;
         }
      }

      //Restore spin lock count
      cpuIndex = OS_CpuIndex();             //removed warning
      threadNext = ThreadCurrent[cpuIndex]; //removed warning
      threadNext->state = THREAD_RUNNING;
      OS_SpinCountSet(threadNext->spinLocks);
      threadNext->cpuIndex = (int)cpuIndex;
      OS_ThreadRescheduleCheck();
      longjmp(threadNext->env, 1);          //ANSI C call to restore registers
   }
   OS_ThreadRescheduleCheck();
}


/******************************************/
void OS_ThreadCpuLock(OS_Thread_t *Thread, int CpuIndex)
{
   Thread->cpuLock = CpuIndex;
   if(Thread == OS_ThreadSelf() && CpuIndex != (int)OS_CpuIndex())
      OS_ThreadSleep(1);
}

#endif //#if OS_CPU_COUNT <= 1


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

   OS_SemaphorePend(SemaphoreLock, OS_WAIT_FOREVER);
   if(NeedToFree)
      OS_HeapFree(NeedToFree);
   NeedToFree = NULL;
   OS_SemaphorePost(SemaphoreLock);

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
   thread->info = NULL;
   thread->semaphorePending = NULL;
   thread->returnCode = 0;
   thread->spinLocks = 1;
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
   OS_SemaphorePend(SemaphoreLock, OS_WAIT_FOREVER);
   if(NeedToFree)
      OS_HeapFree(NeedToFree);
   NeedToFree = NULL;
   OS_SemaphorePost(SemaphoreLock);

   state = OS_CriticalBegin();
   OS_ThreadPriorityRemove(&ThreadHead, ThreadCurrent[cpuIndex]);
   NeedToFree = ThreadCurrent[cpuIndex];
   OS_ThreadReschedule(0);
   OS_CriticalEnd(state);
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
void OS_ThreadInfoSet(OS_Thread_t *thread, void *Info)
{
   thread->info = Info;
}


/******************************************/
void *OS_ThreadInfoGet(OS_Thread_t *thread)
{
   return thread->info;
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
   if(thread->state != THREAD_PEND)
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
   assert(InterruptInside == 0);
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
      OS_ThreadPriorityRemove(&ThreadHead, thread);
      OS_ThreadPriorityInsert(&semaphore->threadHead, thread);
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

   if(jobThread == NULL)
   {
      jobQueue = OS_MQueueCreate("job", 100, 16);
      jobThread = OS_ThreadCreate("job", JobThread, NULL, 150, 4000);
   }
   message[0] = (uint32)funcPtr;
   message[1] = (uint32)arg0;
   message[2] = (uint32)arg1;
   message[3] = (uint32)arg2;
   do
   {
      rc = OS_MQueueSend(jobQueue, message);
      if(rc)
         OS_ThreadSleep(1);
   } while(rc);
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
   int startThread=0;

   OS_SemaphorePend(SemaphoreLock, OS_WAIT_FOREVER);
   if(SemaphoreTimer == NULL)
   {
      SemaphoreTimer = OS_SemaphoreCreate("Timer", 0);
      startThread = 1;
   }
   OS_SemaphorePost(SemaphoreLock);
   if(startThread)
      OS_ThreadCreate("Timer", OS_TimerThread, NULL, 250, 2000);

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
   assert(InterruptInside == 0);
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
   assert(InterruptInside == 0);
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
   uint32 state;

   if(status == 0 && Isr[31])
      Isr[31](stack);                   //SYSCALL or BREAK

   InterruptInside = 1;
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
   InterruptInside = 0;

   state = OS_SpinLock();
   if(ThreadNeedReschedule)
      OS_ThreadReschedule(ThreadNeedReschedule & 1);
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


/******************************************/
//Must be called with interrupts disabled and spin locked
uint32 OS_SpinCountGet(void)
{
   uint32 cpuIndex, count;
   cpuIndex = OS_CpuIndex();
   count = SpinLockArray[cpuIndex];
   return count;
}


/******************************************/
//Must be called with interrupts disabled and spin locked
void OS_SpinCountSet(uint32 count)
{
   uint32 cpuIndex;
   cpuIndex = OS_CpuIndex();
   SpinLockArray[cpuIndex] = (uint8)count;
   assert(count);
}


/******************************************/
void OS_CpuInterrupt(uint32 cpuIndex, uint32 bitfield)
{
   //Request other CPU to reschedule threads
   (void)cpuIndex;
   (void)bitfield;
}


/******************************************/
void OS_CpuInterruptServiceRoutine(void *arg)
{
   uint32 state;
   (void)arg;
   state = OS_SpinLock();
   OS_ThreadReschedule(0);
   OS_SpinUnlock(state);
}
#endif


/************** WIN32 Support *************/
#ifdef WIN32
//Support RTOS inside Windows
extern int kbhit();
extern int getch(void);
extern int putch(int);
extern void __stdcall Sleep(unsigned long value);

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
#endif

