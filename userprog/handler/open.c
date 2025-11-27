#include "userprog/handler.h"
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "threads/mmu.h"
#include <syscall-nr.h>
#include "threads/malloc.h"

void
sys_open(struct intr_frame *f) {
    char* name = (char*)f->R.rdi;
    
    if (name != NULL && is_user_vaddr(name) && 
        pml4_get_page(thread_current()->pml4, name) != NULL) {
        struct file *fo = filesys_open(name);
        if (fo != NULL) {
            struct thread *cur = thread_current();
            struct fd *fd = (struct fd *)malloc(sizeof(struct fd));
            if (fd == NULL) {
                file_close(fo);
                f->R.rax = -1;
                return;
            }
            fd->file = file_duplicate(fo);
            file_close(fo);
            list_push_back(&cur->fd_table, &fd->fd_elem);
            fd->fd_num = cur->fd_count++;
            fd->type = 2;
            f->R.rax = fd->fd_num;
        } else {
            f->R.rax = -1;
        }
    } else {
        f->R.rax = SYS_EXIT;
        f->R.rdi = -1;
        sys_exit(f);
    }
}
