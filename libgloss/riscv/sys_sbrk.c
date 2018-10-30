#ifdef USING_SIM_SPECS

// Gdb simulator requires that sbrk be implemented without a syscall.
extern char _end[];                /* _end is set in the linker command file */
char *heap_ptr;

/*
 * sbrk -- changes heap size size. Get nbytes more
 *         RAM. We just increment a pointer in what's
 *         left of memory on the board.
 */
char *
_sbrk (nbytes)
     int nbytes;
{
  char        *base;

  if (!heap_ptr)
    heap_ptr = (char *)&_end;
  base = heap_ptr;
  heap_ptr += nbytes;

  return base;
}

#else

#ifdef CKB_NO_MMU

#include <sys/types.h>
/*
 * For simplicity, we are allocating 8MB - 120MB memory range as the
 * memory space for brk. This preserves 8MB for code size, and 8MB for
 * stack size. This is just a temorary setting for now, in production
 * we should make those tunable build parameters.
 * Since ckb-vm doesn't have MMU, we don't need to make real syscalls.
 */
#define BRK_MIN 0x00800000
#define BRK_MAX 0x07800000

void*
_sbrk(ptrdiff_t incr)
{
  static uintptr_t p = BRK_MIN;
  uintptr_t start = p;
  p += incr;
  if (p > BRK_MAX) {
    return (void *) (-1);
  }
  return start;
}

#else
// QEMU uses a syscall.
#include <machine/syscall.h>
#include <sys/types.h>
#include "internal_syscall.h"

/* Increase program data space. As malloc and related functions depend
   on this, it is useful to have a working implementation. The following
   is suggested by the newlib docs and suffices for a standalone
   system.  */
void *
_sbrk(ptrdiff_t incr)
{
  static unsigned long heap_end;

  if (heap_end == 0)
    {
      long brk = __internal_syscall (SYS_brk, 0, 0, 0, 0, 0, 0);
      if (brk == -1)
        return (void *)__syscall_error (-ENOMEM);
      heap_end = brk;
    }

  if (__internal_syscall (SYS_brk, heap_end + incr, 0, 0, 0, 0, 0) != heap_end + incr)
    return (void *)__syscall_error (-ENOMEM);

  heap_end += incr;
  return (void *)(heap_end - incr);
}
#endif  /* CKB_NO_MMU */
#endif  /* USING_SIM_SPECS */
