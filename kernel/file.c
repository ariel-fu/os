//
// File descriptors
//

#include <cdefs.h>
#include <defs.h>
#include <file.h>
#include <fs.h>
#include <param.h>
#include <sleeplock.h>
#include <spinlock.h>
#include <stat.h>
#include <proc.h>
#include <fcntl.h>
#include <fs.h>


struct devsw devsw[NDEV];

struct file_info gfiledescriptors[NFILE];
int smallestFd;

int fileopen(char* filepath, int mode) {
    struct inode* newiNode = namei(filepath);
    if(newiNode == NULL) {
        return -1;
    }
    struct stat* st;
    concurrent_stati(newiNode, st);
    
    struct proc* currProc = myproc();
    if(currProc == NULL) {
        return -1;
    }

    int availableSlot = 0;
    int processFD;
    for(processFD = 0; processFD < NOFILE; processFD++) {
        if(currProc->filetable[processFD] == NULL) {
            // found an available slot
            availableSlot = 1;
            break;
        }
    }

    if(availableSlot == 0) {
        // no more valid places for a new file
        return -1;
    }

    // place into global file descriptor
    for(int globalFD = 0; globalFD < NFILE; globalFD++) {
        if(gfiledescriptors[globalFD].memRefCount == 0) {
            gfiledescriptors[globalFD].memRefCount++;
            gfiledescriptors[globalFD].node = newiNode;
            gfiledescriptors[globalFD].flags = mode;
            // set processor to point to global file table
            currProc->filetable[processFD] = &gfiledescriptors[globalFD];
            break;
        }
    }

    return processFD;
}

int filewrite(int fd, char* buffer, int writebytes) {
    struct proc* currProc = myproc();
    if(currProc == NULL) {
        return -1;
    }

    if(currProc->filetable[fd] == NULL) {
        return -1;
    }
    struct file_info file = *(currProc->filetable[fd]);
    if(file.node == NULL || file.flags == O_RDONLY) {
        return -1;
    }

    return concurrent_writei(file.node, buffer, file.currOffset, writebytes);
}

int fileread(int fd, char* buffer, int readbytes) {
    struct proc* currProc = myproc();
    if(currProc == NULL) {
        return -1;
    }

    if(currProc->filetable[fd] == NULL) {
        return -1;
    }

    struct file_info file = *(currProc->filetable[fd]);
    if(file.node == NULL || file.flags == O_WRONLY) {
        return -1;
    }

    int numRead = concurrent_readi(file.node, buffer, file.currOffset, readbytes);
    
    file.currOffset += numRead;

    return numRead;
}

int fileclose(int fd) {
    struct proc* currProc = myproc();
    if(currProc == NULL) {
        return -1;
    }

    if(currProc->filetable[fd] == NULL) {
        return -1;
    }

    struct file_info file = *(currProc->filetable[fd]);
    if(file.node == NULL) {
        return -1;
    }

    if(file.memRefCount == 1) {
        // only 1 instance - remove completely
        file.currOffset = 0;
        file.flags = 0;
        file.memRefCount = 0;
        file.node = NULL;
    } else {
        // multipl;e instances - remove one from the references
        file.memRefCount--;
    }

    currProc->filetable[fd] = NULL;
    return 0;

}

int filedup(int fd) {
    struct proc* currProc = myproc();
    if(currProc == NULL) {
        return -1;
    }
    
    if(currProc->filetable[fd] == NULL) {
        return -1;
    }

    struct file_info file = *(currProc->filetable[fd]);
    if(file.node == NULL) {
        return -1;
    }

    int slot;
    for(slot = 0; slot < NOFILE; slot++) {
        if(currProc->filetable[slot] == NULL) {
            // found the first available slot
            currProc->filetable[slot] = currProc->filetable[fd];
            currProc->filetable[fd]->memRefCount++;
            return slot;
        }
    }

    return -1;
}

int filestat(int fd, struct stat* fstat) {
    struct proc* currProc = myproc();
    if(currProc->filetable[fd] == NULL) {
        // invalid FD
        return -1;
    }

    if(currProc->filetable[fd] == NULL) {
        return -1;
    }

    struct file_info file = *(currProc->filetable[fd]);
    if(file.node == NULL) {
        // invalid file
        return -1;
    }

    // get the stats
    concurrent_stati(file.node, fstat);
    // return success
    return 0;
}