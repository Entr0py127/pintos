#ifndef THREADS_THREAD_H
#define THREADS_THREAD_H

#include <debug.h>
#include <list.h>
#include <stdint.h>
#include "threads/interrupt.h"
#include "threads/synch.h"
#ifdef VM
#include "vm/vm.h"
#endif


/* States in a thread's life cycle. */
enum thread_status {
	THREAD_RUNNING,     /* Running thread. */
	THREAD_READY,       /* Not running but ready to run. */
	THREAD_BLOCKED,     /* Waiting for an event to trigger. */
	THREAD_DYING        /* About to be destroyed. */
};

/* Thread identifier type.
   You can redefine this to whatever type you like. */
typedef int tid_t;
#define TID_ERROR ((tid_t) -1)          /* Error value for tid_t. */

/* Thread priorities. */
#define PRI_MIN 0                       /* Lowest priority. */
#define PRI_DEFAULT 31                  /* Default priority. */
#define PRI_MAX 63                      /* Highest priority. */

// 부동소수점 계산하는 거 매크로 정리
typedef int64_t fixed_t;
#define FP_SHIFT 14
#define INT_TO_FP(n) ((n) << FP_SHIFT)				// 정수를 고정소수로 표현할 떄
#define FP_TO_INT_ZERO(x) ((x) >> FP_SHIFT)			// 고정소수를 정수로 반환할 떄
#define FP_TO_INT_FLOOR(x) ((x) >= 0 ? (x) >> FP_SHIFT : ((x) >> FP_SHIFT) - 1)		// 아래쪽 정수로 반올림(floor)
#define FP_ADD(x, y) ((x) + (y))
#define FP_SUB(x, y) ((x) - (y))
#define FP_MUL(x, y) (((int64_t)(x)) * (y) >> FP_SHIFT)
#define FP_DIV(x, y) (((int64_t)(x) << FP_SHIFT) / (y))
#define FP_ADD_INT(x, n) ((x) + (INT_TO_FP(n)))
#define FP_SUB_INT(x, n) ((x) - (INT_TO_FP(n)))
#define FP_MUL_INT(x, n) ((x) * (n))
#define FP_DIV_INT(x, n) ((x) / (n))

/* A kernel thread or user process.
 *
 * Each thread structure is stored in its own 4 kB page.  The
 * thread structure itself sits at the very bottom of the page
 * (at offset 0).  The rest of the page is reserved for the
 * thread's kernel stack, which grows downward from the top of
 * the page (at offset 4 kB).  Here's an illustration:
 *
 *      4 kB +---------------------------------+
 *           |          kernel stack           |
 *           |                |                |
 *           |                |                |
 *           |                V                |
 *           |         grows downward          |
 *           |                                 |
 *           |                                 |
 *           |                                 |
 *           |                                 |
 *           |                                 |
 *           |                                 |
 *           |                                 |
 *           |                                 |
 *           +---------------------------------+
 *           |              magic              |
 *           |            intr_frame           |
 *           |                :                |
 *           |                :                |
 *           |               name              |
 *           |              status             |
 *      0 kB +---------------------------------+
 *
 * The upshot of this is twofold:
 *
 *    1. First, `struct thread' must not be allowed to grow too
 *       big.  If it does, then there will not be enough room for
 *       the kernel stack.  Our base `struct thread' is only a
 *       few bytes in size.  It probably should stay well under 1
 *       kB.
 *
 *    2. Second, kernel stacks must not be allowed to grow too
 *       large.  If a stack overflows, it will corrupt the thread
 *       state.  Thus, kernel functions should not allocate large
 *       structures or arrays as non-static local variables.  Use
 *       dynamic allocation with malloc() or palloc_get_page()
 *       instead.
 *
 * The first symptom of either of these problems will probably be
 * an assertion failure in thread_current(), which checks that
 * the `magic' member of the running thread's `struct thread' is
 * set to THREAD_MAGIC.  Stack overflow will normally change this
 * value, triggering the assertion. */
/* The `elem' member has a dual purpose.  It can be an element in
 * the run queue (thread.c), or it can be an element in a
 * semaphore wait list (synch.c).  It can be used these two ways
 * only because they are mutually exclusive: only a thread in the
 * ready state is on the run queue, whereas only a thread in the
 * blocked state is on a semaphore wait list. */
struct child_info {
	tid_t tid;
	int exit_status;
	int called;
	struct list_elem child_elem;
	struct semaphore child_sema;
};

struct fd {
	struct file *file;
	struct list_elem fd_elem;
	int type;
	int fd_num;						// 지금 이 파일이 가지고 있는 fd의 숫자
};

struct thread {
	/* Owned by thread.c. */
	tid_t tid;                          /* Thread identifier. */
	enum thread_status status;          /* Thread state. */
	char name[16];                      /* Name (for debugging purposes). */
	int original_priority;
	int priority;                       /* Priority. */
	int wake_tick;	// 일어날 tick 정보
	int64_t recent_cpu;			// thread별 cpu차지 시간 초기값 0. 고정소수점(fixed-point)
	int nice;				// thread별 nice 값 초기는 0
	struct lock *waiting_lock; // 현재 대기중인 lock
	struct semaphore *waiting_sema;
	struct semaphore exec_sema;
	struct list donations;
	struct list_elem donation_elem;
	
	struct list_elem all_threads_list_elem;

	/* Shared between thread.c and synch.c. */
	struct list_elem elem;              /* List element. */

#ifdef USERPROG
	/* Owned by userprog/process.c. */
	uint64_t *pml4;                     /* Page map level 4 */
	struct list children;				// 직계 자식들에 관한 리스트
	struct child_info *child_infop;		// 부모쪽의 child_info를 가르키도록 하는 포인터 변수. 이걸 이용해서 sema_up를 실행.
	int exit_status;					// 만약 child_info안에 있다가 부모가 없어지면 애를 들고올 방법이 없음.
	
#endif	
#ifdef VM
	/* Table for whole virtual memory owned by thread. */
	struct supplemental_page_table spt;
#endif
	struct list fd_table;
	int fd_count;							// 전체 fd 숫자
	struct file *running_file;
	/* Owned by thread.c. */
	struct intr_frame tf;               /* Information for switching */
	unsigned magic;                     /* Detects stack overflow. */
};

/* If false (default), use round-robin scheduler.
   If true, use multi-level feedback queue scheduler.
   Controlled by kernel command-line option "-o mlfqs". */
extern bool thread_mlfqs;

void thread_init (void);
void thread_start (void);

void thread_tick (void);
void thread_print_stats (void);

typedef void thread_func (void *aux);
tid_t thread_create (const char *name, int priority, thread_func *, void *);

void thread_block (void);
void thread_unblock (struct thread *);

struct thread *thread_current (void);
tid_t thread_tid (void);
const char *thread_name (void);

void thread_exit (void) NO_RETURN;
void thread_yield (void);

int thread_get_priority (void);
void thread_set_priority (int);

int thread_get_nice (void);
void thread_set_nice (int);
int thread_get_recent_cpu (void);
int thread_get_load_avg (void);

bool thread_priority_less(const struct list_elem *a, const struct list_elem *b, void *aux);

void do_iret (struct intr_frame *tf);

#endif /* threads/thread.h */
