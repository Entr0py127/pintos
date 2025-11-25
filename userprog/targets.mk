userprog_SRC  = userprog/process.c	# Process loading.
userprog_SRC += userprog/exception.c	# User exception handler.
userprog_SRC += userprog/syscall-entry.S # System call entry.
userprog_SRC += userprog/syscall.c	# System call handler.
userprog_SRC += userprog/gdt.c		# GDT initialization.
userprog_SRC += userprog/tss.c		# TSS management.

# Syscall handlers
userprog_SRC += userprog/handler/halt.c
userprog_SRC += userprog/handler/exit.c
userprog_SRC += userprog/handler/fork.c
userprog_SRC += userprog/handler/exec.c
userprog_SRC += userprog/handler/wait.c
userprog_SRC += userprog/handler/create.c
userprog_SRC += userprog/handler/remove.c
userprog_SRC += userprog/handler/open.c
userprog_SRC += userprog/handler/filesize.c
userprog_SRC += userprog/handler/read.c
userprog_SRC += userprog/handler/write.c
userprog_SRC += userprog/handler/seek.c
userprog_SRC += userprog/handler/tell.c
userprog_SRC += userprog/handler/close.c
userprog_SRC += userprog/handler/dup2.c
