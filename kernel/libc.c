/*--------------------------------------------------------------------
 * TITLE: ANSI C Library
 * AUTHOR: Steve Rhoads (rhoadss@yahoo.com)
 * DATE CREATED: 12/17/05
 * FILENAME: clib.c
 * PROJECT: Plasma CPU core
 * COPYRIGHT: Software placed into the public domain by the author.
 *    Software 'as is' without warranty.  Author liable for nothing.
 * DESCRIPTION:
 *    Subset of the ANSI C library
 *--------------------------------------------------------------------*/
#define NO_ELLIPSIS
#include "plasma.h"
#include "rtos.h"


char *strcpy(char *dst, const char *src)
{
   int c;
   do
   {
      c = *dst++ = *src++;
   } while(c);
   return dst;
}


char *strncpy(char *dst, const char *src, int count)
{
   int c=1;
   while(count-- > 0 && c)
      c = *dst++ = *src++;
   *dst = 0;
   return dst;
}


char *strcat(char *dst, const char *src)
{
   int c;
   while(*dst)
      ++dst;
   do
   {
      c = *dst++ = *src++;
   } while(c);
   return dst;
}


char *strncat(char *dst, const char *src, int count)
{
   int c=1;
   while(*dst && --count > 0)
      ++dst;
   while(--count > 0 && c)
      c = *dst++ = *src++;
   *dst = 0;
   return dst;
}


int strcmp(const char *string1, const char *string2)
{
   int diff, c;
   for(;;)
   {
      diff = *string1++ - (c = *string2++);
      if(diff)
         return diff;
      if(c == 0)
         return 0;
   }
}


int strncmp(const char *string1, const char *string2, int count)
{
   int diff, c;
   while(count-- > 0)
   {
      diff = *string1++ - (c = *string2++);
      if(diff)
         return diff;
      if(c == 0)
         return 0;
   }
   return 0;
}


char *strstr(const char *string, const char *find)
{
   int i;
   for(;;)
   {
      for(i = 0; string[i] == find[i] && find[i]; ++i) ;
      if(find[i] == 0)
         return (char*)string;
      if(*string++ == 0)
         return NULL;
   }
}


int strlen(const char *string)
{
   const char *base=string;
   while(*string++) ;
   return string - base - 1;
}


void *memcpy(void *dst, const void *src, unsigned long bytes)
{
   uint8 *Dst = (uint8*)dst;
   uint8 *Src = (uint8*)src;
   while((int)bytes-- > 0)
      *Dst++ = *Src++;
   return dst;
}


void *memmove(void *dst, const void *src, unsigned long bytes)
{
   uint8 *Dst = (uint8*)dst;
   uint8 *Src = (uint8*)src;
   if(Dst < Src)
   {
      while((int)bytes-- > 0)
         *Dst++ = *Src++;
   }
   else
   {
      Dst += bytes;
      Src += bytes;
      while((int)bytes-- > 0)
         *--Dst = *--Src;
   }
   return dst;
}


int memcmp(const void *cs, const void *ct, unsigned long bytes)
{
   uint8 *Dst = (uint8*)cs;
   uint8 *Src = (uint8*)ct;
   int diff;
   while((int)bytes-- > 0)
   {
      diff = *Dst++ - *Src++;
      if(diff)
         return diff;
   }
   return 0;
}


void *memset(void *dst, int c, unsigned long bytes)
{
   uint8 *Dst = (uint8*)dst;
   while((int)bytes-- > 0)
      *Dst++ = (uint8)c;
   return dst;
}


int abs(int n)
{
   return n>=0 ? n : -n;
}


static uint32 Rand1=0x1f2bcda3, Rand2=0xdeafbeef, Rand3=0xc5134306;
int rand(void)
{
   int shift;
   Rand1 += 0x13423123 + Rand2;
   Rand2 += 0x2312fdea + Rand3;
   Rand3 += 0xf2a12de1;
   shift = Rand3 & 31;
   Rand1 = (Rand1 << (32 - shift)) | (Rand1 >> shift);
   Rand3 ^= Rand1;
   shift = (Rand3 >> 8) & 31;
   Rand2 = (Rand2 << (32 - shift)) | (Rand2 >> shift);
   return Rand1;
}


void srand(unsigned int seed)
{
   Rand1 = seed;
}


long strtol(const char *s, const char **end, int base)
{
   int i;
   unsigned long ch, value=0, neg=0;

   if(s[0] == '-')
   {
      neg = 1;
      ++s;
   }
   if(s[0] == '0' && s[1] == 'x')
   {
      base = 16;
      s += 2;
   }
   for(i = 0; i <= 8; ++i)
   {
      ch = *s++;
      if('0' <= ch && ch <= '9')
         ch -= '0';
      else if('A' <= ch && ch <= 'Z')
         ch = ch - 'A' + 10;
      else if('a' <= ch && ch <= 'z')
         ch = ch - 'a' + 10;
      else
         break;
      value = value * base + ch;
   }
   if(end)
      *end = s - 1;
   if(neg)
      value = -(int)value;
   return value;
}


int atoi(const char *s)
{
   return strtol(s, NULL, 10);
}


char *itoa(int num, char *dst, int base)
{
   int digit, negate=0, place;
   char c, text[20];

   if(base == 10 && num < 0)
   {
      num = -num;
      negate = 1;
   }
   text[16] = 0;
   for(place = 15; place >= 0; --place)
   {
      if(base == 10)
         digit = num % base;
      else
         digit = (unsigned int)num % (unsigned int)base;
      if(num == 0 && place < 15 && base == 10 && negate)
      {
         c = '-';
         negate = 0;
      }
      else if(digit < 10)
         c = (char)('0' + digit);
      else
         c = (char)('a' + digit - 10);
      text[place] = c;
      num = (unsigned int)num / (unsigned int)base;
      if(num == 0 && negate == 0)
         break;
   }
   strcpy(dst, text + place);
   return dst;
}


int sprintf(char *s, const char *format, 
            int arg0, int arg1, int arg2, int arg3,
            int arg4, int arg5, int arg6, int arg7)
{
   int argv[8];
   int argc=0, width, length;
   char f, text[20];

   argv[0] = arg0; argv[1] = arg1; argv[2] = arg2; argv[3] = arg3;
   argv[4] = arg4; argv[5] = arg5; argv[6] = arg6; argv[7] = arg7;

   for(;;)
   {
      f = *format++;
      if(f == 0)
         return argc;
      else if(f == '%')
      {
         width = 0;
         f = *format++;
         if(f == 0)
            return argc;
         if('0' <= f && f <= '9')
         {
            width = f - '0';
            f = *format++;
            if(f == 0)
               return argc;
            if('0' <= f && f <= '9')
               width = width * 10 + f - '0';
         }

         if(f == 'd')
         {
            memset(s, ' ', width);
            itoa(argv[argc++], text, 10);
            length = (int)strlen(text);
            if(width < length)
               width = length;
            strcpy(s + width - length, text);
         }
         else if(f == 'x' || f == 'f')
         {
            memset(s, '0', width);
            itoa(argv[argc++], text, 16);
            length = (int)strlen(text);
            if(width < length)
               width = length;
            strcpy(s + width - length, text);
         }
         else if(f == 'c')
         {
            *s++ = (char)argv[argc++];
            *s = 0;
         }
         else if(f == 's')
         {
            length = strlen((char*)argv[argc]);
            if(width > length)
            {
               memset(s, ' ', width - length);
               s += width - length;
            }
            strcpy(s, (char*)argv[argc++]);
         }
         s += strlen(s);
      }
      else if(f == '\\')
      {
         f = *format++;
         if(f == 0)
            return argc;
         else if(f == 'n')
            *s++ = '\n';
         else if(f == 'r')
            *s++ = '\r';
         else if(f == 't')
            *s++ = '\t';
      }
      else
      {
         *s++ = f;
      }
      *s = 0;
   }
}


int sscanf(const char *s, const char *format,
           int arg0, int arg1, int arg2, int arg3,
           int arg4, int arg5, int arg6, int arg7)
{
   int argv[8];
   int argc=0, length;
   char f;

   argv[0] = arg0; argv[1] = arg1; argv[2] = arg2; argv[3] = arg3;
   argv[4] = arg4; argv[5] = arg5; argv[6] = arg6; argv[7] = arg7;

   for(;;)
   {
      if(*s == 0)
         return argc;
      f = *format++;
      if(f == 0)
         return argc;
      else if(f == '%')
      {
         while(isspace(*s))
            ++s;
         f = *format++;
         if(f == 0)
            return argc;
         if(f == 'd')
            *(int*)argv[argc++] = strtol(s, &s, 10);
         else if(f == 'x')
            *(int*)argv[argc++] = strtol(s, &s, 16);
         else if(f == 'c')
            *(char*)argv[argc++] = *s++;
         else if(f == 's')
         {
            length = 0;
            while(!isspace(s[length]))
               ++length;
            strncpy((char*)argv[argc++], s, length);
            s += length;
         }
      }
      else 
      {
         if(f == '\\')
         {
            f = *format++;
            if(f == 0)
               return argc;
            else if(f == 'n')
               f = '\n';
            else if(f == 'r')
               f = '\r';
            else if(f == 't')
               f = '\t';
         }
         while(*s && *s != f)
            ++s;
         if(*s)
            ++s;
      }
   }
}


void dump(const unsigned char *data, int length)
{
   int i, index=0, value;
   char string[80];
   memset(string, 0, sizeof(string));
   for(i = 0; i < length; ++i)
   {
      if((i & 15) == 0)
      {
         if(strlen(string))
            printf("%s\n", string);
         printf("%4x ", i);
         memset(string, 0, sizeof(string));
         index = 0;
      }
      value = data[i];
      printf("%2x ", value);
      if(isprint(value))
         string[index] = (char)value;
      else
         string[index] = '.';
      ++index;
   }
   for(; index < 16; ++index)
      printf("   ");
   printf("%s\n", string);
}



