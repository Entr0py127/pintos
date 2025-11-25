#include "userprog/handler.h"
#include "filesys/file.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "threads/mmu.h"
#include "devices/input.h"
#include <syscall-nr.h>
#include <string.h>
#include <list.h>

void
sys_read(struct intr_frame *f) {
    int fd = (int)f->R.rdi;
    void* buffer = (void*)f->R.rsi;
    off_t size = (off_t)f->R.rdx;
    
    if (buffer == NULL || !is_user_vaddr(buffer) || 
        pml4_get_page(thread_current()->pml4, buffer) == NULL || 
        size < 0) {
        f->R.rax = SYS_EXIT;
        f->R.rdi = -1;
        sys_exit(f);
        return;
    }
    
    struct file *file = NULL;
    struct fd *temp = NULL;
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
        if (fd == STDIN_FILENO || (temp != NULL && temp->type == STDIN_FILENO)) {
            for (int i = 0; i < size; i++) {
                memcpy(buffer, (void*)input_getc(), sizeof(char));
                buffer += sizeof(char);
            }
            f->R.rax = size;
        } else {
            f->R.rax = SYS_EXIT;
            f->R.rdi = -1;
            sys_exit(f);
        }
    } else {
        if (fd > 2 || temp->type == 2) {
            int bytes_read = file_read(file, buffer, size);
            f->R.rax = bytes_read;
        }
    }
}
