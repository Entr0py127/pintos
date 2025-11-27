#include "userprog/handler.h"
#include "filesys/file.h"
#include "filesys/inode.h"
#include "threads/thread.h"
#include <list.h>
#include "threads/malloc.h"

void
sys_dup2(struct intr_frame *f) {
    int oldfd_num = (int)f->R.rdi;
    int newfd_num = (int)f->R.rsi;
    
    if (oldfd_num < 0 || newfd_num < 0) {
        f->R.rax = -1;
        return;
    }
    
    if (oldfd_num == newfd_num) {
        f->R.rax = newfd_num;
        return;
    }
    
    struct fd *oldFD = NULL;
    struct fd *newFD = NULL;
    
    for (struct list_elem *e = list_begin(&thread_current()->fd_table); 
         e != list_end(&thread_current()->fd_table); 
         e = list_next(e)) {
        struct fd *FD = list_entry(e, struct fd, fd_elem);
        if (FD->fd_num == oldfd_num)
            oldFD = FD;
        if (FD->fd_num == newfd_num)
            newFD = FD;
        if (oldFD != NULL && newFD != NULL)
            break;
    }
    
    if (oldfd_num >= 2) {
        if (oldFD == NULL || (oldFD->file == NULL && oldFD->type == 2)) {
            f->R.rax = -1;
            return;
        }
    }
    
    if (newFD == NULL) {
        newFD = (struct fd *)malloc(sizeof(struct fd));
        if (newFD == NULL) {
            f->R.rax = -1;
            return;
        }
        list_push_back(&thread_current()->fd_table, &newFD->fd_elem);
        newFD->fd_num = newfd_num;
    } else if (newFD->file != NULL) {
        ref_count_down(newFD->file);
        if (file_ref_cnt(newFD->file) == 0) {
            file_close(newFD->file);
        }
    }
    
    if (oldfd_num == 0) {
        newFD->type = 0; //stdin
        newFD->file = NULL;
    } else if (oldfd_num == 1) {
        newFD->type = 1; //stdout
        newFD->file = NULL;
    } else if (oldfd_num > 1) {
        newFD->file = oldFD->file;
        newFD->type = oldFD->type;
        ref_count_up(newFD->file);
    }
    
    f->R.rax = newfd_num;
}
