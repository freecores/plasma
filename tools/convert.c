//convert.c by Steve Rhoads 4/26/01
//Now uses the ELF format (get gccmips_elf.zip)
//set $gp and zero .sbss and .bss
//Reads test.exe and creates code.txt
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUF_SIZE (1024*1024*4) 
/*Assumes running on PC little endian*/
#define ntohl(A) (((A)>>24)|(((A)&0x00ff0000)>>8)|(((A)&0xff00)<<8)|((A)<<24))
#define ntohs(A) (unsigned short)((((A)&0xff00)>>8)|((A)<<8))

#define EI_NIDENT 16
#define SHT_PROGBITS 1
#define SHT_STRTAB 3
#define SHT_NOBITS 8

typedef struct
{
   unsigned char  e_ident[EI_NIDENT];
   unsigned short e_e_type;
   unsigned short e_machine;
   unsigned long  e_version;
   unsigned long  e_entry;
   unsigned long  e_phoff;
   unsigned long  e_shoff;
   unsigned long  e_flags;
   unsigned short e_ehsize;
   unsigned short e_phentsize;
   unsigned short e_phnum;
   unsigned short e_shentsize;
   unsigned short e_shnum;
   unsigned short e_shstrndx;
} ElfHeader;

typedef struct
{
   unsigned long p_type;
   unsigned long p_offset;
   unsigned long p_vaddr;
   unsigned long p_paddr;
   unsigned long p_filesz;
   unsigned long p_memsz;
   unsigned long p_flags;
   unsigned long p_align;
} Elf32_Phdr;

typedef struct
{
   unsigned long sh_name;
   unsigned long sh_type;
   unsigned long sh_flags;
   unsigned long sh_addr;
   unsigned long sh_offset;
   unsigned long sh_size;
   unsigned long sh_link;
   unsigned long sh_info;
   unsigned long sh_addralign;
   unsigned long sh_entsize;
} Elf32_Shdr;

typedef struct 
{
   unsigned long ri_gprmask;
   unsigned long ri_cprmask[4];
   unsigned long ri_gp_value;
} ELF_RegInfo;

#define PT_MIPS_REGINFO  0x70000000
#define SHT_MIPS_REGINFO 0x70000006

void set_low(unsigned char *ptr, unsigned long address, unsigned long value)
{
   unsigned long opcode;
   opcode = *(unsigned long *)(ptr + address);
   opcode = ntohl(opcode);
   opcode = (opcode & 0xffff0000) | (value & 0xffff);
   opcode = ntohl(opcode);
   *(unsigned long *)(ptr + address) = opcode;
}

int main(int argc, char *argv[])
{
   FILE *infile, *outfile, *txtfile;
   unsigned char *buf, *code;
   long size, stack_pointer;
   unsigned long length, d, i, gp_ptr = 0, gp_ptr_backup = 0;
   unsigned long bss_start = 0, bss_end = 0;

   ElfHeader *elfHeader;
   Elf32_Phdr *elfProgram;
   ELF_RegInfo *elfRegInfo;
   Elf32_Shdr *elfSection;
   (void)argc; 
   (void)argv;

   printf("test.exe -> code.txt & test.bin\n");
   infile = fopen("test.exe", "rb");
   if(infile == NULL)
   {
      printf("Can't open test.exe");
      return 0;
   }
   buf = (unsigned char *)malloc(BUF_SIZE);
   size = (int)fread(buf, 1, BUF_SIZE, infile);
   fclose(infile);
   code = (unsigned char *)malloc(BUF_SIZE);
   memset(code, 0, BUF_SIZE);

   elfHeader = (ElfHeader *)buf;
   if(strncmp((char*)elfHeader->e_ident + 1, "ELF", 3))
   {
      printf("Error:  Not an ELF file!\n");
      printf("Use the gccmips_elf.zip from opencores/projects/plasma!\n");
      return -1;
   }

   elfHeader->e_entry = ntohl(elfHeader->e_entry);
   elfHeader->e_phoff = ntohl(elfHeader->e_phoff);
   elfHeader->e_shoff = ntohl(elfHeader->e_shoff);
   elfHeader->e_flags = ntohl(elfHeader->e_flags);
   elfHeader->e_phentsize = ntohs(elfHeader->e_phentsize);
   elfHeader->e_phnum = ntohs(elfHeader->e_phnum);
   elfHeader->e_shentsize = ntohs(elfHeader->e_shentsize);
   elfHeader->e_shnum = ntohs(elfHeader->e_shnum);
   printf("Entry=0x%x ", elfHeader->e_entry);
   length = 0;

   for(i = 0; i < elfHeader->e_phnum; ++i)
   {
      elfProgram = (Elf32_Phdr *)(buf + elfHeader->e_phoff +
                         elfHeader->e_phentsize * i);
      elfProgram->p_type = ntohl(elfProgram->p_type);
      elfProgram->p_offset = ntohl(elfProgram->p_offset);
      elfProgram->p_vaddr = ntohl(elfProgram->p_vaddr);
      elfProgram->p_filesz = ntohl(elfProgram->p_filesz);
      elfProgram->p_memsz = ntohl(elfProgram->p_memsz);
      elfProgram->p_flags = ntohl(elfProgram->p_flags);

      elfProgram->p_vaddr -= elfHeader->e_entry;

      if(elfProgram->p_type == PT_MIPS_REGINFO)
      {
         elfRegInfo = (ELF_RegInfo*)(buf + elfProgram->p_offset);
         gp_ptr = ntohl(elfRegInfo->ri_gp_value);
      }
      if(elfProgram->p_vaddr < BUF_SIZE)
      {
         //printf("[0x%x,0x%x,0x%x,0x%x,0x%x]\n", elfProgram->p_vaddr,
         //   elfProgram->p_offset, elfProgram->p_filesz, elfProgram->p_memsz,
         //   elfProgram->p_flags);
         memcpy(code + elfProgram->p_vaddr, buf + elfProgram->p_offset,
                 elfProgram->p_filesz);
         length = elfProgram->p_vaddr + elfProgram->p_filesz;
         //printf("length = %d 0x%x\n", length, length);
      }
   }

   for(i = 0; i < elfHeader->e_shnum; ++i)
   {
      elfSection = (Elf32_Shdr *)(buf + elfHeader->e_shoff +
                         elfHeader->e_shentsize * i);
      elfSection->sh_name = ntohl(elfSection->sh_name);
      elfSection->sh_type = ntohl(elfSection->sh_type);
      elfSection->sh_addr = ntohl(elfSection->sh_addr);
      elfSection->sh_offset = ntohl(elfSection->sh_offset);
      elfSection->sh_size = ntohl(elfSection->sh_size);

      if(elfSection->sh_type == SHT_MIPS_REGINFO)
      {
         elfRegInfo = (ELF_RegInfo*)(buf + elfSection->sh_offset);
         gp_ptr = ntohl(elfRegInfo->ri_gp_value);
      }
      if(elfSection->sh_type == SHT_PROGBITS)
      {
         //printf("elfSection->sh_addr=0x%x\n", elfSection->sh_addr);
         if(elfSection->sh_addr > gp_ptr_backup)
            gp_ptr_backup = elfSection->sh_addr;
      }
      if(elfSection->sh_type == SHT_NOBITS)
      {
         if(bss_start == 0)
         {
            bss_start = elfSection->sh_addr;
         }
         bss_end = elfSection->sh_addr + elfSection->sh_size;
      }
   }

   if(length > bss_start - elfHeader->e_entry)
   {
      length = bss_start - elfHeader->e_entry;
   }
   if(bss_start == length)
   {
      bss_start = length;
      bss_end = length + 4;
   }
   if(gp_ptr == 0)
      gp_ptr = gp_ptr_backup + 0x7ff0;

   /*Initialize the $gp register for sdata and sbss */
   printf("gp_ptr=0x%x ", gp_ptr);
   /*modify the first opcodes in boot.asm */
   /*modify the lui opcode */
   set_low(code, 0, gp_ptr >> 16);
   /*modify the ori opcode */
   set_low(code, 4, gp_ptr & 0xffff);

   /*Clear .sbss and .bss */
   printf("sbss=0x%x bss_end=0x%x\nlength=0x%x ", bss_start, bss_end, length);
   set_low(code, 8, bss_start >> 16);
   set_low(code, 12, bss_start & 0xffff);
   set_low(code, 16, bss_end >> 16);
   set_low(code, 20, bss_end & 0xffff);

   /*Set stack pointer */
   if(elfHeader->e_entry < 0x10000000)
      stack_pointer = bss_end + 512;
   else
      stack_pointer = bss_end + 1024 * 4;
   stack_pointer &= ~7;
   printf("SP=0x%x\n", stack_pointer);
   set_low(code, 24, stack_pointer >> 16);
   set_low(code, 28, stack_pointer & 0xffff);

   /*write out test.bin */
   outfile = fopen("test.bin", "wb");
   fwrite(code, length, 1, outfile);
   fclose(outfile);

   /*write out code.txt */
   txtfile = fopen("code.txt", "w");
   for(i = 0; i <= length; i += 4)
   {
      d = ntohl(*(unsigned long *)(code + i));
      fprintf(txtfile, "%8.8x\n", d);
   }
   fclose(txtfile);
   free(buf);

   return 0;
}

