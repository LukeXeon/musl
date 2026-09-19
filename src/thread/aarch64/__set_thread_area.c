#include <elf.h>
#include "libc.h"

#define BITRANGE(a,b) (2*(1UL<<(b))-(1UL<<(a)))

/* [rosetta 补丁 0003] thread pointer 写侧 —— 上游 msr tpidr_el0(真
 * 寄存器);嵌入式形态改落 rosetta per-TID 表(shim/src/slibc/musl_tls.rs),
 * 真寄存器归宿主语言运行时(与补丁 0002 成对)。返回 0 = 成功且允许
 * 线程(上游 __init_tp 依此置 libc.can_do_threads)。 */
extern int rosetta_guest_musl_set_tp(void *);

int __set_thread_area(void *p)
{
	int r = rosetta_guest_musl_set_tp(p);
	if (r < 0) return r;

	/* Mask off hwcap bits for SME and unknown future features. This is
	 * necessary because SME is not safe to use without libc support for
	 * it, and we do not (yet) have such support. */
	for (size_t *v = libc.auxv; *v; v+=2) {
		if (v[0]==AT_HWCAP) {
			v[1] &= ~BITRANGE(42,63); /* 42-47 are SME */
		} else if (v[0]==AT_HWCAP2) {
			v[1] &= ~(BITRANGE(23,30)
			        | BITRANGE(37,42)
			        | BITRANGE(57,62));
		} else if (v[0]==AT_HWCAP3 || v[0]==AT_HWCAP4) {
			v[0] = AT_IGNORE;
			v[1] = 0;
		}
	}

	return 0;
}
