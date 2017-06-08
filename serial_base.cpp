#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>
#include <termios.h>
#include <errno.h>

#include <android/log.h>
#include <cutils/log.h>

#include "serial_base.h"

#define LOG_TAG "hill"
#define  LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

static int gSerialHandle = -1;
static pthread_mutex_t serial_op_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t serial_r_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t serial_w_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t serial_speed_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t serial_parity_mutex = PTHREAD_MUTEX_INITIALIZER;

static int speed_arr[] = { B115200, B38400, B19200, B9600, B4800, B2400, B1200,
		B300, B38400, B19200, B9600, B4800, B2400, B1200, B300, };
static int name_arr[] = { 115200, 38400, 19200, 9600, 4800, 2400, 1200, 300,
		38400, 19200, 9600, 4800, 2400, 1200, 300, };

static int open_com_dev(int *dev_handle, char *dev_path) {
	if (*dev_handle < 0) {
		*dev_handle = open(dev_path, O_RDWR | O_NOCTTY | O_NDELAY);
		if (*dev_handle < 0) {
			LOGE("%s, Can't Open Serial Port %s", __FUNCTION__, dev_path);
		}
	}
	return *dev_handle;
}

static int close_com_dev(int *dev_handle) {
	if (*dev_handle >= 0) {
		close(*dev_handle);
		*dev_handle = -1;
	}
	return 0;
}

static __inline__ int cfsetdatabits(struct termios *s, int db) {
	if (db == 5) {
		s->c_cflag = (s->c_cflag & ~CSIZE) | (CS5 & CSIZE);
	} else if (db == 6) {
		s->c_cflag = (s->c_cflag & ~CSIZE) | (CS6 & CSIZE);
	} else if (db == 7) {
		s->c_cflag = (s->c_cflag & ~CSIZE) | (CS7 & CSIZE);
	} else if (db == 8) {
		s->c_cflag = (s->c_cflag & ~CSIZE) | (CS8 & CSIZE);
	} else {
		LOGE("%s, Unsupported data size!\n", __FUNCTION__);
	}
	return 0;
}

static __inline__ int cfsetstopbits(struct termios *s, int sb) {
	if (sb == 1) {
		s->c_cflag &= ~CSTOPB;
	} else if (sb == 2) {
		s->c_cflag |= CSTOPB;
	} else {
		LOGE("%s, Unsupported stop bits!\n", __FUNCTION__);
	}
	return 0;
}

static __inline__ int cfsetparity(struct termios *s, int pb) {
	if (pb == 'n' || pb == 'N') {
		s->c_cflag &= ~PARENB; /* Clear parity enable */
		s->c_cflag &= ~INPCK; /* Enable parity checking */
	} else if (pb == 'o' || pb == 'O') {
		s->c_cflag |= (PARODD | PARENB);
		s->c_cflag |= INPCK; /* Disable parity checking */
	} else if (pb == 'e' || pb == 'E') {
		s->c_cflag |= PARENB; /* Enable parity */
		s->c_cflag &= ~PARODD;
		s->c_iflag |= INPCK; /* Disable parity checking */
	} else if (pb == 's' || pb == 'S') {
		s->c_cflag &= ~PARENB;
		s->c_cflag &= ~CSTOPB;
		s->c_cflag |= INPCK; /* Disable parity checking */
	} else {
		LOGE("%s, Unsupported parity!\n", __FUNCTION__);
	}
	return 0;
}

static int gOriAttrGetFlag = 0;
static struct termios gOriAttrValue;
static __inline__ int com_get_attr(int fd, struct termios *s) {
	if (gOriAttrGetFlag == 0) {
		if (tcgetattr(fd, s) != 0) {
			return -1;
		}
		gOriAttrGetFlag = 1;
		gOriAttrValue = *s;
	}
	*s = gOriAttrValue;
	return 0;
}

static int set_com_opt(int hComm, int speed, int db, int sb, int pb, int to,
		int raw_mode) {
	int i = 0;
	struct termios tmpOpt;
	if (com_get_attr(hComm, &tmpOpt) != 0) {
		LOGE("%s, get serial attr error(%s)!\n", __FUNCTION__, strerror(errno));
		return -1;
	}
	for (i = 0; i < sizeof(speed_arr) / sizeof(int); i++) {
		if (speed == name_arr[i]) {
			cfsetispeed(&tmpOpt, speed_arr[i]);
			cfsetospeed(&tmpOpt, speed_arr[i]);
			break;
		}
	}
	cfsetdatabits(&tmpOpt, db);
	cfsetstopbits(&tmpOpt, sb);
	cfsetparity(&tmpOpt, pb);
	if (to >= 0) {
		tmpOpt.c_cc[VTIME] = to; /* 设置超时15 seconds*/
		tmpOpt.c_cc[VMIN] = 0; /* Update the options and do it NOW */
	}
	if (raw_mode == 1) {
		cfmakeraw(&tmpOpt);
	}
	tcflush(hComm, TCIOFLUSH);
	if (tcsetattr(hComm, TCSANOW, &tmpOpt) < 0) {
		LOGE("%s, set serial attr error(%s)!\n", __FUNCTION__, strerror(errno));
		return -1;
	}
	tcflush(hComm, TCIOFLUSH);
	return 0;
}

static int write_com_data(int hComm,unsigned char* pData, unsigned int uLen) {
	unsigned int len;
	if (hComm < 0) {
		return -1;
	}
	if (pData == NULL) {
		return -1;
	}
    int j=0;
    LOGD("write datas:");
	while(j<uLen){
		LOGD("%d\0",pData[j]);
		j++;
	}
	len = write(hComm, pData, uLen);
	if (len == uLen) {
		LOGD("%s, write data success\n", __FUNCTION__);
		return len;
	} else {
		tcflush(hComm, TCOFLUSH);
		LOGE("%s, write data failed and tcflush hComm\n", __FUNCTION__);
		return -1;
	}
	memset(pData,0,sizeof(pData));
}

static int read_com_data(int hComm, unsigned char* pData, unsigned int uLen) {
	unsigned char inbuff[uLen];
	unsigned char buff[uLen];
	unsigned char tempbuff[uLen];
	int i = 0, j = 0;

	memset(inbuff, '\0', uLen);
	memset(buff, '\0', uLen);
	memset(tempbuff, '\0', uLen);

	if (hComm < 0) {
		return -1;
	}

	unsigned char *p = inbuff;

	fd_set readset;
	struct timeval tv;
	int MaxFd = 0;

	unsigned int c = 0;
	int z, k;

	do {
		FD_ZERO(&readset);
		FD_SET(hComm, &readset);
		MaxFd = hComm + 1;
		tv.tv_sec = 0;
		tv.tv_usec = 100000;
		do {
			z = select(MaxFd, &readset, 0, 0, &tv);
		} while (z == -1 && errno == EINTR);
		LOGD("read data select result:%d\n", z);
		if (z == -1) {
			hComm = -1;
			break;
		}

		if (z == 0) {
			hComm = -1;
			break;
		}

		if (FD_ISSET(hComm, &readset)) {
			z = read(hComm, buff, uLen - c);
#if 0
			for (k = 0; k < z; k++) {
				LOGD("%s, inbuff[%d]:%02X", __FUNCTION__, k, buff[k]);
			}
#endif
			c += z;

			if (z > 0) {
				if (z < (signed int) uLen) {
					buff[z + 1] = '\0';
					memcpy(p, buff, z);
					p += z;
				} else {
					memcpy(inbuff, buff, z);
				}

				memset(buff, '\0', uLen);
			} else {
				hComm = -1;
			}

			if (c >= uLen) {
				hComm = -1;
				break;
			}
		}
	} while (hComm >= 0);
	memcpy(pData, inbuff, c);
	int f=0;
	LOGD("read datas:", z);
	if(f<uLen){
		LOGD("%d\0", pData[f]);
		f++;
	}
	p = NULL;
	return c;
}

int com_open_dev(char *devpath) {
	int tmp_ret = 0;

	pthread_mutex_lock(&serial_op_mutex);

	tmp_ret = open_com_dev(&gSerialHandle, devpath);

	pthread_mutex_unlock(&serial_op_mutex);

	return tmp_ret;
}

int com_close_dev(int fp) {
	int tmp_ret = 0;

	pthread_mutex_lock(&serial_op_mutex);

	tmp_ret = close_com_dev(&fp);

	pthread_mutex_unlock(&serial_op_mutex);

	return tmp_ret;
}

int com_get_dev() {
	int tmp_ret = 0;

	pthread_mutex_lock(&serial_op_mutex);

	tmp_ret = gSerialHandle;

	pthread_mutex_unlock(&serial_op_mutex);

	return tmp_ret;
}

int com_set_opt(int fp, int speed, int db, int sb, int pb, int to,
		int raw_mode) {
	int tmp_ret = 0;
	pthread_mutex_lock(&serial_parity_mutex);

	if (com_get_dev() < 0) {
		pthread_mutex_unlock(&serial_parity_mutex);
		return -1;
	}
	tmp_ret = set_com_opt(fp, speed, db, sb, pb, to, raw_mode);
	pthread_mutex_unlock(&serial_parity_mutex);
	return tmp_ret;
}

int com_write_data(int fp,unsigned char* pData, unsigned int uLen) {
	int tmp_ret = 0;
	pthread_mutex_lock(&serial_w_mutex);
	if (com_get_dev() < 0) {
		pthread_mutex_unlock(&serial_w_mutex);
		return -1;
	}
	LOGD("%s, write %d bytes\n", __FUNCTION__, uLen);
	tmp_ret = write_com_data(fp, pData, uLen);
	pthread_mutex_unlock(&serial_w_mutex);
	return tmp_ret;
}

int com_read_data(int fp, unsigned char* pData, unsigned int uLen) {
	int tmp_ret = 0;
	pthread_mutex_lock(&serial_r_mutex);
	if (com_get_dev() < 0) {
		pthread_mutex_unlock(&serial_r_mutex);
		return -1;
	}
	tmp_ret = read_com_data(fp, pData, uLen);
	LOGD("%s, read %d bytes\n", __FUNCTION__, uLen);
	pthread_mutex_unlock(&serial_r_mutex);
	return tmp_ret;
}
