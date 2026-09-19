#include <stdlib.h>
#include <stdint.h>
#include "libc.h"
#include "lock.h"
#include "fork_impl.h"

#define malloc __libc_malloc
#define calloc __libc_calloc
#define realloc undef
#define free undef

/* Ensure that at least 32 atexit handlers can be registered without malloc */
#define COUNT 32

static struct fl
{
	struct fl *next;
	void (*f[COUNT])(void *);
	void *a[COUNT];
} builtin, *head;

static int finished_atexit;
static int slot;
static volatile int lock[1];
volatile int *const __atexit_lockptr = lock;

void __funcs_on_exit()
{
	void (*func)(void *), *arg;
	LOCK(lock);
	for (; head; head=head->next, slot=COUNT) while(slot-->0) {
		func = head->f[slot];
		arg = head->a[slot];
		UNLOCK(lock);
		func(arg);
		LOCK(lock);
	}
	/* Unlock to prevent deadlock if a global dtor
	 * attempts to call atexit. */
	finished_atexit = 1;
	UNLOCK(lock);
}

void __cxa_finalize(void *dso)
{
}

int __cxa_atexit(void (*func)(void *), void *arg, void *dso)
{
	LOCK(lock);

	/* Prevent dtors from registering further atexit
	 * handlers that would never be run. */
	if (finished_atexit) {
		UNLOCK(lock);
		return -1;
	}

	/* Defer initialization of head so it can be in BSS */
	if (!head) head = &builtin;

	/* If the current function list is full, add a new one */
	if (slot==COUNT) {
		struct fl *new_fl = calloc(sizeof(struct fl), 1);
		if (!new_fl) {
			UNLOCK(lock);
			return -1;
		}
		new_fl->next = head;
		head = new_fl;
		slot = 0;
	}

	/* Append function to the list. */
	head->f[slot] = func;
	head->a[slot] = arg;
	slot++;

	UNLOCK(lock);
	return 0;
}

static void call(void *p)
{
	((void (*)(void))(uintptr_t)p)();
}

/* [rosetta 补丁 0004] atexit 定义移除 —— 嵌入式形态的链接驱动(NDK
 * clang)恒带 crtbegin_so.o,其中 atexit 是强定义、实现同型(= 转
 * `__cxa_atexit(f,0,__dso_handle)`);两处强定义在同一 .so 内撞名。
 * 保留本文件的 `__cxa_atexit`(musl 退出登记表的真入口,crtbegin 的
 * atexit 与 C++ 静态析构都落到它),仅去掉重复的裸名。 */
#if 0
int atexit(void (*func)(void))
{
	return __cxa_atexit(call, (void *)(uintptr_t)func, 0);
}
#endif
