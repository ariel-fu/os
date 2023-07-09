
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
struct file_info gftable[NFILE];
int currOffset = 0;

int fileopen(char *path, int mode) {
  /*
 *Finds an open spot in the process open file table and has it point the global
open file table entry .
 * Finds an open entry in the global open file table and allocates a new
file_info struct for the process open file table spot to point to. Will always
open a device, and only open a file if permissions passed are O_RDONLY. Returns
the index into the process open file table as the file descriptor, or -1 on
failure.
 *
 *
 */

  struct inode *node =
      namei(path); // find the inode with the path - increments reference count

  // need to allocate emtpy stat
  struct stat *istat; // TODO Not sure I can create local varible here like this
                      // or I need to allocate some memory
  memset(&istat, 0, sizeof(istat));
  // This function is inspired by thread on Ed :
  // https://us.edstem.org/courses/399/discussion/28068
  if (node == 0)
    return -1;
  concurrent_stati(node, istat);

  if (node->type == T_DIR) {
    unlocki(node);
    return -1;
  }

  // trying to open a file that is not read only
  if (node->type == T_FILE && mode != O_RDONLY)
    return -1;

  int foundSlot = 0;
  // find open slot on process open file table filetable
  int pfd = 0; // process file descriptor index
  struct proc *p = myproc();
  for (pfd = 0; pfd < NOFILE; pfd++) {
    if (p->filetable[pfd] == NULL) { // TODO Not sure how to check is emtpty
      foundSlot = 1;
      break;
    }
  }

  if (foundSlot == 0) {
    //   cprintf("THERE ARE NO MORE OPEN SLOTS IN PROCESS FILE TABLE\n");
    return -1;
  }

  int gfd = 0; // global file descriptor index
  for (gfd = 0; gfd < NFILE; gfd++) {
    if (gftable[gfd].ref == 0) { // check if slot is empty

      // Update gftable[gfd] file_info struct value
      gftable[gfd].ref += 1;
      gftable[gfd].node = node;

      if (node == 0)
        cprintf("why you are zero pointer");
      // gftable[gfd].currOffset =0;//should it be zero?
      gftable[gfd].flags = mode; // TODO Not sure what value should be assign
                                // here

      // Assign pointer to filetable in slot pfd
      p->filetable[pfd] = &gftable[gfd];
      break;
    }
  }

  //  cprintf("%s open in gftable %d and point to global gftable %d with ref %d:
  //  \n",path,pfd,gfd,gftable[gfd].ref);
  return pfd;
}

int filestat(int fd, struct stat *fstat) {
  struct proc *p = myproc();
  // not valid
  if (p->filetable[fd] == NULL)
    return -1;

  struct file_info f = *(p->filetable[fd]);
  if (f.node == NULL)
    return -1;

  concurrent_stati(f.node, fstat);
  return 0;
}

int fileclose(int fd) {

  struct proc *p = myproc();
  if (p->filetable[fd] == NULL)
    return -1;
  struct file_info f = *(p->filetable[fd]);
  if (f.node == NULL)
    return -1;
  // Decrease reference count of file by 1
  // If ref count is 1
  if (f.ref > 1) {
    p->filetable[fd]->ref -= 1;
  } else {
    // cprintf("irelease ===\n\n");
    irelease(f.node);
    // reset everyting
    p->filetable[fd]->node = 0;
    p->filetable[fd]->ref = 0;
    p->filetable[fd]->flags = 0;
    p->filetable[fd]->currOffset = 0;
  }
  //   cprintf("%s close  for fd =%d and ref is %d
  //   \n",f.path,fd,p->filetable[fd]->ref);

  // remove file from current process's file table
  p->filetable[fd] = NULL;
  return 0;
}

int fileread(int fd, char *buf, int bytes_read) {
  struct proc *p = myproc();
  if (p->filetable[fd] == NULL)
    return -1;
  struct file_info f = *(p->filetable[fd]);
  if (f.node == NULL)
    return -1;
  if (f.flags == O_WRONLY)
    return -1;
  // TODO need to change currOffset to gftable's struct in order to avoid multi
  // tread issue
  currOffset = concurrent_readi(f.node, buf, f.currOffset, bytes_read);

  p->filetable[fd]->currOffset += currOffset;
  // cprintf("currOffset right now %d and try to read %d bytes and got %d read
  // \n",p->filetable[fd]->currOffset,bytes_read,currOffset);
  return currOffset;
}

int filewrite(int fd, char *buf, int bytes_written) {

  struct proc *p = myproc();
  if (p->filetable[fd] == NULL)
    return -1;

  struct file_info f = *(p->filetable[fd]);

  if (f.node == NULL)
    return -1;
  // if (f.flags == O_RDONLY)
  //   return -1;

  return concurrent_writei(f.node, buf, f.currOffset, bytes_written);
}

int filedup(int fd) {
  struct proc *p = myproc();
  if (p->filetable[fd] == NULL)
    return -1;
  struct file_info f = *(p->filetable[fd]);
  if (f.node == NULL)
    return -1;

  for (int i = 0; i < NOFILE; i++) {
    if (p->filetable[i] == NULL) {
      p->filetable[i] = p->filetable[fd];
      p->filetable[fd]->ref++; // increase reference count
      // cprintf("dup file:%s from fd = %d to new fd = %d and ref become %d
      // \n",f.path,fd,i,p->filetable[fd]->ref);
      return i;
    }
  }
  return -1; // not available
}

// //
// // File descriptors
// //

// #include <cdefs.h>
// #include <defs.h>
// #include <file.h>
// #include <fs.h>
// #include <param.h>
// #include <sleeplock.h>
// #include <spinlock.h>
// #include <stat.h>
// #include <proc.h>
// #include <fcntl.h>
// #include <fs.h>

// struct devsw devsw[NDEV];

// struct file_info gfiledescriptors[NFILE];
// int smallestFd;

// int fileopen(char* filepath, int mode) {
//     cprintf("MODE: %d\n", mode);
//     struct inode* newiNode = namei(filepath);
//     if(newiNode == NULL) {
//         return -1;
//     }
//     struct stat st;
//     concurrent_stati(newiNode, &st);

//     struct proc* currProc = myproc();
//     if(currProc == NULL) {
//         return -1;
//     }

//     int type = newiNode->type;
//     if(type == T_FILE && mode != O_RDONLY) {
//         cprintf("a non-console file & non-read mode");
//         return -1;
//     }

//     int availableSlot = 0;
//     int processFD;
//     for(processFD = 0; processFD < NOFILE; processFD++) {
//         if(currProc->filetable[processFD] == NULL) {
//             // found an available slot
//             availableSlot = 1;
//             break;
//         }
//     }

//     if(availableSlot == 0) {
//         // no more valid places for a new file
//         return -1;
//     }

//     // place into global file descriptor
//     for(int globalFD = 0; globalFD < NFILE; globalFD++) {
//         if(gfiledescriptors[globalFD].memRefCount == 0) {
//             gfiledescriptors[globalFD].memRefCount++;
//             gfiledescriptors[globalFD].node = newiNode;
//             gfiledescriptors[globalFD].flags = mode;
//             // set processor to point to global file table
//             currProc->filetable[processFD] = &gfiledescriptors[globalFD];
//             break;
//         }
//     }

//     return processFD;
// }

// int filewrite(int fd, char* buffer, int writebytes) {
//     struct proc* currProc = myproc();

//     if(currProc == NULL) {
//         return -1;
//     }

//     if(currProc->filetable[fd] == NULL) {
//         return -1;
//     }

//     struct file_info file = *(currProc->filetable[fd]);
//     if(file.node == NULL) {
//         cprintf("node\n");
//         return -1;
//     }

//     if(file.flags == O_RDONLY) {
//         cprintf("flags\n");
//         return -1;
//     }

//     //write bytes_to_write from the buffer into the fd
//     int bytesWritten = concurrent_writei(file.node, buffer,
//     file.currcurrOffset, writebytes);
//     //update the current position
//     if(bytesWritten != -1){
//         file.currcurrOffset = file.currcurrOffset + bytesWritten;
//     }
//     cprintf("done %d\n", bytesWritten);
//     return bytesWritten;
// }

// int fileread(int fd, char* buffer, int readbytes) {
//     struct proc* currProc = myproc();
//     if(currProc == NULL) {
//         return -1;
//     }

//     if(currProc->filetable[fd] == NULL) {
//         return -1;
//     }

//     struct file_info file = *(currProc->filetable[fd]);
//     if(file.node == NULL || file.flags == O_WRONLY) {
//         return -1;
//     }

//     int numRead = concurrent_readi(file.node, buffer, file.currcurrOffset,
//     readbytes); if(numRead != -1) {
//         file.currcurrOffset += numRead;
//     }
//     return numRead;
// }

// int fileclose(int fd) {
//     struct proc* currProc = myproc();
//     if(currProc == NULL) {
//         return -1;
//     }

//     if(currProc->filetable[fd] == NULL) {
//         return -1;
//     }

//     struct file_info file = *(currProc->filetable[fd]);
//     if(file.node == NULL) {
//         return -1;
//     }

//     if(file.memRefCount == 1) {
//         // only 1 instance - remove completely
//         file.currcurrOffset = 0;
//         file.flags = 0;
//         file.memRefCount = 0;
//         file.node = NULL;
//     } else {
//         // multipl;e instances - remove one from the references
//         file.memRefCount--;
//     }

//     currProc->filetable[fd] = NULL;
//     return 0;

// }

// int filedup(int fd) {
//     struct proc* currProc = myproc();
//     if(currProc == NULL) {
//         return -1;
//     }

//     if(currProc->filetable[fd] == NULL) {
//         return -1;
//     }

//     struct file_info file = *(currProc->filetable[fd]);
//     if(file.node == NULL) {
//         return -1;
//     }

//     int slot;
//     for(slot = 0; slot < NOFILE; slot++) {
//         if(currProc->filetable[slot] == NULL) {
//             // found the first available slot
//             currProc->filetable[slot] = currProc->filetable[fd];
//             currProc->filetable[fd]->memRefCount++;
//             return slot;
//         }
//     }

//     return -1;
// }

// int filestat(int fd, struct stat* fstat) {
//     struct proc* currProc = myproc();
//     if(currProc->filetable[fd] == NULL) {
//         // invalid FD
//         return -1;
//     }

//     struct file_info file = *(currProc->filetable[fd]);
//     if(file.node == NULL) {
//         // invalid file
//         return -1;
//     }

//     // get the stats
//     concurrent_stati(file.node, fstat);
//     // return success
//     return 0;
// }