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
#include "filesys/inode.h"
#include "include/lib/stdio.h"

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
			//printf("[syscall] FORK called\n");
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
			const char * f_name = (const char *)arg0;
			if(f_name!=NULL&&is_user_vaddr(f_name)&&pml4_get_page(thread_current()->pml4,f_name)!=NULL) {
				int return_status = process_exec(f_name);
				if(return_status == -1) {
					f->R.rax=SYS_EXIT; //exit
					f->R.rdi=-1;
					syscall_handler(f);
				}
			}
			else{
				f->R.rax=SYS_EXIT; //exit
				f->R.rdi=-1;
				syscall_handler(f);
			}
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
		{
			/* 파일 삭제 성공 시 true 그렇지 않다면 false */
			char* name=(char*)arg0;
			if(name!=NULL && is_user_vaddr(name)&&pml4_get_page(thread_current()->pml4,name)!=NULL){
				/* file 삭제 */
				f->R.rax = filesys_remove(name);
			}
			break;
		}
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
					fd->file = file_duplicate(fo);
					list_push_back(&t->fd_table, &fd->fd_elem);
					fd->cur_fd = t->fd_count++;
					fd->type = 2;
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
		case SYS_FILESIZE:{
			int fd = (int)arg0;
			struct file *file=NULL;
			struct list *fd_table = &thread_current()->fd_table;
			for(struct list_elem *e = list_begin(fd_table); e != list_end(fd_table); e = list_next(e)){
				struct fd *temp= list_entry(e, struct fd, fd_elem);
				if (temp->cur_fd == fd) {
					file = temp->file;
					break;
				}
			}
			off_t filesize=file_length(file); 
			//printf("filesize: %d\n",filesize);
			f->R.rax=filesize;
			break;
		}

		case SYS_READ:{
			int fd = (int)arg0;
			void* buffer=(void*)arg1;
			off_t size = (off_t)arg2;
			if(buffer==NULL||!is_user_vaddr(buffer)||pml4_get_page(thread_current()->pml4,buffer)==NULL||size<0){
				f->R.rax=SYS_EXIT; //exit
				f->R.rdi=-1;
				syscall_handler(f);
			}
			struct file *file=NULL;
			struct fd *temp=NULL;
			struct list *fd_table = &thread_current()->fd_table;
			for(struct list_elem *e = list_begin(fd_table); e != list_end(fd_table); e = list_next(e)){
				temp= list_entry(e, struct fd, fd_elem);
				if (temp->cur_fd == fd) {
					file = temp->file;
					break;
				}
			}
			/*
			if(fd==1||buffer==NULL||!is_user_vaddr(buffer)||pml4_get_page(thread_current()->pml4,buffer)==NULL||size<0){
				f->R.rax=SYS_EXIT; //exit
				f->R.rdi=-1;
				syscall_handler(f);
			} //fd==0 or 2

			if(file==NULL){
				for(int i=0;i<size;i++){
					memcpy(buffer,(void*)input_getc(),sizeof(char));
					buffer+=sizeof(char);
				}
				f->R.rax=size;
			}
			else{
				int bytes_read=file_read(file,buffer,size);
				f->R.rax=bytes_read;
			}*/
			if(file == NULL){
				if(fd == STDIN_FILENO || (temp != NULL && temp->type == STDIN_FILENO)){
					for(int i=0;i<size;i++){
						memcpy(buffer,(void*)input_getc(),sizeof(char));
						buffer+=sizeof(char);
					}
					f->R.rax=size;
				}
				else{
					f->R.rax=SYS_EXIT; //exit
					f->R.rdi=-1;
					syscall_handler(f);
				}
				
			}
			else{
				if(fd > 2 || temp->type == 2){
					int bytes_read=file_read(file,buffer,size);
					f->R.rax=bytes_read;
				}
			}
			break;
		}
		case SYS_WRITE:{
			uint64_t fd=regs.rdi;
			const char* buffer=(const char*)regs.rsi;
			size_t size=(size_t)regs.rdx;
			if(buffer==NULL||!is_user_vaddr(buffer)||pml4_get_page(thread_current()->pml4,buffer)==NULL||size<0)
			{
				f->R.rax=SYS_EXIT; //exit
				f->R.rdi=-1;
				syscall_handler(f);
			}
			struct fd *temp=NULL;
			struct file *file=NULL;
			struct list *fd_table = &thread_current()->fd_table;
			for(struct list_elem *e = list_begin(fd_table); e != list_end(fd_table); e = list_next(e)){
				temp= list_entry(e, struct fd, fd_elem);
				if(temp->cur_fd == fd){
					file=temp->file;
					break;
				}
			}
			/*
			if(fd==STDIN_FILENO){
				f->R.rax=SYS_EXIT;
				f->R.rdi=-1;
				syscall_handler(f);
			}
			else if(file!=NULL){
				int bytes_write=file_write(file,buffer,size);
				if(bytes_write==size)
					f->R.rax=bytes_write;
				else
					f->R.rax=0;
			}
			else if(file==NULL){
				if(fd==STDOUT_FILENO||temp->type==STDOUT_FILENO)
					{putbuf((const char*)buffer, (size_t)size);
					f->R.rax=size;}
				else{
					f->R.rax=SYS_EXIT;
					f->R.rdi=-1;
					syscall_handler(f);
				}
			}*/
			if(file == NULL){
				if(fd == STDOUT_FILENO || (temp != NULL && temp->type == STDOUT_FILENO)){
					putbuf((const char*)buffer, (size_t)size);
					f->R.rax=size;
				}
				else{
					f->R.rax=SYS_EXIT; //exit
					f->R.rdi=-1;
					syscall_handler(f);
				}
			}
			else{
				if(fd > 2 || temp->type == 2){
					int bytes_write=file_write(file,buffer,size);
					if(bytes_write==size)
						f->R.rax=bytes_write;
					else
						f->R.rax=0;
				}
			}
			break;
		}
		case SYS_SEEK:
		{
			int fd = (int)arg0;
			unsigned new_pos = (unsigned)arg1;
			struct file *file = NULL;
			/* 현재 fd를 가진 파일의 pos 바꾸기 */
			// 1. fd 파일 찾기
			for(struct list_elem *e = list_begin(&thread_current()->fd_table); e != list_end(&thread_current()->fd_table); e = list_next(e)){
				struct fd *FD= list_entry(e, struct fd, fd_elem);
				if (FD->cur_fd == fd) {
					file = FD->file;
					break;
				}
			}
			// fd에 해당되는 파일이 없음
			if(file == NULL){
				break;
			}
			// 2. 해당 fd 파일의 pos 바꾸기(file_seek())
			file_seek(file, new_pos);

			break;
		}
			
		case SYS_TELL:
		{
			/* 파일의 위치 바이트 단위로 반환 */
			int fd = (int)arg0;
			if (fd < 2) {
				break;
			}
			struct file *file = NULL;
			// 1. fd 파일 찾기
			for(struct list_elem *e = list_begin(&thread_current()->fd_table); e != list_end(&thread_current()->fd_table); e = list_next(e)){
				struct fd *FD= list_entry(e, struct fd, fd_elem);
				if (FD->cur_fd == fd) {
					file = FD->file;
					break;
				}
			}
			// fd에 해당되는 파일이 없음
			if(file == NULL){
				f->R.rax = 0;
				break;
			}
			// 2. 파일 위치 반환
			unsigned pos = (unsigned)file_tell(file);
			f->R.rax = pos;

			break;
		}
		case SYS_CLOSE:{
			int fd = (int)arg0;
			/*if (fd < 2) {
				break;
			}*/
			for(struct list_elem *e = list_begin(&thread_current()->fd_table); e != list_end(&thread_current()->fd_table); e = list_next(e)){
				struct fd *FD= list_entry(e, struct fd, fd_elem);
				if (FD->cur_fd == fd) {
					if (FD->cur_fd == 0 || FD->cur_fd == 1){
						if (FD->cur_fd == 0 ){
							FD->file = NULL;
							FD->type = 0;
						}
						else if (FD->cur_fd == 1 ){
							FD->file = NULL;
							FD->type = 1;
						}
					}
					else{
						if(FD->file!=NULL){
							ref_count_down(FD->file);
							if (file_ref_cnt(FD->file) == 0){
								file_close(FD->file);
							}
						}
					list_remove(e);
					free(FD);
					}
					break;
				}
			}
			break;
		}
		/*
		* 기존 파일 디스크립터 oldfd를 복제해서, newfd 번호로 새로 할당.
		* 성공 시: newfd 반환
		* 표준 입출력 닫기 지원(원래는 닫는 것이 금지되어 있지만 한번 해보기)
		* stdin을 닫으면 이후 프로그램은 입력을 더 이상 읽을 수 없어야 한다.
		* (즉, read() 호출 시 입력이 오지 않음)
		* `stdout`을 닫으면 이후 프로그램은 아무 것도 출력하지 않아야 한다.
		*/
		case SYS_DUP2:
        {
            int oldfd = (int)arg0;
            int newfd = (int)arg1;
            /* oldfd가 유효하지 않다면 실패 반환 */
			//printf("oldfd: %d, newfd: %d.\n", oldfd, newfd);
            if(oldfd < 0 || newfd < 0){
                f->R.rax = -1;
                break;
            }
            /* oldfd == newfd라면 */
            if(oldfd == newfd){
                f->R.rax = newfd;
                break;
            }
            /* newfd가 이미 열려있다면 재사용 inode_open이용 */
            // 1. oldfd를 가진 fd 찾기
            struct fd *oldFD = NULL;
            for(struct list_elem *e = list_begin(&thread_current()->fd_table); e != list_end(&thread_current()->fd_table); e = list_next(e)){
                struct fd *FD = list_entry(e, struct fd, fd_elem);
                if (FD->cur_fd == oldfd) {
                    oldFD = FD;
                    break;
                }
            }
            // 2. newfd를 가진 fd 찾기
            struct fd *newFD = NULL;
            for(struct list_elem *e = list_begin(&thread_current()->fd_table); e != list_end(&thread_current()->fd_table); e = list_next(e)){
                struct fd *FD= list_entry(e, struct fd, fd_elem);
                if (FD->cur_fd == newfd) {
                    newFD = FD;
                    break;
                }
            }
			if (oldfd >= 2) {
				if(oldFD == NULL || (oldFD->file == NULL&&oldFD->type == 2)){
					f->R.rax = -1;
					break;
				}
			}
				
	            if(newFD != NULL){
				if (newFD->file != NULL){
					ref_count_down(newFD->file);
					if (file_ref_cnt(newFD->file) == 0){
						file_close(newFD->file);
					}
				}
				if(oldfd == 0){
					newFD->type = STDIN_FILENO;
					newFD->file = NULL;
				}
				if(oldfd == 1){
					newFD->type = STDOUT_FILENO;
					newFD->file = NULL;
				}
				if(oldfd > 1){
					newFD->file = oldFD->file;
					ref_count_up(newFD->file);
					newFD->type = oldFD->type;
				}
				//printf("newFD->fd: %d, newFD->type: %d\n", newFD->cur_fd, newFD->type);
                f->R.rax = newfd;
            }
            else {
                newFD = (struct fd *)malloc(sizeof(struct fd));
				if (newFD == NULL) {
					f->R.rax = -1;
					break;
				}
				if(oldfd > 1){
					newFD->file = oldFD->file;
					ref_count_up(newFD->file);
					newFD->type = oldFD->type;
				}
				if(oldfd == 0){
					newFD->type = STDIN_FILENO;
					newFD->file = NULL;
				}
				else if(oldfd == 1){
					newFD->type = STDOUT_FILENO;
					newFD->file = NULL;
				}
                newFD->cur_fd = newfd;
				//printf("newFD->fd: %d, newFD->type: %d\n", newFD->cur_fd, newFD->type);
                list_push_back(&thread_current()->fd_table, &newFD->fd_elem);
                f->R.rax = newfd;
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
		case SYS_MOUNT:
			break;
		case SYS_UMOUNT:
			break;*/
		default:
			break;
	}
}
