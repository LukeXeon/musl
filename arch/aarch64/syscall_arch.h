#define __SYSCALL_LL_E(x) (x)
#define __SYSCALL_LL_O(x) (x)

/* [rosetta 补丁 0001] guest syscall 上行 —— 上游本体 = 内联 "svc 0"
 * (宿主内核直呼);嵌入式形态必须走 rosetta transport 上行 sentry
 * (gVisor 语义;语义只在 sentry,宿主内核零调用)。
 *
 * ABI 全等:调用号 = 调用号(x8 语义),参数 = x0..x5,返回值 = 原语
 * 返回值(负值 = -errno,musl 的 __syscall_ret/errno 处理零改动)。
 * 桥本体 = shim/src/slibc/capi.rs 的 rosetta_guest_syscall6
 * (→ pal::syscall6 → transport::call_hot)。
 *
 * 上游 __asm_syscall 的存在前提 = "内核是唯一上位";本形态改由宿主
 * 函数承接,故 __syscall0..6 一并收成薄转发(上位文件已声明,ABI 见上)。 */
extern long rosetta_guest_syscall6(long n, long a0, long a1, long a2, long a3, long a4, long a5);

static inline long __syscall0(long n)
{
	return rosetta_guest_syscall6(n, 0, 0, 0, 0, 0, 0);
}

static inline long __syscall1(long n, long a)
{
	return rosetta_guest_syscall6(n, a, 0, 0, 0, 0, 0);
}

static inline long __syscall2(long n, long a, long b)
{
	return rosetta_guest_syscall6(n, a, b, 0, 0, 0, 0);
}

static inline long __syscall3(long n, long a, long b, long c)
{
	return rosetta_guest_syscall6(n, a, b, c, 0, 0, 0);
}

static inline long __syscall4(long n, long a, long b, long c, long d)
{
	return rosetta_guest_syscall6(n, a, b, c, d, 0, 0);
}

static inline long __syscall5(long n, long a, long b, long c, long d, long e)
{
	return rosetta_guest_syscall6(n, a, b, c, d, e, 0);
}

static inline long __syscall6(long n, long a, long b, long c, long d, long e, long f)
{
	return rosetta_guest_syscall6(n, a, b, c, d, e, f);
}

#define VDSO_USEFUL
#define VDSO_CGT_SYM "__kernel_clock_gettime"
#define VDSO_CGT_VER "LINUX_2.6.39"

#define IPC_64 0
