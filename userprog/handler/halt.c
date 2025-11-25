#include "userprog/handler.h"
#include "threads/interrupt.h"
#include "devices/shutdown.h"

void
sys_halt(struct intr_frame *f UNUSED) {
    power_off();
}
