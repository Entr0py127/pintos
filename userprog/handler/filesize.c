#include "userprog/handler.h"
#include "filesys/file.h"
#include "threads/thread.h"

void
sys_filesize(struct intr_frame *f) {
    int fd = (int)f->R.rdi;
    struct thread *cur = thread_current();
    
    if (fd < 2 || fd >= FD_MAX) {
        f->R.rax = -1;
        return;
    }
    
    struct file *file = cur->fd_table[fd];
    if (file == NULL) {
        f->R.rax = -1;
        return;
    }
    
    off_t filesize = file_length(file);
    f->R.rax = filesize;
}
