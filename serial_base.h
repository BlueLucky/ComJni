#ifndef __SERIAL_BASE_H__
#define __SERIAL_BASE_H__

extern int com_open_dev(char *devpath);
extern int com_close_dev(int fp);
extern int com_get_dev();
extern int com_set_opt(int fp, int speed, int db, int sb, int pb, int timeout,
		int raw_mode);
extern int com_write_data(int fp,unsigned char* pData, unsigned int uLen);
extern int com_read_data(int fp, unsigned char* pData, unsigned int uLen);

#endif//__SERIAL_BASE_H__
