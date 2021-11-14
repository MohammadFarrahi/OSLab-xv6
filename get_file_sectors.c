#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "fcntl.h"

int main(int argc, char *argv[])
{
   int fd, sector_count, ind, i;
   char buf[2048] = "";
   uint sectors[NINDIRECT+NDIRECT];
   char* file_names[4] = {"text1.txt", "text2.txt", "text3.txt", "text4.txt"};

   for(i = 0; i < 4; i++){
      unlink(file_names[i]);
      fd = open(file_names[i], O_CREATE | O_RDWR);
      for(int i = 0; i < 80; ++i)
         strcpy(buf + i*20, "sectortestsectortest");
      write(fd, buf, strlen(buf));
      close(fd);
      fd = open(file_names[i], O_RDONLY);
    
      sector_count = get_file_sectors(fd, sectors, 128+12);
      printf(2, "sector addresses of file with name = %s: \n", file_names[i]);
      for(ind = 0; ind < sector_count; ind++){
         printf(2, "%x   ", sectors[ind]);
      }
      printf(2, "\n");
      close(fd);
   }
   exit();
}