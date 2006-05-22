/*--------------------------------------------------------------------
 * TITLE: Plasma TCP/IP HTTP Server
 * AUTHOR: Steve Rhoads (rhoadss@yahoo.com)
 * DATE CREATED: 4/22/06
 * FILENAME: http.c
 * PROJECT: Plasma CPU core
 * COPYRIGHT: Software placed into the public domain by the author.
 *    Software 'as is' without warranty.  Author liable for nothing.
 * DESCRIPTION:
 *    Plasma TCP/IP HTTP Server
 *--------------------------------------------------------------------*/
#ifdef WIN32
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#define _LIBC
#endif
#include "rtos.h"
#include "tcpip.h"
#ifdef WIN32
#define UartPrintf printf
#endif

const char pageGif[]=
{
   "HTTP/1.0 200 OK\r\n"
   "Content-Type: binary/gif\r\n\r\n"
};
const char pageBinary[]=
{
   "HTTP/1.0 200 OK\r\n"
   "Content-Type: binary/binary\r\n\r\n"
};
const char pageHtml[]={
   "HTTP/1.0 200 OK\r\n"
   "Content-Type: text/html\r\n\r\n"
};
const char pageText[]={
   "HTTP/1.0 200 OK\r\n"
   "Content-Type: text/text\r\n\r\n"
};
const char pageEmpty[]=
{
   "HTTP/1.0 404 OK\r\n"
   "Content-Type: text/html\r\n\r\n"
};

static const PageEntry_t *HtmlPages;
static int HtmlFiles;


void HttpServer(IPSocket *socket)
{
   uint8 buf[600];
   int bytes, i, length, len, needFooter;
   char *name=NULL, *page=NULL;

   if(socket == NULL)
      return;
   bytes = IPRead(socket, buf, sizeof(buf)-1);
   if(bytes)
   {
      buf[bytes] = 0;
      if(strncmp((char*)buf, "GET /", 5) == 0)
      {
         for(i = 0; ; ++i)
         {
            length = HtmlPages[i].length;
            if(length == -1)
               break;
            name = (char*)HtmlPages[i].name;
            page = (char*)HtmlPages[i].page;
            len = (int)strlen(name);
            if(strncmp((char*)buf+4, name, len) == 0)
               break;
         }
#ifdef WIN32
         if(length == -1 && HtmlFiles)
         {
            FILE *file;
            char *ptr;

            name = (char*)buf + 5;
            ptr = strstr(name, " ");
            if(ptr)
               *ptr = 0;
            file = fopen(name, "rb");
            if(file)
            {
               page = (char*)malloc(1024*1024*8);
               length = (int)fread(page, 1, 1024*1024*8, file);
               fclose(file);
            }
         }
#endif
         if(length != HTML_LENGTH_LIST_END)
         {
            if(length == HTML_LENGTH_CALLBACK)
            {
               IPFuncPtr funcPtr = (IPFuncPtr)page;
               funcPtr(socket, buf, bytes);
               return;
            }
            if(length == 0)
               length = (int)strlen(page);
            needFooter = 0;
            if(strstr(name, ".html"))
               IPWrite(socket, (uint8*)pageHtml, (int)strlen(pageHtml));
            else if(strstr(name, ".htm") || strcmp(name, "/ ") == 0)
            {
               IPWrite(socket, (uint8*)HtmlPages[0].page, (int)strlen(HtmlPages[0].page));
               needFooter = 1;
            }
            else if(strstr(HtmlPages[i].name, ".gif"))
               IPWrite(socket, (uint8*)pageGif, (int)strlen(pageGif));
            else
               IPWrite(socket, (uint8*)pageBinary, (int)strlen(pageBinary));
            IPWrite(socket, (uint8*)page, length);
            if(needFooter)
               IPWrite(socket, (uint8*)HtmlPages[1].page, (int)strlen(HtmlPages[1].page));
#ifdef WIN32
            if(page != HtmlPages[i].page)
               free(page);
#endif
         }
         else
         {
            IPWrite(socket, (uint8*)pageEmpty, (int)strlen(pageEmpty));
         }
         IPClose(socket);
      }
   }
}


void HttpInit(const PageEntry_t *Pages, int UseFiles)
{
   HtmlPages = Pages;
   HtmlFiles = UseFiles;
   IPOpen(IP_MODE_TCP, 0, 80, HttpServer);
   IPOpen(IP_MODE_TCP, 0, 8080, HttpServer);
}


#if 0
//Example test code
static void MyProg(IPSocket *socket, char *request, int bytes)
{
   char *text="HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n"
              "<html><body>Hello World!</body></html>";
   (void)request; (void)bytes;
   IPWrite(socket, text, (int)strlen(text));
   IPClose(socket);
}
static const PageEntry_t pageEntry[]=
{  //name, length, htmlText
   {"/Header", 0, "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n"
                  "<HTML><HEAD><TITLE>Plasma CPU</TITLE></HEAD>\n<BODY>"},
   {"/Footer", 0, "</BODY></HTML>"},
   {"/ ", 0, "<h2>Home Page</h2>Welcome!  <a href='/other.htm'>Other</a>"
             " <a href='/cgi/myprog'>myprog</a>"},
   {"/other.htm ", 0, "<h2>Other</h2>Other."},
   //{"/binary/plasma.gif ", 1945, PlasmaGif},
   {"/cgi/myprog", HTML_LENGTH_CALLBACK, (char*)MyProg},
   {"", HTML_LENGTH_LIST_END, NULL}
};
void HttpTest(void)
{
   HttpInit(pageEntry, 0);
}
#endif


