#include "userprog/handler.h"
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "threads/mmu.h"
#include <syscall-nr.h>

void
sys_open(struct intr_frame *f) {
    char* name = (char*)f->R.rdi;
    
    if (name != NULL && is_user_vaddr(name) && 
        pml4_get_page(thread_current()->pml4, name) != NULL) {
        struct file *fo = filesys_open(name);
        if (fo != NULL) {
            struct thread *cur = thread_current();
            // 빈 fd 슬롯 찾기 (2부터 시작, 0=stdin, 1=stdout)
            int fd_num = -1;
            for (int i = 2; i < FD_MAX; i++) {
                if (cur->fd_table[i] == NULL) {
                    fd_num = i;
                    break;
                }
            }
            if (fd_num == -1) {
                // fd 테이블 가득 참
                file_close(fo);
                f->R.rax = -1;
                return;
            }
            cur->fd_table[fd_num] = file_duplicate(fo);
            file_close(fo);
            f->R.rax = fd_num;
        } else {
            f->R.rax = -1;
        }
    } else {
        f->R.rax = SYS_EXIT;
        f->R.rdi = -1;
        sys_exit(f);
    }
}
