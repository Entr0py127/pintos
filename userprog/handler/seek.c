#include "userprog/handler.h"
#include "filesys/file.h"
#include "threads/thread.h"
#include <list.h>

void
sys_seek(struct intr_frame *f) {
    int fd = (int)f->R.rdi;
    unsigned new_pos = (unsigned)f->R.rsi;
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
        return;
    }
    
    file_seek(file, new_pos);
}
