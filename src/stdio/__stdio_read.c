#include "stdio_impl.h"
#include <sys/uio.h>

size_t __stdio_read(FILE *f, unsigned char *buf, size_t len)
{
	/* [rosetta 补丁 0005] iovec 表落 **guest 堆**(malloc 经
	 * musl_redirect.h = guest 堆):表原本住 TU 栈 = 宿主地址,而
	 * sentry 只解析 guest VA(表本体与各 iov 缓冲都得在 guest 窗内)。
	 * 语义零改动 —— 仍是同一趟 readv。 */
	struct iovec *iov = malloc(2 * sizeof(struct iovec));
	ssize_t cnt;

	if (!iov) {
		f->flags |= F_ERR;
		return 0;
	}
	iov[0] = (struct iovec){ .iov_base = buf, .iov_len = len - !!f->buf_size };
	iov[1] = (struct iovec){ .iov_base = f->buf, .iov_len = f->buf_size };

	cnt = iov[0].iov_len ? syscall(SYS_readv, f->fd, iov, 2)
		: syscall(SYS_read, f->fd, iov[1].iov_base, iov[1].iov_len);
	free(iov);
	if (cnt <= 0) {
		f->flags |= cnt ? F_ERR : F_EOF;
		return 0;
	}
	if (cnt <= iov[0].iov_len) return cnt;
	cnt -= iov[0].iov_len;
	f->rpos = f->buf;
	f->rend = f->buf + cnt;
	if (f->buf_size) buf[len-1] = *f->rpos++;
	return len;
}
