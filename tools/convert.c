//convert.c by Steve Rhoads 4/26/01
//This program strips off the first 0x1000 bytes
#include <stdio.h>
#include <stdlib.h>

#define BUF_SIZE (1024*1024)
#define CODE_OFFSET 0x1000

int main(int argc,char *argv[])
{
   FILE *infile,*outfile,*txtfile;
   unsigned char *buf;
   long size,i,j;
   unsigned long d;
   infile=fopen("test.exe","rb");
   if(infile==NULL) {
      printf("Can't open test.exe");
      return 0;
   }
   buf=(unsigned char*)malloc(BUF_SIZE);
   size=fread(buf,1,BUF_SIZE,infile);
   fclose(infile);
   outfile=fopen("test2.exe","wb");
   txtfile=fopen("code.txt","w");
   for(i=CODE_OFFSET;i<size;i+=4) {
      d=(buf[i]<<24)|(buf[i+1]<<16)|(buf[i+2]<<8)|buf[i+3];
//      if((d>>24)==0x0c) {          //JAL
//         j=(d&0xfffff)-0x400-((i-CODE_OFFSET)>>2)-1;   //BGEZAL
//         d=0x04110000+(j&0xffff); 
//      }
      fprintf(txtfile,"%8.8x\n",d);
      fwrite(buf+i,4,1,outfile);
   }
   fclose(outfile);
   fclose(txtfile);
   free(buf);
   return 0;
}

