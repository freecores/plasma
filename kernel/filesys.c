/*--------------------------------------------------------------------
 * TITLE: Plasma File System
 * AUTHOR: Steve Rhoads (rhoadss@yahoo.com)
 * DATE CREATED: 4/26/07
 * FILENAME: filesys.c
 * PROJECT: Plasma CPU core
 * COPYRIGHT: Software placed into the public domain by the author.
 *    Software 'as is' without warranty.  Author liable for nothing.
 * DESCRIPTION:
 *    Plasma File System.  Supports RAM, flash, and disk file systems.
 *    Possible call tree:
 *      OS_fclose()
 *        FileFindRecursive()      //find the existing file
 *          FileOpen()             //open root file system
 *          FileFind()             //find the next level of directory
 *            OS_fread()           //read the directory file
 *              BlockRead()        //read blocks of directory
 *                MediaBlockRead() //low level read
 *          FileOpen()             //open next directory
 *        OS_fwrite()              //write file entry into directory
 *        BlockRead()              //flush changes to directory
 *--------------------------------------------------------------------*/
#ifndef WIN32
#include "rtos.h"
#else
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef unsigned int   uint32;
typedef unsigned short uint16;
typedef unsigned char  uint8;
#endif

#define BLOCK_SIZE      512
#define FILE_NAME_SIZE   40
#define FULL_NAME_SIZE  128
#define BLOCK_MALLOC    0x0
#define BLOCK_EOF       0xffffffff

typedef enum {
   FILE_MEDIA_RAM,
   FILE_MEDIA_FLASH,
   FILE_MEDIA_DISK
} OS_MediaType_e;

typedef struct OS_FileEntry_s {
   char name[FILE_NAME_SIZE];
   uint32 blockIndex;       //first block of file
   uint32 modifiedTime;
   uint32 length;
   uint8 isDirectory;
   uint8 attributes;
   uint8 valid;
   uint8 mediaType;
   uint16 blockSize;        //Normally BLOCK_SIZE
} OS_FileEntry_t;

typedef struct OS_Block_s {
   uint32 next;
   uint8 data[4];
} OS_Block_t;

struct OS_FILE_s {
   OS_FileEntry_t fileEntry;  //written to directory upon OS_fclose()
   uint8 fileModified;
   uint8 blockModified;
   uint32 blockIndex;         //index of block
   uint32 blockOffset;        //byte offset into block
   uint32 fileOffset;         //byte offset into file
   char fullname[FULL_NAME_SIZE]; //includes full path
   OS_Block_t *block;
   OS_Block_t *blockLocal;    //local copy for flash or disk file system
};

static OS_FileEntry_t rootFileEntry;

// Public prototypes
#ifndef _FILESYS_
typedef struct OS_FILE_s OS_FILE;
#endif
OS_FILE *OS_fopen(char *name, char *mode);
void OS_fclose(OS_FILE *file);
int OS_fread(void *buffer, int size, int count, OS_FILE *file);
int OS_fwrite(void *buffer, int size, int count, OS_FILE *file);
int OS_fseek(OS_FILE *file, int offset, int mode);
int OS_fmkdir(char *name);
int OS_fdir(OS_FILE *dir, char name[64]);
void OS_fdelete(char *name);


/***************** Media Functions Start ***********************/
static uint32 MediaBlockMalloc(OS_FILE *file)
{
   if(file->fileEntry.mediaType == FILE_MEDIA_RAM)
      return (uint32)malloc(file->fileEntry.blockSize);
   else
      return (uint32)malloc(file->fileEntry.blockSize);  //TODO
}


static void MediaBlockFree(OS_FILE *file, uint32 blockIndex)
{
   if(file->fileEntry.mediaType == FILE_MEDIA_RAM)
      free((void*)blockIndex);
   else
      free((void*)blockIndex);  //TODO
}


static void MediaBlockRead(OS_FILE *file, uint32 blockIndex)
{
   if(file->fileEntry.mediaType == FILE_MEDIA_RAM)
      file->block = (OS_Block_t*)blockIndex;
   else
   {
      if(file->blockLocal == NULL)
         file->blockLocal = (OS_Block_t*)malloc(file->fileEntry.blockSize);
      file->block = file->blockLocal;
      memcpy(file->block, (void*)blockIndex, file->fileEntry.blockSize); //TODO
   }
}


static void MediaBlockWrite(OS_FILE *file)
{
   if(file->fileEntry.mediaType != FILE_MEDIA_RAM)
      memcpy((void*)file->blockIndex, file->block, file->fileEntry.blockSize); //TODO
}

/***************** Media Functions End *************************/

// Get the next block and write the old block if it was modified
static void BlockRead(OS_FILE *file, uint32 blockIndex)
{
   uint32 blockIndexSave = blockIndex;
   if(blockIndex == BLOCK_MALLOC)
   {
      // Get a new block
      blockIndex = MediaBlockMalloc(file);
      if(blockIndex == 0)
         blockIndex = BLOCK_EOF;
      if(file->block)
      {
         // Set next pointer in previous block
         file->block->next = blockIndex;
         file->blockModified = 1;
      }
   }
   if(file->block && file->blockModified)
   {
      // Write block back to flash or disk
      MediaBlockWrite(file);
      file->blockModified = 0;
   }
   if(blockIndex == BLOCK_EOF)
      return;
   file->blockIndex = blockIndex;
   file->blockOffset = 0;
   MediaBlockRead(file, blockIndex);
   if(blockIndexSave == BLOCK_MALLOC)
   {
      memset(file->block, 0xff, file->fileEntry.blockSize);
      file->blockModified = 1;
   }
}


int OS_fread(void *buffer, int size, int count, OS_FILE *file)
{
   int items, bytes;
   uint8 *buf = (uint8*)buffer;

   for(items = 0; items < count; ++items)
   {
      for(bytes = 0; bytes < size; ++bytes)
      {
         if(file->fileOffset >= file->fileEntry.length && 
            file->fileEntry.isDirectory == 0)
            return items;
         if(file->blockOffset >= file->fileEntry.blockSize - sizeof(uint32))
         {
            if(file->block->next == BLOCK_EOF)
               return items;
            BlockRead(file, file->block->next);
         }
         *buf++ = file->block->data[file->blockOffset++];
         ++file->fileOffset;
      }
   }
   return items;
}


int OS_fwrite(void *buffer, int size, int count, OS_FILE *file)
{
   int items, bytes;
   uint8 *buf = (uint8*)buffer;

   file->blockModified = 1;
   for(items = 0; items < count; ++items)
   {
      for(bytes = 0; bytes < size; ++bytes)
      {
         if(file->blockOffset >= file->fileEntry.blockSize - sizeof(uint32))
         {
            if(file->block->next == BLOCK_EOF)
               file->block->next = BLOCK_MALLOC;
            BlockRead(file, file->block->next);
            if(file->blockIndex == BLOCK_EOF)
            {
               count = 0;
               --items;
               break;
            }
            file->blockModified = 1;
         }
         file->block->data[file->blockOffset++] = *buf++;
         ++file->fileOffset;
      }
   }
   file->blockModified = 1;
   file->fileModified = 1;
   if(file->fileOffset > file->fileEntry.length)
      file->fileEntry.length = file->fileOffset;
   return items;
}


int OS_fseek(OS_FILE *file, int offset, int mode)
{
   if(mode == 1)      //SEEK_CUR
      offset += file->fileOffset;
   else if(mode == 2) //SEEK_END
      offset += file->fileEntry.length;
   file->fileOffset = offset;
   BlockRead(file, file->fileEntry.blockIndex);
   while(offset > (int)file->fileEntry.blockSize - (int)sizeof(uint32))
   {
      BlockRead(file, file->block->next);
      offset -= file->fileEntry.blockSize - (int)sizeof(uint32);
   }
   file->blockOffset = offset;
   return 0;
}


static int FileOpen(OS_FILE *file, char *name, OS_FileEntry_t *fileEntry)
{
   memset(file, 0, sizeof(OS_FILE));
   if(fileEntry == NULL)
   {
      // Open root file
      memcpy(&file->fileEntry, &rootFileEntry, sizeof(OS_FileEntry_t));
   }
   else if(fileEntry->valid == 1)
   {
      // Open existing file
      memcpy(&file->fileEntry, fileEntry, sizeof(OS_FileEntry_t));
   }
   else
   {
      // Initialize new file
      file->fileModified = 1;
      file->blockModified = 1;
      memset(&file->fileEntry, 0, sizeof(OS_FileEntry_t));
      file->fileEntry.isDirectory = 0;
      file->fileEntry.length = 0;
      strncpy(file->fileEntry.name, name, FILE_NAME_SIZE-1);
      file->fileEntry.blockIndex = 0;
      file->fileEntry.valid = 1;
      file->fileEntry.blockSize = fileEntry->blockSize;
      file->fileEntry.mediaType = fileEntry->mediaType;
   } 
   BlockRead(file, file->fileEntry.blockIndex);    //Get first block
   file->fileEntry.blockIndex = file->blockIndex;
   file->fileOffset = 0;
   if(file->blockIndex == BLOCK_EOF)
      return -1;
   return 0;
}


static int FileFind(OS_FILE *directory, char *name, OS_FileEntry_t *fileEntry)
{
   int count, rc = -1;
   uint32 blockIndex, blockOffset;
   uint32 blockIndexEmpty=BLOCK_EOF, blockOffsetEmpty=0;

   // Loop through files in directory
   for(;;)
   {
      blockIndex = directory->blockIndex;
      blockOffset = directory->blockOffset;
      count = OS_fread(fileEntry, sizeof(OS_FileEntry_t), 1, directory);
      if(count == 0 || fileEntry->blockIndex == BLOCK_EOF)
         break;
      if(fileEntry->valid == 1 && strcmp(fileEntry->name, name) == 0)
      {
         rc = 0;  //Found the file in the directory
         break;
      }
      if(fileEntry->valid != 1 && blockIndexEmpty == BLOCK_EOF)
      {
         blockIndexEmpty = blockIndex;
         blockOffsetEmpty = blockOffset;
      }
   }
   if(rc == 0 || directory->fileEntry.mediaType == FILE_MEDIA_FLASH || 
      blockIndexEmpty == BLOCK_EOF)
   {
      // Backup to start of fileEntry or last entry in directory
      if(directory->blockIndex != blockIndex)
         BlockRead(directory, blockIndex);
      directory->blockOffset = blockOffset;
   }
   else
   {
      // Backup to empty slot
      if(directory->blockIndex != blockIndexEmpty)
         BlockRead(directory, blockIndexEmpty);
      directory->blockOffset = blockOffsetEmpty;
   }
   return rc;
}


static int FileFindRecursive(OS_FILE *directory, char *name, 
                             OS_FileEntry_t *fileEntry, char *filename)
{
   int rc, length;

   rc = FileOpen(directory, NULL, NULL);            //Open root directory
   for(;;)
   {
      if(name[0] == '/')
         ++name;
      for(length = 0; length < FILE_NAME_SIZE; ++length)
      {
         if(name[length] == 0 || name[length] == '/')
            break;
         filename[length] = name[length];
      }
      filename[length] = 0;
      rc = FileFind(directory, filename, fileEntry);  //Find file
      if(rc)
      {
         // File not found
         fileEntry->mediaType = directory->fileEntry.mediaType;
         fileEntry->blockSize = directory->fileEntry.blockSize;
         if(strstr(name, "/") == NULL)
            return rc;
         else
            return -2;  //can't find parent directory
      }
      name += length;
      if(name[0])
         rc = FileOpen(directory, filename, fileEntry);  //Open subdir
      else
         break;
   }
   return rc;
}


OS_FILE *OS_fopen(char *name, char *mode)
{
   OS_FILE *file;
   OS_FileEntry_t fileEntry;
   OS_FILE dir;
   char filename[FILE_NAME_SIZE];  //Name without directories
   int rc;

   if(rootFileEntry.blockIndex == 0)
   {
      // Mount file system
      memset(&dir, 0, sizeof(OS_FILE));
      dir.fileEntry.blockSize = BLOCK_SIZE;
      //dir.fileEntry.mediaType = FILE_MEDIA_FLASH;  //Test flash
      BlockRead(&dir, BLOCK_MALLOC);
      strcpy(rootFileEntry.name, "/");
      rootFileEntry.mediaType = dir.fileEntry.mediaType;
      rootFileEntry.blockIndex = dir.blockIndex;
      rootFileEntry.blockSize = dir.fileEntry.blockSize;
      rootFileEntry.isDirectory = 1;
      BlockRead(&dir, BLOCK_EOF);    //Flush data
   }

   file = (OS_FILE*)malloc(sizeof(OS_FILE));
   if(file == NULL)
      return NULL;
   if(strcmp(name, "/") == 0)
   {
      FileOpen(file, NULL, NULL);
      return file;
   }
   if(strcmp(mode, "w") == 0)
      OS_fdelete(name);
   rc = FileFindRecursive(&dir, name, &fileEntry, filename);
   if(dir.blockLocal)
      free(dir.blockLocal);
   if(rc == -2 || (rc && mode[0] == 'r'))
   {
      free(file);
      return NULL;
   }
   rc = FileOpen(file, filename, &fileEntry);  //Open file
   file->fullname[0] = 0;
   strncat(file->fullname, name, FULL_NAME_SIZE);
   return file;
}


void OS_fclose(OS_FILE *file)
{
   OS_FileEntry_t fileEntry;
   OS_FILE dir;
   char filename[FILE_NAME_SIZE];
   int rc;

   if(file->fileModified)
   {
      // Write file->fileEntry into parent directory
      BlockRead(file, BLOCK_EOF);
      rc = FileFindRecursive(&dir, file->fullname, &fileEntry, filename);
      if(file->fileEntry.mediaType == FILE_MEDIA_FLASH && rc == 0)
      {
         // Invalidate old entry and add new entry at the end
         fileEntry.valid = 0;
         OS_fwrite(&fileEntry, sizeof(OS_FileEntry_t), 1, &dir);
         FileFind(&dir, "endoffile", &fileEntry);
      }
      OS_fwrite(&file->fileEntry, sizeof(OS_FileEntry_t), 1, &dir);
      BlockRead(&dir, BLOCK_EOF);  //flush data
      if(dir.blockLocal)
         free(dir.blockLocal);
   }
   if(file->blockLocal)
      free(file->blockLocal);
   free(file);
}


int OS_fmkdir(char *name)
{
   OS_FILE *file;
   file = OS_fopen(name, "w+");
   if(file == NULL)
      return -1;
   file->fileEntry.isDirectory = 1;
   OS_fclose(file);
   return 0;
}


void OS_fdelete(char *name)
{
   OS_FILE dir, file;
   OS_FileEntry_t fileEntry;
   int rc;
   uint32 blockIndex;
   char filename[FILE_NAME_SIZE];  //Name without directories

   rc = FileFindRecursive(&dir, name, &fileEntry, filename);
   if(rc == 0)
   {
      FileOpen(&file, NULL, &fileEntry);
      for(blockIndex = file.blockIndex; file.block->next != BLOCK_EOF; blockIndex = file.blockIndex)
      {
         BlockRead(&file, file.block->next);
         MediaBlockFree(&file, blockIndex);
      }
      MediaBlockFree(&file, blockIndex);
      fileEntry.valid = 0;
      OS_fwrite((char*)&fileEntry, sizeof(OS_FileEntry_t), 1, &dir);
      BlockRead(&dir, BLOCK_EOF);
      if(file.blockLocal)
         free(file.blockLocal);
   }
   if(dir.blockLocal)
      free(dir.blockLocal);
}


int OS_fdir(OS_FILE *dir, char name[64])
{
   OS_FileEntry_t *fileEntry = (OS_FileEntry_t*)name;
   int count;
   for(;;)
   {
      count = OS_fread(fileEntry, sizeof(OS_FileEntry_t), 1, dir);
      if(count == 0 || fileEntry->blockIndex == BLOCK_EOF)
         return -1;
      if(fileEntry->valid == 1)
         break;
   }
   return 0;
}

/*************************************************/
#define TEST_FILES
#ifdef TEST_FILES
int DirRecursive(char *name)
{
   OS_FileEntry_t fileEntry;
   OS_FILE *dir;
   char fullname[FULL_NAME_SIZE];
   int rc;

   dir = OS_fopen(name, "r");
   for(;;)
   {
      rc = OS_fdir(dir, (char*)&fileEntry);
      if(rc)
         break;
      printf("%s %d\n", fileEntry.name, fileEntry.length);
      if(fileEntry.isDirectory)
      {
         if(strcmp(name, "/") == 0)
            sprintf(fullname, "/%s", fileEntry.name);
         else
            sprintf(fullname, "%s/%s", name, fileEntry.name);
         DirRecursive(fullname);
      }
   }
   OS_fclose(dir);
   return 0;
}

int OS_ftest(void)
{
   OS_FILE *file;
   char *buf;
   int count;
   int i, j;

   buf = (char*)malloc(5000);
   memset(buf, 0, 5000);
   for(count = 0; count < 4000; ++count)
      buf[count] = (char)('A' + (count % 26));
   OS_fmkdir("dir");
   OS_fmkdir("/dir/subdir");
   file = OS_fopen("/dir/subdir/test.txt", "w");
   count = OS_fwrite(buf, 1, 4000, file);
   OS_fclose(file);
   memset(buf, 0, 5000);
   file = OS_fopen("/dir/subdir/test.txt", "r");
   count = OS_fread(buf, 1, 5000, file);
   OS_fclose(file);
   printf("(%s)\n", buf);

   DirRecursive("/");

   for(i = 0; i < 5; ++i)
   {
      sprintf(buf, "/dir%d", i);
      OS_fmkdir(buf);
      for(j = 0; j < 5; ++j)
      {
         sprintf(buf, "/dir%d/file%d%d", i, i, j);
         file = OS_fopen(buf, "w");
         sprintf(buf, "i=%d j=%d", i, j);
         OS_fwrite(buf, 1, 8, file);
         OS_fclose(file);
      }
   }

   OS_fdelete("/dir1/file12");
   DirRecursive("/");
   file = OS_fopen("/baddir/myfile.txt", "w");
   if(file)
      printf("ERROR!\n");

   for(i = 0; i < 5; ++i)
   {
      for(j = 0; j < 5; ++j)
      {
         sprintf(buf, "/dir%d/file%d%d", i, i, j);
         file = OS_fopen(buf, "r");
         if(file)
         {
            count = OS_fread(buf, 1, 500, file);
            printf("i=%d j=%d count=%d (%s)\n", i, j, count, buf);
            OS_fclose(file);
         }
      }
   }

   OS_fdelete("/dir/subdir/test.txt");
   OS_fdelete("/dir/subdir");
   OS_fdelete("/dir");
   for(i = 0; i < 5; ++i)
   {
      for(j = 0; j < 5; ++j)
      {
         sprintf(buf, "/dir%d/file%d%d", i, i, j);
         OS_fdelete(buf);
      }
      sprintf(buf, "/dir%d", i);
      OS_fdelete(buf);
   }

   DirRecursive("/");

   free(buf);
   return 0;
}
#endif  //TEST_FILES
