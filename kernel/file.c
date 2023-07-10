//
// File descriptors
//

#include <cdefs.h>
#include <defs.h>
#include <fcntl.h>
#include <file.h>
#include <fs.h>
#include <param.h>
#include <proc.h>
#include <sleeplock.h>
#include <spinlock.h>
#include <stat.h>

struct devsw devsw[NDEV];

struct file_info gfiledescriptors[NFILE];
int smallestFd;

int fileopen(char *filepath, int mode) {
  struct inode *newiNode = namei(filepath);
  if (newiNode == NULL) {
    return -1;
  }

  // locki(newiNode);

  if (newiNode->type == T_DIR) {
    unlocki(newiNode);
    return -1;
  }

  struct stat st;
  concurrent_stati(newiNode, &st);

  struct proc *currProc = myproc();

  if (currProc == NULL) {
    return -1;
  }

  int type = newiNode->type;
  if (type == T_FILE && mode != O_RDONLY) {
    return -1;
  }

  int availableSlot = 0;
  int processFD;
  for (processFD = 0; processFD < NOFILE; processFD++) {
    if (currProc->filetable[processFD] == NULL) {
      // found an available slot
      availableSlot = 1;
      break;
    }
  }
  if (availableSlot == 0) {
    // no more valid places for a new file

    return -1;
  }

  // place into global file descriptor
  for (int globalFD = 0; globalFD < NFILE; globalFD++) {
    if (gfiledescriptors[globalFD].ref == 0 ) {
      struct file_info file;
      file.ref = 1;
      file.node = newiNode;
      file.currOffset = 0;
      file.flags = mode;
      gfiledescriptors[globalFD] = file;
      // set processor to point to global file table
      currProc->filetable[processFD] = &gfiledescriptors[globalFD];
      return processFD;
    }
  }

  unlocki(newiNode);
  return -1;
}

int filewrite(int fd, char *buffer, int writebytes) {
  cprintf("in write\n");
  struct proc *currProc = myproc();
  if (currProc == NULL) {
    return -1;
  }

  if (currProc->filetable[fd] == NULL) {
    return -1;
  }

  struct file_info file = *(currProc->filetable[fd]);
  if (file.node == NULL) {
    return -1;
  }

  if (file.flags == O_RDONLY) {
    return -1;
  }

  // write bytes_to_write from the buffer into the fd
  int bytesWritten =
      concurrent_writei(file.node, buffer, file.currOffset, writebytes);
  // update the current position
  if (bytesWritten > 0) {
    currProc->filetable[fd]->currOffset += bytesWritten;
  }

  return bytesWritten;
}

int fileread(int fd, char *buffer, int readbytes) {
  cprintf("in read\n");
  struct proc *currProc = myproc();
  if (currProc == NULL) {
    return -1;
  }

  if (currProc->filetable[fd] == NULL) {
    return -1;
  }

  struct file_info file = *(currProc->filetable[fd]);
  if (file.node == NULL || file.flags == O_WRONLY) {
    return -1;
  }

  int numRead = concurrent_readi(file.node, buffer, file.currOffset, readbytes);
  if (numRead > 0) {
    (currProc->filetable[fd])->currOffset += numRead;
  }

  return numRead;
}

int fileclose(int fd) {
  cprintf("in close\n");
  struct proc *currProc = myproc();
  if (currProc == NULL) {
    return -1;
  }

  if (currProc->filetable[fd] == NULL) {
    return -1;
  }

  struct file_info file = *(currProc->filetable[fd]);
  if (file.node == NULL) {
    return -1;
  }

  if (file.ref == 1) {
    irelease(file.node);
    // only 1 instance - remove completely
    file.currOffset = 0;
    file.flags = 0;
    file.ref = 0;
    file.node = 0;
  } else {
    // multipl;e instances - remove one from the references
    currProc->filetable[fd]->ref--;
  }

  currProc->filetable[fd] = NULL;
  return 0;
}

int filedup(int fd) {
  cprintf("in dup\n");
  struct proc *currProc = myproc();
  if (currProc == NULL) {
    return -1;
  }

  if (currProc->filetable[fd] == NULL) {
    return -1;
  }

  struct file_info file = *(currProc->filetable[fd]);
  if (file.node == NULL) {
    return -1;
  }

  int slot;
  for (slot = 0; slot < NOFILE; slot++) {
    if (currProc->filetable[slot] == NULL) {
      // found the first available slot
      currProc->filetable[slot] = currProc->filetable[fd];
      currProc->filetable[fd]->ref++;
      return slot;
    }
  }

  return -1;
}

int filestat(int fd, struct stat *fstat) {
  cprintf("in stat\n");
  struct proc *currProc = myproc();
  if (currProc->filetable[fd] == NULL) {
    // invalid FD
    return -1;
  }

  struct file_info file = *(currProc->filetable[fd]);
  if (file.node == NULL) {
    // invalid file
    return -1;
  }

  // get the stats
  concurrent_stati(file.node, fstat);
  // return success
  return 0;
}