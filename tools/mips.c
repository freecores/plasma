/*-------------------------------------------------------------------
-- TITLE: MIPS CPU simulator
-- AUTHOR: Steve Rhoads (rhoadss@yahoo.com)
-- DATE CREATED: 1/31/01
-- FILENAME: mips.c
-- PROJECT: MIPS CPU core
-- COPYRIGHT: Software placed into the public domain by the author.
--    Software 'as is' without warranty.  Author liable for nothing.
-- DESCRIPTION:
--   MIPS CPU simulator in C code.  
--   This file served as the starting point for the VHDL code.
--------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#define MEM_SIZE (1024*1024*2)
#define ntohs(A) ( ((A)>>8) || (((A)&0xff)<<8) )
#define htons(A) ntohs(A)
#define ntohl(A) ( ((A)>>24) || (((A)&0xff0000)>>8) || (((A)&0xff00)<<8) || ((A)<<24) )
#define htonl(A) ntohl(A)

int getch(void);

typedef struct {
   long r[32];
   long pc,pc_next;
   long hi;
   long lo;
   long skip;
   char *mem;
   long wakeup;
   long big_endian;
} State;

static char *opcode_string[]={
   "SPECIAL","REGIMM","J","JAL","BEQ","BNE","BLEZ","BGTZ",
   "ADDI","ADDIU","SLTI","SLTIU","ANDI","ORI","XORI","LUI",
   "COP0","COP1","COP2","COP3","BEQL","BNEL","BLEZL","BGTZL",
   "?","?","?","?","?","?","?","?",
   "LB","LH","LWL","LW","LBU","LHU","LWR","?",
   "SB","SH","SWL","SW","?","?","SWR","CACHE",
   "LL","LWC1","LWC2","LWC3","?","LDC1","LDC2","LDC3"
   "SC","SWC1","SWC2","SWC3","?","SDC1","SDC2","SDC3"
};

static char *special_string[]={
   "SLL","?","SRL","SRA","SLLV","?","SRLV","SRAV",
   "JR","JALR","MOVZ","MOVN","SYSCALL","BREAK","?","SYNC",
   "MFHI","MTHI","MFLO","MTLO","?","?","?","?",
   "MULT","MULTU","DIV","DIVU","?","?","?","?",
   "ADD","ADDU","SUB","SUBU","AND","OR","XOR","NOR",
   "?","?","SLT","SLTU","?","DADDU","?","?",
   "TGE","TGEU","TLT","TLTU","TEQ","?","TNE","?",
   "?","?","?","?","?","?","?","?"
};

static char *regimm_string[]={
   "BLTZ","BGEZ","BLTZL","BGEZL","?","?","?","?",
   "TGEI","TGEIU","TLTI","TLTIU","TEQI","?","TNEI","?",
   "BLTZAL","BEQZAL","BLTZALL","BGEZALL","?","?","?","?",
   "?","?","?","?","?","?","?","?"
};

static long big_endian=0;

static long mem_read(State *s,long size,unsigned long address)
{
   long value=0;
   address%=MEM_SIZE;
   address+=(long)s->mem;
   switch(size) {
      case 4: value=*(long*)address;
         if(big_endian) value=ntohl(value);
         break;
      case 2: if(big_endian) address^=2;
         value=*(unsigned short*)address;
         if(big_endian) value=ntohs((unsigned short)value);
         break;
      case 1: if(big_endian) address^=3;
         value=*(unsigned char*)address;
         break;
      default: printf("ERROR");
   }
   return(value);
}

static void mem_write(State *s,long size,long unsigned address,long value)
{
   static char_count=0;
   if(address==0xffff) {          //UART write register at 0xffff
      if(isprint(value)) {
         printf("%c",value);
         if(++char_count>=72) {
            printf("\n");
            char_count=0;
         }
      } else if(value=='\n') {
         printf("\n");
         char_count=0;
      } else {
         printf(".");
      }
   }
   address%=MEM_SIZE;
   address+=(long)s->mem;
   switch(size) {
      case 4: if(big_endian) value=htonl(value);
         *(long*)address=value;
         break;
      case 2:
         if(big_endian) {
            address^=2;
            value=htons((unsigned short)value);
         }
         *(short*)address=(short)value; 
         break;
      case 1: if(big_endian) address^=3;
         *(char*)address=(char)value; 
         break;
      default: printf("ERROR");
   }
}

//execute one cycle of a MIPS CPU
void cycle(State *s,int show_mode)
{
   volatile unsigned long opcode;
   volatile unsigned long op,rs,rt,rd,re,func,imm,target;
   volatile long imm_shift,branch=0,lbranch=2;
   volatile long *r=s->r;
   volatile unsigned long *u=(unsigned long*)s->r;
   volatile unsigned long ptr;
   opcode=mem_read(s,4,s->pc);
   op=(opcode>>26)&0x3f;
   rs=(opcode>>21)&0x1f;
   rt=(opcode>>16)&0x1f;
   rd=(opcode>>11)&0x1f;
   re=(opcode>>6)&0x1f;
   func=opcode&0x3f;
   imm=opcode&0xffff;
   imm_shift=(((long)(short)imm)<<2)-4;
   target=(opcode<<6)>>4;
   ptr=(short)imm+r[rs];
   r[0]=0;
   if(show_mode) {
      printf("%8.8lx %8.8lx ",s->pc,opcode);
      if(op==0) printf("%8s ",special_string[func]);
      else if(op==1) printf("%8s ",regimm_string[rt]);
      else printf("%8s ",opcode_string[op]);
      printf("$%2.2ld $%2.2ld $%2.2ld $%2.2ld ",rs,rt,rd,re);
      printf("%4.4lx\n",imm);
   }
   if(show_mode>5) return;
   s->pc=s->pc_next;
   s->pc_next=s->pc_next+4;
   if(s->skip) {
      s->skip=0;
      return;
   }
   switch(op) {
      case 0x00:/*SPECIAL*/
         switch(func) {
            case 0x00:/*SLL*/  r[rd]=r[rt]<<re;          break;
            case 0x02:/*SRL*/  r[rd]=u[rt]>>re;          break;
            case 0x03:/*SRA*/  r[rd]=r[rt]>>re;          break;
            case 0x04:/*SLLV*/ r[rd]=r[rt]<<r[rs];       break;
            case 0x06:/*SRLV*/ r[rd]=u[rt]>>r[rs];       break;
            case 0x07:/*SRAV*/ r[rd]=r[rt]>>r[rs];       break;
            case 0x08:/*JR*/   s->pc_next=r[rs];         break;
            case 0x09:/*JALR*/ r[rd]=s->pc_next; s->pc_next=r[rs]; break;
            case 0x0a:/*MOVZ*/ if(!r[rt]) r[rd]=r[rs];   break;  /*IV*/
            case 0x0b:/*MOVN*/ if(r[rt]) r[rd]=r[rs];    break;  /*IV*/
            case 0x0c:/*SYSCALL*/                        break;
            case 0x0d:/*BREAK*/ s->wakeup=1; break;
            case 0x0f:/*SYNC*/ s->wakeup=1; break;
            case 0x10:/*MFHI*/ r[rd]=s->hi;              break;
            case 0x11:/*FTHI*/ s->hi=r[rs];              break;
            case 0x12:/*MFLO*/ r[rd]=s->lo;              break;
            case 0x13:/*MTLO*/ s->lo=r[rs];              break;
            case 0x18:/*MULT*/ s->lo=r[rs]*r[rt]; s->hi=0; break;
            case 0x19:/*MULTU*/ s->lo=r[rs]*r[rt]; s->hi=0; break;
            case 0x1a:/*DIV*/  s->lo=r[rs]/r[rt]; s->hi=r[rs]%r[rt]; break;
            case 0x1b:/*DIVU*/ s->lo=r[rs]/r[rt]; s->hi=r[rs]%r[rt]; break;
            case 0x20:/*ADD*/  r[rd]=r[rs]+r[rt];        break;
            case 0x21:/*ADDU*/ r[rd]=r[rs]+r[rt];        break;
            case 0x22:/*SUB*/  r[rd]=r[rs]-r[rt];        break;
            case 0x23:/*SUBU*/ r[rd]=r[rs]-r[rt];        break;
            case 0x24:/*AND*/  r[rd]=r[rs]&r[rt];        break;
            case 0x25:/*OR*/   r[rd]=r[rs]|r[rt];        break;
            case 0x26:/*XOR*/  r[rd]=r[rs]^r[rt];        break;
            case 0x27:/*NOR*/  r[rd]=~(r[rs]|r[rt]);     break;
            case 0x2a:/*SLT*/  r[rd]=r[rs]<r[rt];        break;
            case 0x2b:/*SLTU*/ r[rd]=u[rs]<u[rt];        break;
            case 0x2d:/*DADDU*/r[rd]=r[rs]+u[rt];        break;
            case 0x31:/*TGEU*/ break;
            case 0x32:/*TLT*/  break;
            case 0x33:/*TLTU*/ break;
            case 0x34:/*TEQ*/  break;
            case 0x36:/*TNE*/  break;
            default: printf("ERROR0(*0x%x~0x%x)\n",s->pc,opcode);
               s->wakeup=1;
         }
         break;
      case 0x01:/*REGIMM*/
         switch(rt) {
            case 0x10:/*BLTZAL*/ r[31]=s->pc_next;
            case 0x00:/*BLTZ*/   branch=r[rs]<0;   break;
            case 0x11:/*BGEZAL*/ r[31]=s->pc_next;
            case 0x01:/*BGEZ*/   branch=r[rs]>=0;  break;
            case 0x12:/*BLTZALL*/r[31]=s->pc_next;
            case 0x02:/*BLTZL*/  lbranch=r[rs]<0;  break;
            case 0x13:/*BGEZALL*/r[31]=s->pc_next;
            case 0x03:/*BGEZL*/  lbranch=r[rs]>=0; break;
            default: printf("ERROR1\n"); s->wakeup=1;
          }
         break;
      case 0x03:/*JAL*/    r[31]=s->pc_next;
      case 0x02:/*J*/      s->pc_next=(s->pc&0xf0000000)|target; break;
      case 0x04:/*BEQ*/    branch=r[rs]==r[rt];     break;
      case 0x05:/*BNE*/    branch=r[rs]!=r[rt];     break;
      case 0x06:/*BLEZ*/   branch=r[rs]<=0;         break;
      case 0x07:/*BGTZ*/   branch=r[rs]>0;          break;
      case 0x08:/*ADDI*/   r[rt]=r[rs]+(short)imm;  break;
      case 0x09:/*ADDIU*/  u[rt]=u[rs]+(short)imm;  break;
      case 0x0a:/*SLTI*/   r[rt]=r[rs]<(short)imm;  break;
      case 0x0b:/*SLTIU*/  u[rt]=u[rs]<(unsigned long)(short)imm; break;
      case 0x0c:/*ANDI*/   r[rt]=r[rs]&imm;         break;
      case 0x0d:/*ORI*/    r[rt]=r[rs]|imm;         break;
      case 0x0e:/*XORI*/   r[rt]=r[rs]^imm;         break;
      case 0x0f:/*LUI*/    r[rt]=(imm<<16);         break;
      case 0x10:/*COP0*/ break;
//      case 0x11:/*COP1*/ break;
//      case 0x12:/*COP2*/ break;
//      case 0x13:/*COP3*/ break;
      case 0x14:/*BEQL*/   lbranch=r[rs]==r[rt];    break;
      case 0x15:/*BNEL*/   lbranch=r[rs]!=r[rt];    break;
      case 0x16:/*BLEZL*/  lbranch=r[rs]<=0;        break;
      case 0x17:/*BGTZL*/  lbranch=r[rs]>0;         break;
//      case 0x1c:/*MAD*/  break;   /*IV*/
//      case 0x20:/*LB*/   r[rt]=*(signed char*)ptr;  break;
      case 0x20:/*LB*/   r[rt]=(signed char)mem_read(s,1,ptr);  break;
//      case 0x21:/*LH*/   r[rt]=*(signed short*)ptr; break;
      case 0x21:/*LH*/   r[rt]=(signed short)mem_read(s,2,ptr); break;
      case 0x22:/*LWL*/  break; //fixme
//      case 0x23:/*LW*/   r[rt]=*(long*)ptr;       break;
      case 0x23:/*LW*/   r[rt]=mem_read(s,4,ptr);   break;
//      case 0x24:/*LBU*/  r[rt]=*(unsigned char*)ptr; break;
      case 0x24:/*LBU*/  r[rt]=(unsigned char)mem_read(s,2,ptr); break;
//      case 0x25:/*LHU*/  r[rt]=*(unsigned short*)ptr; break;
      case 0x25:/*LHU*/  r[rt]=(unsigned short)mem_read(s,2,ptr); break;
      case 0x26:/*LWR*/  break; //fixme
//      case 0x28:/*SB*/   *(char*)ptr=(char)r[rt]; break;
      case 0x28:/*SB*/   mem_write(s,1,ptr,r[rt]);  break;
//      case 0x29:/*SH*/   *(short*)ptr=(short)r[rt]; break;
      case 0x29:/*SH*/   mem_write(s,2,ptr,r[rt]);  break;
      case 0x2a:/*SWL*/  break; //fixme
//      case 0x2b:/*SW*/   *(long*)ptr=r[rt];       break;
      case 0x2b:/*SW*/   mem_write(s,4,ptr,r[rt]);  break;
      case 0x2e:/*SWR*/  break; //fixme
      case 0x2f:/*CACHE*/break;
//      case 0x30:/*LL*/   r[rt]=*(long*)ptr;       break;
      case 0x30:/*LL*/   r[rt]=mem_read(s,4,ptr);   break;
//      case 0x31:/*LWC1*/ break;
//      case 0x32:/*LWC2*/ break;
//      case 0x33:/*LWC3*/ break;
//      case 0x35:/*LDC1*/ break;
//      case 0x36:/*LDC2*/ break;
//      case 0x37:/*LDC3*/ break;
//      case 0x38:/*SC*/     *(long*)ptr=r[rt]; r[rt]=1; break;
      case 0x38:/*SC*/     mem_write(s,4,ptr,r[rt]); r[rt]=1; break;
//      case 0x39:/*SWC1*/ break;
//      case 0x3a:/*SWC2*/ break;
//      case 0x3b:/*SWC3*/ break;
//      case 0x3d:/*SDC1*/ break;
//      case 0x3e:/*SDC2*/ break;
//      case 0x3f:/*SDC3*/ break;
      default: printf("ERROR2\n"); s->wakeup=1;
   }
   s->pc_next+=branch|(lbranch==1)?imm_shift:0;
   s->skip=(lbranch==0);
}

void show_state(State *s)
{
   long i,j;
   for(i=0;i<4;++i) {
      printf("%2.2ld ",i*8);
      for(j=0;j<8;++j) {
         printf("%8.8lx ",s->r[i*8+j]);
      }
      printf("\n");
   }
   printf("%8.8lx %8.8lx %8.8lx %8.8lx\n",s->pc,s->pc_next,s->hi,s->lo);
   j=s->pc;
   for(i=-4;i<=8;++i) {
      printf("%c",i==0?'*':' ');
      s->pc=j+i*4;
      cycle(s,10);
   }
   s->pc=j;
}

void do_debug(State *s)
{
   int ch;
   long i,j=0,watch=0,addr;
   s->pc_next=s->pc+4;
   s->skip=0;
   s->wakeup=0;
   show_state(s);
   for(;;) {
      if(watch) printf("0x%8.8lx=0x%8.8lx\n",watch,mem_read(s,4,watch));
      printf("1=Debug 2=Trace 3=Step 4=BreakPt 5=Go 6=Memory ");
      printf("7=Watch 8=Jump 9=Quit> ");
      ch=getch();
      printf("\n");
      switch(ch) {
      case '1': case 'd': case ' ': cycle(s,0); show_state(s); break;
      case '2': case 't': cycle(s,0); printf("*"); cycle(s,10); break;
      case '3': case 's':
         printf("Count> ");
         scanf("%ld",&j);
         for(i=0;i<j;++i) cycle(s,0);
         show_state(s);
         break;
      case '4': case 'b':
         printf("Line> ");
         scanf("%lx",&j);
         break;
      case '5': case 'g':
         s->wakeup=0;
         while(s->wakeup==0) {
            if(s->pc==j) break;
            cycle(s,0);
         }
         show_state(s);
         break;
      case '6': case 'm':
         printf("Memory> ");
         scanf("%lx",&j);
         for(i=0;i<8;++i) {
            printf("%8.8lx ",mem_read(s,4,j+i*4));
         }
         printf("\n");
         break;
      case '7': case 'w':
         printf("Watch> ");
         scanf("%lx",&watch);
         break;
      case '8': case 'j':
         printf("Jump> ");
         scanf("%lx",&addr);
         s->pc=addr;
         s->pc_next=addr+4;
         show_state(s);
         break;
      case '9': case 'q': return;
      }
   }
}
/************************************************************/

int main(int argc,char *argv[])
{
   State state,*s=&state;
   FILE *in;
   long i,k;
   printf("MIPS emulator\n");
   memset(s,0,sizeof(State));
   s->big_endian=0;
   s->mem=malloc(MEM_SIZE);
   if(argc<=1) {
      printf("   Usage:  mips file.exe\n");
      printf("           mips file.exe B   {for big_endian}\n");
      printf("           mips file.exe DD  {disassemble}\n");
      printf("           mips file.exe BD  {disassemble big_endian}\n");
      return 0;
   }
   in=fopen(argv[1],"rb");
   if(in==NULL) { printf("Can't open file %s!\n",argv[1]); getch(); return(0); }
   i=fread(s->mem,1,MEM_SIZE,in);
   fclose(in);
   printf("Read %ld bytes.\n",i);
   if(argc==3&&argv[2][0]=='B') {
      printf("Big Endian\n");
      s->big_endian=1;
      big_endian=1;
   }
   if(argc==3&&argv[2][0]=='S') {   /*make big endian*/
      printf("Big Endian\n");
      for(k=0;k<i+3;k+=4) {
         *(long*)&s->mem[k]=htonl(*(long*)&s->mem[k]);
      }
      in=fopen("big.exe","wb");
      fwrite(s->mem,i,1,in);
      fclose(in);
      return(0);
   }
   if(argc==3&&argv[2][1]=='D') {   /*dump image*/
      for(k=0;k<i;k+=4) {
         s->pc=k;
         cycle(s,10);
      }
      free(s->mem);
      return(0);
   }
   s->pc=0x0;
   do_debug(s);
   free(s->mem);
   return(0);
}

