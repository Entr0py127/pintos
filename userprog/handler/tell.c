#include "userprog/handler.h"
#include "filesys/file.h"
#include "threads/thread.h"
#include <list.h>

void
sys_tell(struct intr_frame *f) {
    int fd = (int)f->R.rdi;
    
    if (fd < 2) {
        return;
    }
    
    struct file *file = NULL;
    
    for (struct list_elem *e = list_begin(&thread_current()->fd_table); 
         e != list_end(&thread_current()->fd_table); 
         e = list_next(e)) {
        struct fd *FD = list_entry(e, struct fd, fd_elem);
        if (FD->fd_num == fd) {
            file = FD->file;
            break;
        }
    }
    
    if (file == NULL) {
        f->R.rax = 0;
        return;
    }
    
    unsigned pos = (unsigned)file_tell(file);
    f->R.rax = pos;
}
