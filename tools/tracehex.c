/***********************************************************
| tracehex by Steve Rhoads 12/25/01
| This tool modifies trace files from the free VHDL simulator 
| http://www.symphonyeda.com/.  
| The binary numbers are converted to hex values.
************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define BUF_SIZE (1024*1024)

char drop_char[10000];

int main(int argc, char *argv[])
{
   FILE *file;
   char *buf,*ptr_in,line[1000],*ptr_out;
   int bytes,digits,value,isbinary,col,col_num,row,drop_cnt;
   int col_index,line_index,back_count,temp;
   (void)argc,argv;

   /* Reading trace.txt */
   file=fopen("trace.txt","r");
   if(file==NULL) {
      printf("Can't open file\n");
      return -1;
   }
   buf=(char*)malloc(BUF_SIZE*2);
   ptr_out=buf+BUF_SIZE;
   bytes=fread(buf,1,BUF_SIZE-1,file);
   buf[bytes]=0;
   fclose(file);

   digits=0;
   value=0;
   isbinary=0;
   col=0;
   col_num=0;
   row=0;
   for(ptr_in=strstr(buf,"=");*ptr_in;++ptr_in) {
      ++col;
      if(col<4||col==10||col==11||col==12) {
         drop_char[col]=1;
         continue;
      }
  
      /* convert binary number to hex */
      if(isbinary&&(*ptr_in=='0'||*ptr_in=='1')) {
         value=value*2+*ptr_in-'0';
         ++digits;
         drop_char[col_num++]=1;
      } else if(isbinary&&*ptr_in=='Z') {
         value=1000;
         ++digits;
         drop_char[col_num++]=1;
      } else if(isbinary&&*ptr_in=='U') {
         value=10000;
         ++digits;
         drop_char[col_num++]=1;
      } else {
         /* end of binary number? */
         if(digits) {
            drop_char[--col_num]=0;
            if(value<100) {
               /* test if digits not divisible by 4 */
               if('0'<=ptr_out[-1]&&ptr_out[-1]<='9') {
                  /* adjust previous digit */
                  value=value+((ptr_out[-1]-'0')<<digits);
                  temp=(value>>4);
                  ptr_out[-1]=temp<10?temp+'0':temp-10+'A';
               } else if('A'<=ptr_out[-1]&&ptr_out[-1]<='F') {
                  value=value+((ptr_out[-1]-'A'+10)<<digits);
                  temp=(value>>4);
                  ptr_out[-1]=temp<10?temp+'0':temp-10+'A';
               } 
               value&=0xf;
               *ptr_out++=value<10?value+'0':value-10+'A';
            } else if(value<5000) {
               *ptr_out++='Z';
            } else {
               *ptr_out++='U';
            }
         }
         if(*ptr_in=='\n') {
            col=0;
            isbinary=0;
            ++row;
         }
         if(isspace(*ptr_in)) {
            if(col>10) {
               isbinary=1;
               col_num=col;
            }
         } else {
            isbinary=0;
         }
         *ptr_out++=*ptr_in;
         digits=0;
         value=0;
      }

      /* convert every four binary digits to a hex digit */
      if(digits==4) {
         drop_char[--col_num]=0;
         if(value<100) {
            *ptr_out++=value<10?value+'0':value-10+'A';
         } else if(value<5000) {
            *ptr_out++='Z';
         } else {
            *ptr_out++='U';
         }
         digits=0;
         value=0;
      }
   }
   *ptr_out=0;

   /* now process the header */
   file=fopen("trace2.txt","w");
   col=0;
   line[0]=0;
   for(ptr_in=buf;*ptr_in;++ptr_in) {
      if(*ptr_in=='=') {
         break;
      }
      line[col++]=*ptr_in;
      if(*ptr_in=='\n') {
         line[col]=0;
         line_index=0;
         for(col_index=0;col_index<col;++col_index) {
            if(drop_char[col_index]) {
               back_count=0;
               while(line[line_index-back_count]!=' '&&back_count<10) {
                  ++back_count;
               }
               if(line[line_index-back_count-1]!=' ') {
                  --back_count;
               }
               strcpy(line+line_index-back_count,line+line_index-back_count+1);
            } else {
               ++line_index;
            }
         }
         fprintf(file,"%s",line);
         col=0;
      }
   }
   drop_cnt=0;
   for(col_index=13;col_index<sizeof(drop_char);++col_index) {
      if(drop_char[col_index]) {
         ++drop_cnt;
      }
   }
   fprintf(file,"%s",buf+BUF_SIZE+drop_cnt);

   fclose(file);
   free(buf);
   return 0;
}
