#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/loader.h"
#include "userprog/gdt.h"
#include "threads/flags.h"
#include "intrinsic.h"
#include "userprog/process.h"
#include "threads/synch.h"
#include "devices/timer.h"

void syscall_entry (void);
void syscall_handler (struct intr_frame *);

/* System call.
 *
 * Previously system call services was handled by the interrupt handler
 * (e.g. int 0x80 in linux). However, in x86-64, the manufacturer supplies
 * efficient path for requesting the system call, the `syscall` instruction.
 *
 * The syscall instruction works by reading the values from the the Model
 * Specific Register (MSR). For the details, see the manual. */

#define MSR_STAR 0xc0000081         /* Segment selector msr */
#define MSR_LSTAR 0xc0000082        /* Long mode SYSCALL target */
#define MSR_SYSCALL_MASK 0xc0000084 /* Mask for the eflags */

void
syscall_init (void) {
	write_msr(MSR_STAR, ((uint64_t)SEL_UCSEG - 0x10) << 48  |
			((uint64_t)SEL_KCSEG) << 32);
	write_msr(MSR_LSTAR, (uint64_t) syscall_entry);

	/* The interrupt service rountine should not serve any interrupts
	 * until the syscall_entry swaps the userland stack to the kernel
	 * mode stack. Therefore, we masked the FLAG_FL. */
	write_msr(MSR_SYSCALL_MASK,
			FLAG_IF | FLAG_TF | FLAG_DF | FLAG_IOPL | FLAG_AC | FLAG_NT);
}

/* The main system call interface */
void
syscall_handler (struct intr_frame *f UNUSED) {
	// TODO: Your implementation goes here.
	struct gp_registers regs=f->R;
	int syscall_number=regs.rax;
	//printf("syscall_number = %d\n", syscall_number);
	uint64_t arg0=regs.rdi;
	uint64_t arg1=regs.rsi;
	uint64_t arg2=regs.rdx;
	//int arg3=regs.r10;
	//int arg4=regs.r8;
	//int arg5=regs.r9;
	switch(syscall_number){
		case SYS_HALT:
			//printf("(halt) begin\n");
			power_off();
			break;
		case SYS_EXIT:
			char* file_name=thread_current()->name; //rsi는 0이라 thread_current로 받아야함
			int exit_status=(int)arg0;
			printf("%s: exit(%d)\n",file_name,exit_status);
			//printf("(exit) begin\n");
			thread_exit ();
			break;
		
		case SYS_FORK:{
			printf("[syscall] FORK called\n");
			char* thread_name=(char*)arg0;
			tid_t child_tid = process_fork(thread_name,f);
			timer_sleep(15);
			// wait() 필요
			if(child_tid == TID_ERROR){
				f->R.rax=-1;
				printf("TID ERROR\n");
			}
			else{
				f->R.rax = child_tid;
				printf("SUCCESS\n");
			}
			break;
		}
		case SYS_WAIT:
			tid_t wait_tid = (tid_t)arg0;
			int status = process_wait(wait_tid);
			printf("status: %d\n", status);
			f->R.rax = status;
			break;
		/*		
		case SYS_EXEC:
			break;
		
		case SYS_WAIT:
			break;
		case SYS_CREATE:
			break;
		case SYS_REMOVE:
			break;
		case SYS_OPEN:
			break;
		case SYS_FILESIZE:
			break;
		case SYS_READ:
			break;*/
		case SYS_WRITE:{
			//printf("sys_write called\n");
		uint64_t fd=regs.rdi;
		const char* buffer=(const char*)regs.rsi;
		size_t size=(size_t)regs.rdx;
		//printf("starting write\n");

		//printf("sys_write: fd=%llu, buffer=%p, size=%zu\n", fd, buffer, size);
			if(fd==STDOUT_FILENO){
				putbuf((const char*)buffer, (size_t)size);
				f->R.rax=size;
			}
			else if(fd==STDIN_FILENO){
				
			}
			else if(arg0==-1){
				//f->R.rax=-1;
			}
			//return하는 rax는 실제 쓰인 바이트 수
			break;
		}
		/*case SYS_SEEK:
			break;
		case SYS_TELL:
			break;
		case SYS_CLOSE:
			break;
		case SYS_MMAP:
			break;
		case SYS_MUNMAP:
			break;
		case SYS_CHDIR:
			break;
		case SYS_MKDIR:
			break;
		case SYS_READDIR:
			break;
		case SYS_ISDIR:
			break;
		case SYS_INUMBER:
			break;
		case SYS_SYMLINK:
			break;
		case SYS_DUP2:
			break;
		case SYS_MOUNT:
			break;
		case SYS_UMOUNT:
			break;*/
		default:
			break;
	}
}