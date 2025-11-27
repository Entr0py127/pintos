#include "userprog/handler.h"
#include "filesys/file.h"
#include "filesys/inode.h"
#include "threads/thread.h"
#include <list.h>
#include "threads/malloc.h"

void
sys_close(struct intr_frame *f) {
    int fd_num = (int)f->R.rdi;
    
    for (struct list_elem *e = list_begin(&thread_current()->fd_table); 
         e != list_end(&thread_current()->fd_table); 
         e = list_next(e)) {
        struct fd *FD = list_entry(e, struct fd, fd_elem);
        if (FD->fd_num == fd_num) {
            if (FD->fd_num == 0 || FD->fd_num == 1) {
                if (FD->fd_num == 0)
                    FD->type = 0;
                else if (FD->fd_num == 1)
                    FD->type = 1;
                FD->file = NULL;
            } else {
                if (FD->file != NULL) {
                    ref_count_down(FD->file);
                    if (file_ref_cnt(FD->file) == 0)
                        file_close(FD->file);
                }
                list_remove(e);
                free(FD);
            }
            break;
        }
    }
}
