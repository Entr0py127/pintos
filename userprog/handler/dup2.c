#include "userprog/handler.h"
#include "filesys/file.h"
#include "filesys/inode.h"
#include "threads/thread.h"

void
sys_dup2(struct intr_frame *f) {
    int oldfd = (int)f->R.rdi;
    int newfd = (int)f->R.rsi;
    struct thread *cur = thread_current();
    
    if (oldfd < 0 || newfd < 0 || oldfd >= FD_MAX || newfd >= FD_MAX) {
        f->R.rax = -1;
        return;
    }
    
    if (oldfd == newfd) {
        f->R.rax = newfd;
        return;
    }
    
    // oldfd 유효성 검사
    if (oldfd == 0) {
        if (cur->fd_stdin == -1) {
            f->R.rax = -1;
            return;
        }
    } else if (oldfd == 1) {
        if (cur->fd_stdout == -1) {
            f->R.rax = -1;
            return;
        }
    } else {
        if (cur->fd_table[oldfd] == NULL) {
            f->R.rax = -1;
            return;
        }
    }
    
    // newfd가 열려있으면 먼저 닫기
    if (newfd == 0) {
        cur->fd_stdin = -1;
    } else if (newfd == 1) {
        cur->fd_stdout = -1;
    } else if (cur->fd_table[newfd] != NULL) {
        ref_count_down(cur->fd_table[newfd]);
        if (file_ref_cnt(cur->fd_table[newfd]) == 0) {
            file_close(cur->fd_table[newfd]);
        }
        cur->fd_table[newfd] = NULL;
    }
    
    // 복제 수행
    if (oldfd == 0) {
        // stdin을 newfd로 복제
        if (newfd == 1) {
            cur->fd_stdout = -2;  // -2: redirected to stdin
        } else {
            // 일반 fd로 stdin 복제는 지원 안 함 (특수 처리 필요)
            cur->fd_table[newfd] = NULL;  // stdin은 file 없음
        }
    } else if (oldfd == 1) {
        // stdout을 newfd로 복제
        if (newfd == 0) {
            cur->fd_stdin = -2;  // -2: redirected to stdout
        } else {
            cur->fd_table[newfd] = NULL;  // stdout은 file 없음
        }
    } else {
        // 일반 파일 복제
        if (newfd == 0 || newfd == 1) {
            // 파일을 stdin/stdout으로 리다이렉션하는 것은 특수 처리
            f->R.rax = -1;
            return;
        }
        cur->fd_table[newfd] = cur->fd_table[oldfd];
        ref_count_up(cur->fd_table[newfd]);
    }
    
    f->R.rax = newfd;
}
