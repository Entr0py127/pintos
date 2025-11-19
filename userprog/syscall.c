#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/loader.h"
#include "userprog/gdt.h"
#include "threads/flags.h"
#include "intrinsic.h"
#include "filesys/filesys.h"
#include "threads/vaddr.h"
#include "threads/mmu.h"
#include "filesys/file.h"
#include "userprog/process.h"

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
	uint64_t arg0=regs.rdi;
	uint64_t arg1=regs.rsi;
	uint64_t arg2=regs.rdx;
	//int arg3=regs.r10;
	//int arg4=regs.r8;
	//int arg5=regs.r9;
	switch(syscall_number){
		case SYS_HALT:
			power_off();
			break;
		case SYS_EXIT:{
			char* file_name=thread_current()->name; //rsi는 0이라 thread_current로 받아야함
			int exit_status=(int)arg0;
			thread_current()->exit_status = exit_status;
			printf("%s: exit(%d)\n",file_name,exit_status);
			thread_exit ();
			break;
		}
		case SYS_FORK:{
			// printf("[syscall] FORK called\n");
			char* thread_name=(char*)arg0;
			
			tid_t child_tid = process_fork(thread_name,f);

			// wait() 필요
			if(child_tid == TID_ERROR){
				f->R.rax=-1;
			}
			else{
				f->R.rax = child_tid;
			}
			break;
		}
		case SYS_EXEC:
		 	break;
		case SYS_WAIT:{
			tid_t tid = (tid_t)arg0;
			int status = process_wait(tid);
			f->R.rax = status;
			break;
		}
		case SYS_CREATE:{
			char* name=(char*)arg0;
			off_t initial_size=(off_t)arg1;
			bool success=false;
			 //순서대로 name 이 null이 아닌지유저 stack인지, 할당되어 있는지
			if(name!=NULL&&is_user_vaddr(name)&&pml4_get_page(thread_current()->pml4,name)!=NULL&&initial_size>=0) 
				success=filesys_create(name, initial_size);
			else
				{ 
					f->R.rax=SYS_EXIT; //exit
					f->R.rdi=-1;
					syscall_handler(f);
				}
			f->R.rax=success;
			break;
		}
		case SYS_REMOVE:
			break;
		case SYS_OPEN:{
			char* name=(char*)arg0;
			if(name!=NULL&&is_user_vaddr(name)&&pml4_get_page(thread_current()->pml4,name)!=NULL){
				struct file *fo =filesys_open(name);
				if(fo != NULL){
					// 리턴해주는 값이 fd인 거임. 이제 이 fd값을 어디에 저장을 해서 출력을 해야하는게 문제인 거지. 어떤 방식으로 해볼까..... 스레드에 저장을 해야 하는 건가???? 그런 거 같은데?
					// 파일이 오픈이 되었다는 것.
					struct thread *t = thread_current();
					struct fd *fd = (struct fd *)malloc(sizeof(struct fd));
					if(fd == NULL) {
						file_close(fo);
						f->R.rax = -1;
						break;
					}
					fd->file = fo;
					list_push_back(&t->fd_table, &fd->fd_elem);
					fd->cur_fd = ++t->fd_count;
					f->R.rax=fd->cur_fd;
				}
				else
					f->R.rax=-1;
				}
			// 이름이 정상적이지 않거나 하면 exit으로. 재귀 수정 나중에 수정
			else
				{
					f->R.rax=SYS_EXIT; //exit
					f->R.rdi=-1;
					syscall_handler(f);
				}
			break;
			}
		case SYS_FILESIZE:
			break;
		case SYS_READ:
			break;
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
			break;*/
		case SYS_CLOSE:{
			int fd = (int)arg0;
			if (fd < 2) {
				break;
			}
			for(struct list_elem *e = list_begin(&thread_current()->fd_table); e != list_end(&thread_current()->fd_table); e = list_next(e)){
				struct fd *FD= list_entry(e, struct fd, fd_elem);
				if (FD->cur_fd == fd) {
					file_close(FD->file);
					list_remove(e);
					free(FD);
					break;
				}
			}
			break;
		}
		/*case SYS_MMAP:
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