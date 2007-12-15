/* bootldr.c */
#include "plasma.h"

#define MemoryRead(A) (*(volatile unsigned long*)(A))
#define MemoryWrite(A,V) *(volatile unsigned long*)(A)=(V)

extern void putchar(int ch);
extern int puts(const char *string);
extern int getch(void);
extern int kbhit(void);
extern int DdrInit(void);

typedef void (*FuncPtr)(void);

char *xtoa(unsigned long num)
{
   static char buf[12];
   int i, digit;
   buf[8] = 0;
   for (i = 7; i >= 0; --i)
   {
      digit = num & 0xf;
      buf[i] = digit + (digit < 10 ? '0' : 'A' - 10);
      num >>= 4;
   }
   return buf;
}

unsigned long getnum(void)
{
   int i;
   unsigned long ch, ch2, value=0;
   for(i = 0; i < 16; )
   {
      ch = ch2 = getch();
      if(ch == '\n' || ch == '\r')
         break;
      if('0' <= ch && ch <= '9')
         ch -= '0';
      else if('A' <= ch && ch <= 'Z')
         ch = ch - 'A' + 10;
      else if('a' <= ch && ch <= 'z')
         ch = ch - 'a' + 10;
      else if(ch == 8)
      {
         if(i > 0)
         {
            --i;
            putchar(ch);
            putchar(' ');
            putchar(ch);
         }
         value >>= 4;
         continue;
      }
      putchar(ch2);
      value = (value << 4) + ch;
      ++i;
   }
   putchar('\r');
   putchar('\n');
   return value;
}

int main(void)
{
   int i, j, ch;
   unsigned long address, value, count;
   FuncPtr funcPtr;
   unsigned char *ptr1;

   DdrInit();  //Harmless if SDRAM instead of DDR

   puts("\nGreetings from the bootloader ");
   puts(__DATE__);
   puts(" ");
   puts(__TIME__);
   puts(":\n");
   for(;;)
   {
      puts("\nWaiting for binary image linked at 0x10000000\n");
      puts("Other Menu Options:\n");
      puts("1. Memory read word\n");
      puts("2. Memory write word\n");
      puts("3. Memory read byte\n");
      puts("4. Memory write byte\n");
      puts("5. Jump to address\n");
      puts("6. Raw memory read\n");
      puts("7. Raw memory write\n");
      puts("8. Checksum\n");
      puts("9. Dump\n");
      puts("> ");
      ch = getch();
      address = 0;
      if('0' <= ch && ch <= '9')
      {
         putchar(ch);
         puts("\nAddress in hex> ");
         address = getnum();
         puts("Address = ");
         puts(xtoa(address));
         puts("\n");
      }
      switch(ch)
      {
      case '1':
         value = MemoryRead(address);
         puts(xtoa(value));
         puts("\n");
         break;
      case '2':
         puts("\nValue in hex> ");
         value = getnum();
         puts(xtoa(value));
         MemoryWrite(address, value);
         break;
      case '3':
         value = *(unsigned char*)address;
         puts(xtoa(value));
         puts("\n");
         break;
      case '4':
         puts("\nValue in hex> ");
         value = getnum();
         puts(xtoa(value));
         *(unsigned char*)address = value;
         break;
      case '5':
         funcPtr = (FuncPtr)address;
         funcPtr();
         break;
      case '6':
         puts("\nCount in hex> ");
         count = getnum();
         //puts(xtoa(count));
         //puts("\n");
         for(i = 0; i < count; ++i)
         {
            ch = *(unsigned char*)(address + i);
            putchar(ch);
         }
         break;
      case '7':
         puts("\nCount in hex> ");
         count = getnum();
         //puts(xtoa(count));
         //putchar('\n');
         for(i = 0; i < count; ++i)
         {
            ch = getch();
            *(unsigned char*)(address+i) = ch;
         }
         break;
      case '8':
         puts("\nCount in hex> ");
         count = getnum();
         value = 0;
         for(i = 0; i < count; ++i)
         {
            value += *(unsigned char*)(address+i);
         }
         puts(xtoa(value));
         putchar('\n');
         break;
      case '9':
         puts("\nCount in hex> ");
         count = getnum();
         value = 0;
         for(i = 0; i < count; i += 4)
         {
            if((i & 15) == 0)
               puts("\r\n");
            value = *(unsigned long*)(address+i);
            puts(xtoa(value));
            putchar(' ');
         }
         puts("\r\n");
         break;
      case 0x3c:   //raw test.bin file
         ptr1 = (unsigned char*)0x10000000;
         for(i = 0; i < 1024*1024; ++i)
         {
            ptr1[i] = (unsigned char)ch;
            for(j = 0; j < 10000; ++j)
            {
               if(kbhit())
                  break;
            }
            if(j >= 10000)
               break;       //assume end of file
            ch = getch();
         }
         funcPtr = (FuncPtr)0x10000000;
         funcPtr();
         break;
      }
   }
   return 0;
}

