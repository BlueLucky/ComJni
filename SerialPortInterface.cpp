#include <jni.h>
#include <android/log.h>
#include <serial_base.h>
#define LOG_TAG "serialport"
#define CC_MAX_SERIAL_RD_BUF_LEN   (512)
#define FIFO_CLEAR 0x01
#define BUFFER_LEN 20

#include <string.h>
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

#ifndef _Included_com_sky_serialport_SerialPortInterface
#define _Included_com_sky_serialport_SerialPortInterface
#ifdef __cplusplus
extern "C" {
#endif
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#define  LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,__VA_ARGS__)

JNIEXPORT jint JNICALL Java_com_open_SPinterface_SerialPortInterface_comInit(
		JNIEnv *env, jobject jobj, jstring devpath) {
	LOGD("Java_com_sky_serialport_SerialPortInterface_comInit");
	char *buff = (char *) env->GetStringUTFChars(devpath, NULL);
	int handle = com_open_dev(buff);
	LOGD("com_open_dev handle:%d\n", handle);
	return handle;
}

JNIEXPORT jint JNICALL Java_com_open_SPinterface_SerialPortInterface_comGet(JNIEnv *,
		jobject, jint speed){
	LOGD("Java_com_sky_serialport_SerialPortInterface_comGet");
	int handle = com_get_dev();
	LOGD("open_dev handle:%d\n,speed:%d",handle,speed);
	if (handle >= 0) {
		if (com_set_opt(handle, speed, 8, 1, 'N', 150, 1) < 0) {
			LOGD("%s, Set serial opt Error!\n", "com_set_opt");
			return -1;
		}
	}
	return handle;
}
JNIEXPORT jint JNICALL Java_com_open_SPinterface_SerialPortInterface_comWrite(
		JNIEnv *env, jobject jobj, jint fp, jbyteArray cmd, jint len) {
	LOGD("Java_com_sky_serialport_SerialPortInterface_comWrite");
	jbyte* byteArr = env->GetByteArrayElements(cmd, 0);
	jsize alen = env->GetArrayLength(cmd);
	unsigned char* buffer = (unsigned char*)malloc(len*sizeof(char));
	memset(buffer, '\0', len);
	int j = 0;
	while (j < len) {
		LOGD("Java_com_sky_serialport_SerialPortInterface_comWrite:%d",
				byteArr[j]);
		j++;
	}
	memcpy(buffer,byteArr,len);
	int i = 0;
	while (i < len) {
		LOGD("Java_com_sky_serialport_SerialPortInterface_comWrite:%d",buffer[i]);
		i++;
	}
	int res = com_write_data(fp, buffer, len);
	env->ReleaseByteArrayElements(cmd, byteArr, 0);
	delete[] buffer;
	return res;
}

JNIEXPORT jbyteArray JNICALL Java_com_open_SPinterface_SerialPortInterface_comRead(
		JNIEnv *env, jobject jobj, jint fp) {
	LOGD("Java_com_sky_serialport_SerialPortInterface_comRead");
	int nread = 0;
	unsigned char buff[512] = { 0 };
	nread = com_read_data(fp, buff, CC_MAX_SERIAL_RD_BUF_LEN);

	jbyteArray bytes = env->NewByteArray(nread);
	env->SetByteArrayRegion(bytes, 0, nread, (jbyte*) buff);
	return bytes;
}

JNIEXPORT jint JNICALL Java_com_open_SPinterface_SerialPortInterface_comExt(
		JNIEnv *env, jobject jobj, jint fp) {
	LOGD("Java_com_sky_serialport_SerialPortInterface_comExt");
	com_close_dev(fp);
	return 0;
}
#ifdef __cplusplus
}
#endif
#endif
