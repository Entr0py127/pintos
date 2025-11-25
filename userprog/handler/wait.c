#include "userprog/handler.h"
#include "userprog/process.h"
#include "threads/thread.h"

void
sys_wait(struct intr_frame *f) {
    tid_t tid = (tid_t)f->R.rdi;
    int status = process_wait(tid);
    f->R.rax = status;
}
