#include "userprog/handler.h"
#include "filesys/file.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "threads/mmu.h"
#include "devices/input.h"
#include <syscall-nr.h>
#include <string.h>

void
sys_read(struct intr_frame *f) {
    int fd = (int)f->R.rdi;
    void* buffer = (void*)f->R.rsi;
    off_t size = (off_t)f->R.rdx;
    struct thread *cur = thread_current();
    
    if (buffer == NULL || !is_user_vaddr(buffer) || 
        pml4_get_page(cur->pml4, buffer) == NULL || 
        size < 0) {
        f->R.rax = SYS_EXIT;
        f->R.rdi = -1;
        sys_exit(f);
        return;
    }
    
    // stdin 처리
    if (fd == 0) {
        if (cur->fd_stdin == -1) {  // stdin closed
            f->R.rax = -1;
            return;
        }
        for (int i = 0; i < size; i++) {
            ((char*)buffer)[i] = input_getc();
        }
        f->R.rax = size;
        return;
    }
    
    // stdout에서 읽기 시도
    if (fd == 1) {
        f->R.rax = -1;
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
    
    int bytes_read = file_read(file, buffer, size);
    f->R.rax = bytes_read;
}
