#include "userprog/handler.h"
#include "threads/thread.h"
#include <stdio.h>

void
sys_exit(struct intr_frame *f) {
    char* file_name = thread_current()->name;
    int exit_status = (int)f->R.rdi;
    thread_current()->exit_status = exit_status;
    printf("%s: exit(%d)\n", file_name, exit_status);
    thread_exit();
}
