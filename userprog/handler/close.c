#include "userprog/handler.h"
#include "filesys/file.h"
#include "filesys/inode.h"
#include "threads/thread.h"

void
sys_close(struct intr_frame *f) {
    int fd_num = (int)f->R.rdi;
    struct thread *cur = thread_current();
    
    if (fd_num < 0 || fd_num >= FD_MAX) {
        return;
    }
    
    if (fd_num == 0) {
        cur->fd_stdin = -1;  // stdin closed
        return;
    }
    if (fd_num == 1) {
        cur->fd_stdout = -1;  // stdout closed
        return;
    }
    
    struct file *file = cur->fd_table[fd_num];
    if (file != NULL) {
        ref_count_down(file);
        if (file_ref_cnt(file) == 0) {
            file_close(file);
        }
        cur->fd_table[fd_num] = NULL;
    }
}
