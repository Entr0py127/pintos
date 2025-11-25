#include "userprog/handler.h"
#include "filesys/filesys.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "threads/mmu.h"
#include <syscall-nr.h>

void
sys_create(struct intr_frame *f) {
    char* name = (char*)f->R.rdi;
    off_t initial_size = (off_t)f->R.rsi;
    
    if (name != NULL && is_user_vaddr(name) && 
        pml4_get_page(thread_current()->pml4, name) != NULL && 
        initial_size >= 0) {
        f->R.rax = filesys_create(name, initial_size);
    } else {
        f->R.rax = SYS_EXIT;
        f->R.rdi = -1;
        sys_exit(f);
    }
}
