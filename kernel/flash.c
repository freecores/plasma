/*--------------------------------------------------------------------
 * TITLE: Plasma Flash
 * AUTHOR: Steve Rhoads (rhoadss@yahoo.com)
 * DATE CREATED: 12/17/05
 * FILENAME: plasma.h
 * PROJECT: Plasma CPU core
 * COPYRIGHT: Software placed into the public domain by the author.
 *    Software 'as is' without warranty.  Author liable for nothing.
 * DESCRIPTION:
 *    Plasma flash controller
 *    Only the lower 16-bits of each 32-bit word are connected --
 *    this changes the address mapping to the flash.
 *    ByteOffset and bytes must be a multiple of two.
 *--------------------------------------------------------------------*/
#include "plasma.h"
#include "rtos.h"


void FlashRead(uint16 *dst, uint32 byteOffset, int bytes)
{
   volatile uint32 *ptr=(uint32*)(FLASH_BASE + (byteOffset << 1));
   *ptr = 0xff;                   //read mode
   while(bytes > 0)
   {
      *dst++ = (uint16)*ptr++;
      bytes -= 2;
   }
}


void FlashWrite(uint16 *src, uint32 byteOffset, int bytes)
{
   volatile uint32 *ptr=(uint32*)(FLASH_BASE + (byteOffset << 1));
   while(bytes > 0)
   {
      *ptr = 0x40;                //write mode
      *ptr++ = *src++;            //write data
      while((*ptr & 0x80) == 0)   //check status
         ;
      bytes -= 2;
   }
}


void FlashErase(uint32 byteOffset)
{
   volatile uint32 *ptr=(uint32*)(FLASH_BASE + (byteOffset << 1));
   *ptr = 0x20;                   //erase block
   *ptr = 0xd0;                   //confirm
   while((*ptr & 0x80) == 0)      //check status
      ;
}
