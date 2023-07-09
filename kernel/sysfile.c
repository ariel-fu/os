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


int sys_dup(void) {
  // LAB1
  int fd;
  int result = argint(0, &fd);
  if(result == -1) {
    // failure
    return -1;
  }

  if(fd > NOFILE || fd < 0) {
    // invalid FD
    return -1;
  }

  int dupFD = filedup(fd);

  if(dupFD == -1) {
    return -1;
  }

  return dupFD;
}

int sys_read(void) {
  // LAB1
  int fd;
  char *buf;
  int n;

  int result = argint(0, &fd);
  if(result < 0) {
    return -1;
  }
  result = argstr(1, &buf);
  if(result < 0) {
    return -1;
  }

  result = argint(2, &n);
  if(result < 0) {
    // invalid filepath
    return -1;
  }

  int readRes = fileread(fd, buf, n);
  return readRes;
}

int sys_write(void) {
  // you have to change the code in this function.
  // Currently it supports printing one character to the screen.
  int n;
  char *p;

  if (argint(2, &n) < 0 || argptr(1, &p, n) < 0)
    return -1;
  uartputc((int)(*p));
  return 1;
}

int sys_close(void) {
  // LAB1
  int fd;
  int res = argint(0, &fd);
  if(res < 0) {
    return -1;
  }

  res = fileclose(fd);
  return res;
}

int sys_fstat(void) {
  // LAB1
  int fd;
  struct stat *fstat;

  if(argint(0, fd) < 0) {
    return -1;
  }

  if(argptr(1, (char**)&fstat, sizeof(fstat)) < 0) {
    return -1;
  }

  return filestat(fd, fstat);
}

int sys_open(void) {
  // LAB1
  char *filepath;
  int mode;
  int result = argint(1, &mode);
  if(result == -1) {
    // invalid FD
    return -1;
  }

   if (mode == O_CREATE) {
    return -1;
  }

  result = argstr(0, &filepath);
  if(result < 0) {
    // invalid filepath
    return -1;
  }

  int open_res = fileopen(filepath, mode);
  if(open_res == -1) {
    return -1;
  }
  return open_res;
}

int sys_exec(void) {
  // LAB2
  return -1;
}

int sys_pipe(void) {
  // LAB2
  return -1;
}

int sys_unlink(void) {
  // LAB 4
  return -1;
}
