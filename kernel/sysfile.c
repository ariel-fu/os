//
// File-system system calls.
// Mostly argument checking, since we don't trust
// user code, and calls into file.c and fs.c.
//

#include <cdefs.h>
#include <defs.h>
#include <fcntl.h>
#include <file.h>
#include <fs.h>
#include <mmu.h>
#include <param.h>
#include <proc.h>
#include <sleeplock.h>
#include <spinlock.h>
#include <stat.h>

struct file_info ftable[NFILE];
int sys_dup(void) {
  // LAB1
  int fd;

  if(argint(0,&fd)<0 || argfd(0, &fd) < 0)
    return -1;

  return filedup(fd);

}

int sys_read(void) {
  // LAB1
  int fd;
  char *buf;
  int bytes_read;
  //fd is not a file descriptor open for read
  if(argint(0,&fd)<0||argfd(0, &fd) < 0)
    return -1;


  //Number of bytes to read is not positive
  argint(2, &bytes_read);
  if(bytes_read < 0)
    return -1;

  //Some address between [arg1, arg1+arg2-1] is invalid
  if(argptr(1, &buf, bytes_read) < 0)
    return -1;

  bytes_read=fileread(fd, buf, bytes_read);

  return bytes_read;
}

int sys_write(void) {
  /*
  //you have to change the code in this function.
 ` // Currently it supports printint one character to the screen.

  int n;
  char *p;

  if(argint(2, &n) < 0 || argptr(1, &p, n) < 0 )
    return -1;
  uartputc((int)(*p));
  return 1;
  */

  int bytes_written;
  char *buf;
  int fd;
  //argint : get file descriptor from argument 
  //argfd : check this fd is valid (not exceeding the range)
  //argint : get byte_written from arguemnt
  //argptr : get char need to be written from argument and put in buf
  //argstr : read buff and check it's positive
  if(argint(0,&fd)<0||argfd(0,&fd)<0|| argint(2, &bytes_written) < 0|| 
     argptr(1, &buf, bytes_written) < 0||  argstr(1,&buf)<0)
    return -1;
 // uartputc((int)(*buf));//print it out on console;
  return  filewrite(fd, buf, bytes_written);
  
}

int sys_close(void) {
  // LAB1
  int fd;

  argint(0,&fd);
  //fd is not an open file descriptor
  //check if given fd is valid in global file table

  if(argfd(0, &fd) < 0)
    return -1;

  int res = fileclose( fd);
  return res;
}

int sys_fstat(void) {
  // LAB1
  int fd;
  struct stat *fstat;
  
  //fd is not an open file descriptor
  if(argint(0,&fd)||argfd(0, &fd) < 0)
    return -1;


  //if there is an invalid address between [arg1, arg1+sizeof(fstat)]
  if(argptr(1, (char**)(&fstat), sizeof(fstat)) == -1)
    return -1;

  return filestat(fd, fstat);
}

int sys_open(void) {
  // LAB1

  char *path; //path to the file 
  int mode; // mode got opening the file 
  int fd;

  if(argstr(0, &path) < 0 || argint(1, &mode) < 0)
    return -1;

  //O_CREATE mode not supported for this lab 
  if(mode == O_CREATE) 
    return -1;

  //call appropriate file function
  fd = fileopen(path,mode);
  return fd;

}

int sys_exec(void) {
  // LAB2
  return -1;
}

int sys_pipe(void) {
  // LAB2
  return -1;
}

// //
// // File-system system calls.
// // Mostly argument checking, since we don't trust
// // user code, and calls into file.c and fs.c.
// //

// #include <cdefs.h>
// #include <defs.h>
// #include <fcntl.h>
// #include <file.h>
// #include <fs.h>
// #include <mmu.h>
// #include <param.h>
// #include <proc.h>
// #include <sleeplock.h>
// #include <spinlock.h>
// #include <stat.h>

// int sys_dup(void) {
//   // LAB1
//   int fd;
//   int result = argint(0, &fd);
//   if (result == -1) {
//     // failure
//     return -1;
//   }

//   if (fd > NOFILE || fd < 0) {
//     // invalid FD
//     return -1;
//   }

//   int dupFD = filedup(fd);

//   if (dupFD == -1) {
//     return -1;
//   }

//   return dupFD;
// }

// int sys_read(void) {
//   // LAB1
//   int fd;
//   char *buf;
//   int n;

//   int result = argint(0, &fd);
//   if (result < 0) {
//     return -1;
//   }
//   result = argstr(1, &buf);
//   if (result < 0) {
//     return -1;
//   }

//   result = argint(2, &n);
//   if (result < 0 || n < 0) {
//     // invalid filepath
//     return -1;
//   }

//   int readRes = fileread(fd, buf, n);
//   return readRes;
// }

// int sys_write(void) {
//   cprintf("call write\n");
//   int fd;
//   char *buf;
//   int n;

//   if (argint(0, &fd) < 0 || argint(2, &n) < 0 || argptr(1, &buf, n) < 0 ||
//       argstr(1, &buf) < 0) {
//     return -1;
//   }
//   // int result = argint(0, &fd);
//   // if(result < 0) {
//   //   return -1;
//   // }

//   // cprintf("got fd\n");

//   // result = argstr(1, &buf);
//   // if(result < 0) {
//   //   return -1;
//   // }
//   // cprintf("got buff\n");

//   // result = argint(2, &n);
//   // if(result < 0 || n < 0) {
//   //   return -1;
//   // }

//   // cprintf("got writebytes\n");
//   int writeRes = filewrite(fd, buf, n);
//   // cprintf("got the results %d\n", writeRes);
//   return writeRes;
// }

// int sys_close(void) {
//   // LAB1
//   int fd;
//   int res = argint(0, &fd);
//   if (res < 0) {
//     return -1;
//   }

//   res = fileclose(fd);
//   return res;
// }

// int sys_fstat(void) {
//   // LAB1
//   int fd;
//   struct stat *fstat;

//   if (argint(0, fd) < 0) {
//     return -1;
//   }

//   if (argptr(1, (char **)&fstat, sizeof(fstat)) < 0) {
//     return -1;
//   }

//   return filestat(fd, fstat);
// }

// int sys_open(void) {
//   // LAB1
//   char *filepath;
//   int mode;
//   int result = argint(1, &mode);
//   if (result == -1) {
//     // invalid FD
//     return -1;
//   }

//   if (mode == O_CREATE) {
//     return -1;
//   }

//   result = argstr(0, &filepath);
//   if (result < 0) {
//     // invalid filepath
//     return -1;
//   }

//   int open_res = fileopen(filepath, mode);
//   if (open_res == -1) {
//     return -1;
//   }
//   return open_res;
// }

// int sys_exec(void) {
//   // LAB2
//   return -1;
// }

// int sys_pipe(void) {
//   // LAB2
//   return -1;
// }

// int sys_unlink(void) {
//   // LAB 4
//   return -1;
// }
