#ifndef USERPROG_HANDLER_H
#define USERPROG_HANDLER_H

#include "threads/interrupt.h"

/* System call handler functions */
void sys_halt(struct intr_frame *f);
void sys_exit(struct intr_frame *f);
void sys_fork(struct intr_frame *f);
void sys_exec(struct intr_frame *f);
void sys_wait(struct intr_frame *f);
void sys_create(struct intr_frame *f);
void sys_remove(struct intr_frame *f);
void sys_open(struct intr_frame *f);
void sys_filesize(struct intr_frame *f);
void sys_read(struct intr_frame *f);
void sys_write(struct intr_frame *f);
void sys_seek(struct intr_frame *f);
void sys_tell(struct intr_frame *f);
void sys_close(struct intr_frame *f);
void sys_dup2(struct intr_frame *f);

#endif /* userprog/handler.h */
