//convert.c by Steve Rhoads 4/26/01
//set $gp and zero .sbss and .bss
#include <stdio.h>
#include <stdlib.h>

#define BUF_SIZE (1024*1024)
/*Assumes running on PC little endian*/
#define ntohl(A) ((A>>24)|((A&0x00ff0000)>>8)|((A&0xff00)<<8)|(A<<24))

#define CODE_START 0x60
#define SECTION_START 0x4c
#define SECTION_END 0x160
#define SECTION_SIZE 0x28
#define SECTION_OFFSET 0xc
#define SECTION_LENGTH 0x10

struct header_t {
   unsigned long text_offset,text_length;
   unsigned long rdata_offset,rdata_length;
   unsigned long data_offset,data_length;
   unsigned long sdata_offset,sdata_length;
   unsigned long sbss_offset,sbss_length;
   unsigned long bss_offset,bss_length;
} header;

unsigned long load(char *ptr,unsigned long address)
{
   unsigned long value;
   value=*(unsigned long*)(ptr+address);
   value=ntohl(value);
   return value;
}

void set_low(char *ptr,unsigned long address,unsigned long value)
{
   unsigned long opcode;
   opcode=*(unsigned long*)(ptr+address);
   opcode=ntohl(opcode);
   opcode=(opcode&0xffff0000)|(value&0xffff);
   opcode=ntohl(opcode);
   *(unsigned long*)(ptr+address)=opcode;
}

int main(int argc,char *argv[])
{
   FILE *infile,*outfile,*txtfile;
   unsigned char *buf,*code;
   long size;
   unsigned long code_offset,index,name,offset,length,d,i,gp_ptr;

   printf("test.exe -> code.txt & test2.exe\n");
   infile=fopen("test.exe","rb");
   if(infile==NULL) {
      printf("Can't open test.exe");
      return 0;
   }
   buf=(unsigned char*)malloc(BUF_SIZE);
   size=fread(buf,1,BUF_SIZE,infile);
   fclose(infile);

   code_offset=load(buf,CODE_START);
   printf("code_offset=0x%x ",code_offset);
   code=buf+code_offset;

   /*load all of the segment offsets and lengths*/
   for(index=SECTION_START;index<code_offset-0x20;index+=SECTION_SIZE) {
      name=load(buf,index);
      offset=load(buf,index+SECTION_OFFSET);
      length=load(buf,index+SECTION_LENGTH);
      switch(name) {
      case 0x2e746578: /*.text*/
         header.text_offset=offset;
         header.text_length=length;
         offset+=length;
         length=0;
         header.rdata_offset=offset;
         header.rdata_length=length;
         header.data_offset=offset;
         header.data_length=length;
         header.sdata_offset=offset;
         header.sdata_length=length;
         header.sbss_offset=offset;
         header.sbss_length=length;
         header.bss_offset=offset;
         header.bss_length=length;
         break;
      case 0x2e726461: /*.rdata*/
         header.rdata_offset=offset;
         header.rdata_length=length;
         offset+=length;
         length=0;
         header.data_offset=offset;
         header.data_length=length;
         header.sdata_offset=offset;
         header.sdata_length=length;
         header.sbss_offset=offset;
         header.sbss_length=length;
         header.bss_offset=offset;
         header.bss_length=length;
         break;
      case 0x2e646174: /*.data*/
         header.data_offset=offset;
         header.data_length=length;
         offset+=length;
         length=0;
         header.sdata_offset=offset;
         header.sdata_length=length;
         header.sbss_offset=offset;
         header.sbss_length=length;
         header.bss_offset=offset;
         header.bss_length=length;
         break;
      case 0x2e736461: /*.sdata*/
         header.sdata_offset=offset;
         header.sdata_length=length;
         offset+=length;
         length=0;
         header.sbss_offset=offset;
         header.sbss_length=length;
         header.bss_offset=offset;
         header.bss_length=length;
         break;
      case 0x2e736273: /*.sbss*/
         header.sbss_offset=offset;
         header.sbss_length=length;
         offset+=length;
         length=0;
         header.bss_offset=offset;
         header.bss_length=length;
         break;
      case 0x2e627373:  /*.bss*/
         header.bss_offset=offset;
         header.bss_length=length;
         break;
      default: printf("unknown 0x%x\n",name);
      }
   }

   /*Initialize the $gp register for sdata and sbss*/   
   gp_ptr=header.sdata_offset+0x8000;
   printf("gp_ptr=0x%x ",gp_ptr);
   /*modify the first opcodes in boot.asm*/
   /*modify the lui opcode*/
   set_low(code,0,gp_ptr>>16);
   /*modify the ori opcode*/
   set_low(code,4,gp_ptr&0xffff);

   /*Clear .sbss and .bss*/
   printf(".sbss=0x%x .bss_end=0x%x\n",
      header.sbss_offset,header.bss_offset+header.bss_length);
   set_low(code,8,header.sbss_offset);
   set_low(code,12,header.bss_offset+header.bss_length);

   /*write out code.txt*/
   outfile=fopen("test2.exe","wb");
   txtfile=fopen("code.txt","w");
   for(i=0;i<=header.sdata_offset+header.sdata_length;i+=4) {
      d=load(code,i);
      fprintf(txtfile,"%8.8x\n",d);
      fwrite(code+i,4,1,outfile);
   }
   fclose(outfile);
   fclose(txtfile);
   free(buf);

   return 0;
}

