#include "userprog/handler.h"
#include "filesys/file.h"
#include "threads/thread.h"

void
sys_seek(struct intr_frame *f) {
    int fd = (int)f->R.rdi;
    unsigned new_pos = (unsigned)f->R.rsi;
    struct thread *cur = thread_current();
    
    if (fd < 2 || fd >= FD_MAX) {
        return;
    }
    
    struct file *file = cur->fd_table[fd];
    if (file == NULL) {
        return;
    }
    
    file_seek(file, new_pos);
}
