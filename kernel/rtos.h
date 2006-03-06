/*--------------------------------------------------------------------
 * TITLE: Plasma Real Time Operating System
 * AUTHOR: Steve Rhoads (rhoadss@yahoo.com)
 * DATE CREATED: 12/17/05
 * FILENAME: rtos.h
 * PROJECT: Plasma CPU core
 * COPYRIGHT: Software placed into the public domain by the author.
 *    Software 'as is' without warranty.  Author liable for nothing.
 * DESCRIPTION:
 *    Plasma Real Time Operating System
 *--------------------------------------------------------------------*/
#ifndef __RTOS_H__
#define __RTOS_H__

//#define printf     UartPrintf
#define printf     UartPrintfPoll
#define scanf      UartScanf

// Typedefs
typedef unsigned int   uint32;
typedef unsigned short uint16;
typedef unsigned char  uint8;

// Memory Access
#ifdef WIN32
uint32 MemoryRead(uint32 Address);
void MemoryWrite(uint32 Address, uint32 Value);
#else
#define MemoryRead(A) (*(volatile uint32*)(A))
#define MemoryWrite(A,V) *(volatile uint32*)(A)=(V)
#endif

/***************** LibC ******************/
#ifndef NULL
#define NULL (void*)0
#endif

#define assert(A) if((A)==0){OS_Assert();UartPrintfCritical("\r\nAssert %s:%d\r\n", __FILE__, __LINE__);}

#define isprint(c) (' '<=(c)&&(c)<='~')
#define isspace(c) ((c)==' '||(c)=='\t'||(c)=='\n'||(c)=='\r')
#define isdigit(c) ('0'<=(c)&&(c)<='9')
#define islower(c) ('a'<=(c)&&(c)<='z')
#define isupper(c) ('A'<=(c)&&(c)<='Z')
#define isalpha(c) (islower(c)||isupper(c))
#define isalnum(c) (isalpha(c)||isdigit(c))
#define min(a,b)   ((a)<(b)?(a):(b))

char *strcpy(char *dst, const char *src);
char *strncpy(char *dst, const char *src, int count);
char *strcat(char *dst, const char *src);
char *strncat(char *dst, const char *src, int count);
int strcmp(const char *string1, const char *string2);
int strncmp(const char *string1, const char *string2, int count);
char *strstr(char *string, char *find);
int strlen(const char *string);
void *memcpy(void *dst, const void *src, unsigned long bytes);
int memcmp(const void *cs, const void *ct, unsigned long bytes);
void *memset(void *dst, int c, unsigned long bytes);
int abs(int n);
int rand(void);
void srand(unsigned int seed);
long strtol(const char *s, const char **end, int base);
int atoi(const char *s);
void itoa(char *dst, int num, int base, int width);
#ifndef NO_ELLIPSIS
int sprintf(char *s, const char *format, ...);
int sscanf(char *s, const char *format, ...);
#endif

/***************** Assembly **************/
#define OS_CriticalBegin() OS_AsmInterruptEnable(0)
#define OS_CriticalEnd(S) OS_AsmInterruptEnable(S)
typedef uint32 jmp_buf[20];
extern uint32 OS_AsmInterruptEnable(uint32 state);
extern void OS_AsmInterruptInit(void);
extern int setjmp(jmp_buf env);
extern void longjmp(jmp_buf env, int val);
extern uint32 OS_AsmMult(uint32 a, uint32 b, unsigned long *hi);

/***************** Heap ******************/
#define HEAP_SYSTEM  (void*)0
#define HEAP_GENERAL (void*)1
#define HEAP_SMALL   (void*)2
#define HEAP_UI      (void*)3
typedef struct OS_Heap_s OS_Heap_t;
OS_Heap_t *OS_HeapCreate(const char *Name, void *Memory, uint32 Size);
void OS_HeapDestroy(OS_Heap_t *Heap);
void *OS_HeapMalloc(OS_Heap_t *Heap, int Bytes);
void OS_HeapFree(void *Block);
void OS_HeapAlternate(OS_Heap_t *Heap, OS_Heap_t *Alternate);
void OS_HeapRegister(void *Index, OS_Heap_t *Heap);

/***************** Thread *****************/
#ifdef WIN32
#define STACK_SIZE_MINIMUM (1024*4)
#else
#define STACK_SIZE_MINIMUM (1024*1)
#endif
#define STACK_SIZE_DEFAULT 1024*2
#define THREAD_PRIORITY_IDLE 0
#define THREAD_PRIORITY_MAX 255
typedef void (*OS_FuncPtr_t)(void *Arg);
typedef struct OS_Thread_s OS_Thread_t;
OS_Thread_t *OS_ThreadCreate(const char *Name,
                             OS_FuncPtr_t FuncPtr, 
                             void *Arg, 
                             uint32 Priority, 
                             uint32 StackSize);
void OS_ThreadExit(void);
OS_Thread_t *OS_ThreadSelf(void);
void OS_ThreadSleep(int Ticks);
uint32 OS_ThreadTime(void);
void OS_ThreadInfoSet(OS_Thread_t *Thread, void *Info);
void *OS_ThreadInfoGet(OS_Thread_t *Thread);
uint32 OS_ThreadPriorityGet(OS_Thread_t *Thread);
void OS_ThreadPrioritySet(OS_Thread_t *Thread, uint32 Priority);
void OS_ThreadTick(void *Arg);

/***************** Semaphore **************/
#define OS_SUCCESS 0
#define OS_ERROR  -1
#define OS_WAIT_FOREVER -1
#define OS_NO_WAIT 0
typedef struct OS_Semaphore_s OS_Semaphore_t;
OS_Semaphore_t *OS_SemaphoreCreate(const char *Name, uint32 Count);
void OS_SemaphoreDelete(OS_Semaphore_t *Semaphore);
int OS_SemaphorePend(OS_Semaphore_t *Semaphore, int Ticks); //tick ~= 10ms
void OS_SemaphorePost(OS_Semaphore_t *Semaphore);

/***************** Mutex ******************/
typedef struct OS_Mutex_s OS_Mutex_t;
OS_Mutex_t *OS_MutexCreate(const char *Name);
void OS_MutexDelete(OS_Mutex_t *Semaphore);
void OS_MutexPend(OS_Mutex_t *Semaphore);
void OS_MutexPost(OS_Mutex_t *Semaphore);

/***************** MQueue *****************/
enum {
   MESSAGE_TYPE_USER = 0,
   MESSAGE_TYPE_TIMER = 5
};
typedef struct OS_MQueue_s OS_MQueue_t;
OS_MQueue_t *OS_MQueueCreate(const char *Name,
                             int MessageCount,
                             int MessageBytes);
void OS_MQueueDelete(OS_MQueue_t *MQueue);
int OS_MQueueSend(OS_MQueue_t *MQueue, void *Message);
int OS_MQueueGet(OS_MQueue_t *MQueue, void *Message, int Ticks);

/***************** Timer ******************/
typedef struct OS_Timer_s OS_Timer_t;
OS_Timer_t *OS_TimerCreate(const char *Name, OS_MQueue_t *MQueue, uint32 Info);
void OS_TimerDelete(OS_Timer_t *Timer);
void OS_TimerStart(OS_Timer_t *Timer, uint32 Ticks, uint32 TicksRestart);
void OS_TimerStop(OS_Timer_t *Timer);

/***************** ISR ********************/
void OS_InterruptServiceRoutine(uint32 Status);
void OS_InterruptRegister(uint32 Mask, OS_FuncPtr_t FuncPtr);
uint32 OS_InterruptStatus(void);
uint32 OS_InterruptMaskSet(uint32 Mask);
uint32 OS_InterruptMaskClear(uint32 Mask);

/***************** Init ******************/
void OS_Init(uint32 *HeapStorage, uint32 Bytes);
void OS_Start(void);
void OS_Assert(void);
void MainThread(void *Arg);

/***************** UART ******************/
void UartInit(void);
void UartWrite(int C);
uint8 UartRead(void);
void UartWriteData(uint8 *Data, int Length);
void UartReadData(uint8 *Data, int Length);
#ifndef NO_ELLIPSIS2
void UartPrintf(const char *format, ...);
void UartPrintfPoll(const char *format, ...);
void UartPrintfCritical(const char *format, ...);
void UartScanf(const char *format, ...);
#endif
int puts(const char *string);
int getch(void);
int kbhit(void);
void LogWrite(int a);
void LogDump(void);

/***************** Math ******************/
//IEEE single precision floating point math
#ifndef WIN32
#define FP_Neg     __negsf2
#define FP_Add     __addsf3
#define FP_Sub     __subsf3
#define FP_Mult    __mulsf3
#define FP_Div     __divsf3
#define FP_ToLong  __fixsfsi
#define FP_ToFloat __floatsisf
#define sqrt FP_Sqrt
#define cos  FP_Cos
#define sin  FP_Sin
#define atan FP_Atan
#define log  FP_Log
#define exp  FP_Exp
#endif
float FP_Neg(float a_fp);
float FP_Add(float a_fp, float b_fp);
float FP_Sub(float a_fp, float b_fp);
float FP_Mult(float a_fp, float b_fp);
float FP_Div(float a_fp, float b_fp);
long  FP_ToLong(float a_fp);
float FP_ToFloat(long af);
int   FP_Cmp(float a_fp, float b_fp);
float FP_Sqrt(float a);
float FP_Cos(float rad);
float FP_Sin(float rad);
float FP_Atan(float x);
float FP_Atan2(float y, float x);
float FP_Exp(float x);
float FP_Log(float x);
float FP_Pow(float x, float y);

#endif //__PLASMA_H__

