//convert.c by Steve Rhoads 4/26/01
//set $gp and zero .sbss and .bss
#include <stdio.h>
#include <stdlib.h>

#define BUF_SIZE (1024*1024)
#define ntohl(A) ((A>>24)|((A&0x00ff0000)>>8)|((A&0xff00)<<8)|(A<<24))

enum {
   TEXT_OFFSET,  TEXT_LENGTH,
   RDATA_OFFSET, RDATA_LENGTH,
   DATA_OFFSET,  DATA_LENGTH,
   SDATA_OFFSET, SDATA_LENGTH,
   SBSS_OFFSET,  SBSS_LENGTH,
   BSS_OFFSET,   BSS_LENGTH
};

long code_start_offset=0x60;
unsigned long map[12];
long offset[]={
   0x58, 0x5c,  //.text offset,length
   0x80, 0x84,  //.rdata offset,length
   0xa8, 0xac,  //.data offset,length
   0xd0, 0xd4,  //.sdata offset,length
   0xf8, 0xfc,  //.sbss offset,length
   0x120,0x124  //.bss offset,length
};

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
   long size,code_offset;
   unsigned long i,d,gp_ptr;

   printf("test.exe -> code.txt & test2.exe\n");
   infile=fopen("test.exe","rb");
   if(infile==NULL) {
      printf("Can't open test.exe");
      return 0;
   }
   buf=(unsigned char*)malloc(BUF_SIZE);
   size=fread(buf,1,BUF_SIZE,infile);
   fclose(infile);

   code_offset=load(buf,code_start_offset);
   printf("code_offset=0x%x\n",code_offset);
   code=buf+code_offset;
   /*load all of the segment offsets and lengths*/
   for(i=0;i<12;++i) {
      map[i]=load(buf,offset[i]);
   }
   if(code_offset<0x120) {
      printf("no .sdata\n");
      for(i=6;i<12;i+=2) {
         map[i]=map[4];
         map[i+1]=map[5];
      }
   } else if(code_offset<0x140) {
      printf("no .rdata\n");
      for(i=11;i>4;--i) {
         map[i]=map[i-2];
      }
   }
   for(i=0;i<12;i+=2) {
      printf("0x%x 0x%x\n",map[i],map[i+1]);
   }

   /*Initialize the $gp register for sdata and sbss*/   
   gp_ptr=map[SDATA_OFFSET]+0x8000;
   printf("gp_ptr=0x%x\n",gp_ptr);
   /*modify the first opcodes in boot.asm*/
   /*modify the lui opcode*/
   set_low(code,0,gp_ptr>>16);
   /*modify the ori opcode*/
   set_low(code,4,gp_ptr&0xffff);

   /*Clear .sbss and .bss*/
   printf(".sbss=0x%x .bss_end=0x%x\n",
      map[SBSS_OFFSET],map[BSS_OFFSET]+map[BSS_LENGTH]);
   set_low(code,8,map[SBSS_OFFSET]);
   set_low(code,12,map[BSS_OFFSET]+map[BSS_LENGTH]);

   /*write out code.txt*/
   outfile=fopen("test2.exe","wb");
   txtfile=fopen("code.txt","w");
   for(i=0;i<=map[SDATA_OFFSET]+map[SDATA_LENGTH];i+=4) {
      d=load(code,i);
      fprintf(txtfile,"%8.8x\n",d);
      fwrite(code+i,4,1,outfile);
   }
   fclose(outfile);
   fclose(txtfile);
   free(buf);

   return 0;
}

