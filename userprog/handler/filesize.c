#include "userprog/handler.h"
#include "filesys/file.h"
#include "threads/thread.h"
#include <list.h>

void
sys_filesize(struct intr_frame *f) {
    int fd = (int)f->R.rdi;
    struct file *file = NULL;
    struct list *fd_table = &thread_current()->fd_table;
    
    for (struct list_elem *e = list_begin(fd_table); 
         e != list_end(fd_table); 
         e = list_next(e)) {
        struct fd *temp = list_entry(e, struct fd, fd_elem);
        if (temp->fd_num == fd) {
            file = temp->file;
            break;
        }
    }
    
    off_t filesize = file_length(file);
    f->R.rax = filesize;
}
