#include "userprog/handler.h"
#include "filesys/file.h"
#include "threads/thread.h"

void
sys_tell(struct intr_frame *f) {
    int fd = (int)f->R.rdi;
    struct thread *cur = thread_current();
    
    if (fd < 2 || fd >= FD_MAX) {
        f->R.rax = 0;
        return;
    }
    
    struct file *file = cur->fd_table[fd];
    if (file == NULL) {
        f->R.rax = 0;
        return;
    }
    
    unsigned pos = (unsigned)file_tell(file);
    f->R.rax = pos;
}
