//convert.c by Steve Rhoads 4/26/01
//This program takes a little-endian MIPS executable and
//converts it to a big-endian executable and changes
//absolute jumps to branches.
#include <stdio.h>
#include <stdlib.h>

#define BUF_SIZE (1024*1024)
#define CODE_OFFSET 0x200

int main(int argc,char *argv[])
{
   FILE *infile,*outfile;
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
   outfile=fopen("code.txt","w");
   infile=fopen("test2.exe","wb");
   for(i=CODE_OFFSET;i<size;i+=4) {
      d=(buf[i+3]<<24)|(buf[i+2]<<16)|(buf[i+1]<<8)|buf[i];
      if((d>>24)==0x0c) {          //JAL
         j=(d&0xfffff)-0x400-((i-CODE_OFFSET)>>2)-1;   //BGEZAL
         d=0x04110000+(j&0xffff); 
      }
      if(i==CODE_OFFSET) {
         d=0x341d8000;  //ori $29,0,0x8000
      }
      fprintf(outfile,"%8.8x\n",d);
      fwrite(&d,4,1,infile);
   }
   fclose(outfile);
   fclose(infile);
   free(buf);
   return 0;
}

