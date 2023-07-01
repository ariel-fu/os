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

struct devsw devsw[NDEV];

struct file_info gfiledescriptors[NFILE];
int smallestFd;

int fileopen(char* filepath, int mode) {
    struct inode* newiNode = namei(filepath);
    
    return -1;
}

int filewrite(int fd, char* buffer, int writebytes) {
    return -1;
}

int fileread(int fd, char* buffer, int readbytes) {
    return -1;
}

int fileclose(int fd) {
    return -1;
}

int filedup(int fd) {
    return -1;
}

int filestat(int fd, struct stat*) {
    return -1;
}