#include "stdio_impl.h"
#include <sys/uio.h>

size_t __stdio_write(FILE *f, const unsigned char *buf, size_t len)
{
	/* [rosetta 补丁 0005] iovec 表落 **guest 堆**(同 __stdio_read:
	 * 表住 TU 栈 = 宿主地址,sentry 只解析 guest VA)。语义零改动。 */
	struct iovec *iovs = malloc(2 * sizeof(struct iovec));
	struct iovec *iov = iovs;
	if (!iovs) {
		f->wpos = f->wbase = f->wend = 0;
		f->flags |= F_ERR;
		return 0;
	}
	iovs[0] = (struct iovec){ .iov_base = f->wbase, .iov_len = f->wpos-f->wbase };
	iovs[1] = (struct iovec){ .iov_base = (void *)buf, .iov_len = len };
	size_t rem = iov[0].iov_len + iov[1].iov_len;
	int iovcnt = 2;
	ssize_t cnt;

	if (!iov->iov_len) {
		iov++;
		iovcnt--;
	}
	for (;;) {
		cnt = syscall(SYS_writev, f->fd, iov, iovcnt);
		if (cnt == rem) {
			f->wend = f->buf + f->buf_size;
			f->wpos = f->wbase = f->buf;
			free(iovs);
			return len;
		}
		if (cnt < 0) {
			f->wpos = f->wbase = f->wend = 0;
			f->flags |= F_ERR;
			free(iovs);
			return iovcnt == 2 ? 0 : len-iov[0].iov_len;
		}
		rem -= cnt;
		if (cnt > iov[0].iov_len) {
			cnt -= iov[0].iov_len;
			iov++; iovcnt--;
		}
		iov[0].iov_base = (char *)iov[0].iov_base + cnt;
		iov[0].iov_len -= cnt;
	}
}
