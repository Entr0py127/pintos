#include "userprog/handler.h"
#include "userprog/process.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "threads/mmu.h"
#include <syscall-nr.h>

void
sys_exec(struct intr_frame *f) {
    const char *f_name = (const char *)f->R.rdi;
    if (f_name != NULL && is_user_vaddr(f_name) && 
        pml4_get_page(thread_current()->pml4, f_name) != NULL) {
        int return_status = process_exec(f_name);
        if (return_status == -1) {
            f->R.rax = SYS_EXIT;
            f->R.rdi = -1;
            sys_exit(f);
        }
    } else {
        f->R.rax = SYS_EXIT;
        f->R.rdi = -1;
        sys_exit(f);
    }
}
