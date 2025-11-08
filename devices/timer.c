#include "devices/timer.h"
#include <debug.h>
#include <inttypes.h>
#include <round.h>
#include <stdio.h>
#include "threads/interrupt.h"
#include "threads/io.h"
#include "threads/synch.h"
#include "threads/thread.h"

extern struct list sleep_list;

/* See [8254] for hardware details of the 8254 timer chip. */

#if TIMER_FREQ < 19
#error 8254 timer requires TIMER_FREQ >= 19
#endif
#if TIMER_FREQ > 1000
#error TIMER_FREQ <= 1000 recommended
#endif

/* Number of timer ticks since OS booted. */
static int64_t ticks;

/* Number of loops per timer tick.
   Initialized by timer_calibrate(). */
static unsigned loops_per_tick;

static intr_handler_func timer_interrupt;
static bool too_many_loops (unsigned loops);
static void busy_wait (int64_t loops);
static void real_time_sleep (int64_t num, int32_t denom);

/* Sets up the 8254 Programmable Interval Timer (PIT) to
   interrupt PIT_FREQ times per second, and registers the
   corresponding interrupt. */
void
timer_init (void) {
	/* 8254 input frequency divided by TIMER_FREQ, rounded to
	   nearest. */
	uint16_t count = (1193180 + TIMER_FREQ / 2) / TIMER_FREQ;

	outb (0x43, 0x34);    /* CW: counter 0, LSB then MSB, mode 2, binary. */
	outb (0x40, count & 0xff);
	outb (0x40, count >> 8);

	intr_register_ext (0x20, timer_interrupt, "8254 Timer");
}

/* Calibrates loops_per_tick, used to implement brief delays. */
void
timer_calibrate (void) {
	unsigned high_bit, test_bit;

	ASSERT (intr_get_level () == INTR_ON);
	printf ("Calibrating timer...  ");

	/* Approximate loops_per_tick as the largest power-of-two
	   still less than one timer tick. */
	loops_per_tick = 1u << 10;
	while (!too_many_loops (loops_per_tick << 1)) {
		loops_per_tick <<= 1;
		ASSERT (loops_per_tick != 0);
	}

	/* Refine the next 8 bits of loops_per_tick. */
	high_bit = loops_per_tick;
	for (test_bit = high_bit >> 1; test_bit != high_bit >> 10; test_bit >>= 1)
		if (!too_many_loops (high_bit | test_bit))
			loops_per_tick |= test_bit;

	printf ("%'"PRIu64" loops/s.\n", (uint64_t) loops_per_tick * TIMER_FREQ);
}

/* Returns the number of timer ticks since the OS booted. */
// 현재 부팅된 이후 흐른 tick의 수를 안전하게 받아오는 함수
int64_t
timer_ticks (void) {
	enum intr_level old_level = intr_disable (); // 인터럽트 상태을 저장하고, 인터럽트 일시적으로 비활성화(읽는 도중 값이 들어오면 tick값이 바뀔수도 있어서 차단)
	int64_t t = ticks;				
	intr_set_level (old_level);			// 이전에 저장했던 상태 복원. 즉, 꺼져 있으면 키고, 켜져 있으면 끄는 것.
	barrier ();							// 컴파일러가 명령어 순서를 재배치 못하도록 막음. 최적화 방지
	return t;
}

/* Returns the number of timer ticks elapsed since THEN, which
   should be a value once returned by timer_ticks(). */
// 지금 tick이 then때의 tick 보다 얼마나 흘렀는지 반환하는 함수
int64_t
timer_elapsed (int64_t then) {
	return timer_ticks () - then;
}

bool cmp_wakeup_tick(const struct list_elem *a, const struct list_elem *b, void *aux UNUSED) {
	const struct thread *t1 = list_entry(a, struct thread, elem);
	const struct thread *t2 = list_entry(b, struct thread, elem);
	return t1 -> wake_tick < t2 -> wake_tick;
}

/* Suspends execution for approximately TICKS timer ticks. */
void
timer_sleep (int64_t ticks) {
	int64_t start = timer_ticks ();				// 부팅된 이후 안전하게 tick의 값을 가져옴.

	enum intr_level old_level = intr_disable(); // 인터럽트 멈추기.
	struct thread *curr = thread_current ();	// 현재 스레드

	curr -> wake_tick = start + ticks;			// wake tick 설정
	list_insert_ordered(&sleep_list, &curr -> elem, cmp_wakeup_tick, NULL);		// 그리고 sleep_list에 넣어주기

	thread_block();

	intr_set_level(old_level);		// 인터럽트 키기
}

/* Suspends execution for approximately MS milliseconds. */
void
timer_msleep (int64_t ms) {
	real_time_sleep (ms, 1000);
}

/* Suspends execution for approximately US microseconds. */
void
timer_usleep (int64_t us) {
	real_time_sleep (us, 1000 * 1000);
}

/* Suspends execution for approximately NS nanoseconds. */
void
timer_nsleep (int64_t ns) {
	real_time_sleep (ns, 1000 * 1000 * 1000);
}

/* Prints timer statistics. */
void
timer_print_stats (void) {
	printf ("Timer: %"PRId64" ticks\n", timer_ticks ());
}

/* Timer interrupt handler. */
static void
timer_interrupt (struct intr_frame *args UNUSED) {
	ticks++;
	thread_tick ();

	// 어차피 ticks 순서대로 넣었으니깐 맨 처음의 wakeup_tick를 검사를 해서 ticks가 같으면 깨워준다.
	// 이거 할때도 인터럽트 off 시켜줘야 하는 거 아닌가? 당연히?
	// 외부 인터럽트 컨텍스트에서 인터럽트가 이미 꺼져 있는 상태

	int64_t cur_tick = ticks;				// 부팅된 이후 안전하게 tick의 값을 가져옴.
	// 리스트 비었는지 검사
	if (list_empty(&sleep_list)) {
		return;
	}
	struct list_elem *a = list_front(&sleep_list);
	struct thread *t = list_entry(a, struct thread, elem);

	while(t -> wake_tick <= cur_tick){
		list_pop_front(&sleep_list);
		thread_unblock(t);
		if(list_empty(&sleep_list)) {
			break;
		}
		a = list_front(&sleep_list);
		t = list_entry(a, struct thread, elem);
	}
	
}

/* Returns true if LOOPS iterations waits for more than one timer
   tick, otherwise false. */
static bool
too_many_loops (unsigned loops) {
	/* Wait for a timer tick. */
	int64_t start = ticks;
	while (ticks == start)
		barrier ();

	/* Run LOOPS loops. */
	start = ticks;
	busy_wait (loops);

	/* If the tick count changed, we iterated too long. */
	barrier ();
	return start != ticks;
}

/* Iterates through a simple loop LOOPS times, for implementing
   brief delays.

   Marked NO_INLINE because code alignment can significantly
   affect timings, so that if this function was inlined
   differently in different places the results would be difficult
   to predict. */
static void NO_INLINE
busy_wait (int64_t loops) {
	while (loops-- > 0)
		barrier ();
}

/* Sleep for approximately NUM/DENOM seconds. */
static void
real_time_sleep (int64_t num, int32_t denom) {
	/* Convert NUM/DENOM seconds into timer ticks, rounding down.

	   (NUM / DENOM) s
	   ---------------------- = NUM * TIMER_FREQ / DENOM ticks.
	   1 s / TIMER_FREQ ticks
	   */
	int64_t ticks = num * TIMER_FREQ / denom;

	ASSERT (intr_get_level () == INTR_ON);
	if (ticks > 0) {
		/* We're waiting for at least one full timer tick.  Use
		   timer_sleep() because it will yield the CPU to other
		   processes. */
		timer_sleep (ticks);
	} else {
		/* Otherwise, use a busy-wait loop for more accurate
		   sub-tick timing.  We scale the numerator and denominator
		   down by 1000 to avoid the possibility of overflow. */
		ASSERT (denom % 1000 == 0);
		busy_wait (loops_per_tick * num / 1000 * TIMER_FREQ / (denom / 1000));
	}
}
