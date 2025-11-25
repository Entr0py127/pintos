#include "userprog/handler.h"
#include "filesys/file.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "threads/mmu.h"
#include <syscall-nr.h>
#include <stdio.h>
#include <list.h>

void
sys_write(struct intr_frame *f) {
    uint64_t fd = f->R.rdi;
    const char* buffer = (const char*)f->R.rsi;
    size_t size = (size_t)f->R.rdx;
    
    if (buffer == NULL || !is_user_vaddr(buffer) || 
        pml4_get_page(thread_current()->pml4, buffer) == NULL || 
        size < 0) {
        f->R.rax = SYS_EXIT;
        f->R.rdi = -1;
        sys_exit(f);
        return;
    }
    
    struct fd *temp = NULL;
    struct file *file = NULL;
    struct list *fd_table = &thread_current()->fd_table;
    
    for (struct list_elem *e = list_begin(fd_table); 
         e != list_end(fd_table); 
         e = list_next(e)) {
        temp = list_entry(e, struct fd, fd_elem);
        if (temp->fd_num == fd) {
            file = temp->file;
            break;
        }
    }
    
    if (file == NULL) {
        if (fd == STDOUT_FILENO || (temp != NULL && temp->type == STDOUT_FILENO)) {
            putbuf((const char*)buffer, (size_t)size);
            f->R.rax = size;
        } else {
            f->R.rax = SYS_EXIT;
            f->R.rdi = -1;
            sys_exit(f);
        }
    } else {
        if (fd > 2 || temp->type == 2) {
            int bytes_write = file_write(file, buffer, size);
            if (bytes_write == size)
                f->R.rax = bytes_write;
            else
                f->R.rax = 0;
        }
    }
}
