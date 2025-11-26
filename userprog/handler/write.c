#include "userprog/handler.h"
#include "filesys/file.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "threads/mmu.h"
#include <syscall-nr.h>
#include <stdio.h>

void
sys_write(struct intr_frame *f) {
    uint64_t fd = f->R.rdi;
    const char* buffer = (const char*)f->R.rsi;
    size_t size = (size_t)f->R.rdx;
    struct thread *cur = thread_current();
    
    if (buffer == NULL || !is_user_vaddr(buffer) || 
        pml4_get_page(cur->pml4, buffer) == NULL || 
        size < 0) {
        f->R.rax = SYS_EXIT;
        f->R.rdi = -1;
        sys_exit(f);
        return;
    }
    
    // stdin에 쓰기 시도
    if (fd == 0) {
        f->R.rax = -1;
        return;
    }
    
    // stdout 처리
    if (fd == 1) {
        if (cur->fd_stdout == -1) {  // stdout closed
            f->R.rax = -1;
            return;
        }
        putbuf(buffer, size);
        f->R.rax = size;
        return;
    }
    
    // 일반 파일
    if (fd < 2 || fd >= FD_MAX) {
        f->R.rax = -1;
        return;
    }
    
    struct file *file = cur->fd_table[fd];
    if (file == NULL) {
        f->R.rax = -1;
        return;
    }
    
    int bytes_write = file_write(file, buffer, size);
    f->R.rax = bytes_write;
}
