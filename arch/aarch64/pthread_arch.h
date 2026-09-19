/* [rosetta 补丁 0002] thread pointer 虚拟化 —— 上游 TP = TPIDR_EL0
 * (宿主寄存器);嵌入式形态里该寄存器归宿主语言运行时(bionic TLS,
 * shim 自身用 Rust thread_local),musl 的 TP 改由 rosetta 侧 per-TID
 * 表供给(实现 = shim/src/slibc/musl_tls.rs;写侧见补丁 0003)。
 * ABI 全等:仍是"本线程 pthread 结构地址",musl 其余全族零改动。 */
extern uintptr_t rosetta_guest_musl_tp(void);

static inline uintptr_t __get_tp()
{
	return rosetta_guest_musl_tp();
}

#define TLS_ABOVE_TP
#define GAP_ABOVE_TP 16

#define MC_PC pc
