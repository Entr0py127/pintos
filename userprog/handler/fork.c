#include "userprog/handler.h"
#include "userprog/process.h"
#include "threads/thread.h"

void
sys_fork(struct intr_frame *f) {
    char* thread_name = (char*)f->R.rdi;
    
    tid_t child_tid = process_fork(thread_name, f);
    if (child_tid == TID_ERROR) {
        f->R.rax = -1;
    } else {
        f->R.rax = child_tid;
    }
}
