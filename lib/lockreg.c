#include "common_define.h"
#include "upgrade.h"

int32_t lock_reg(int32_t fd, int32_t cmd, int32_t type, off_t offset, int32_t whence, off_t len)
{
	struct flock lock;

	lock.l_type = type; /* F_RDLCK, F_WRLCK, F_UNLCK */
	lock.l_start = offset; /* byte offset, relative to l_whence */
	lock.l_whence = whence; /* SEEK_SET, SEEK_CUR, SEEK_END */
	lock.l_len = len; /* #bytes (0 means to EOF) */

	return (fcntl(fd, cmd, &lock));
}
