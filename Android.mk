LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE    := libComPortjni
LOCAL_SRC_FILES := SerialPortInterface.cpp
LOCAL_SRC_FILES += serial_base.cpp
LOCAL_LDLIBS    := -lm -llog
LOCAL_C_INCLUDES  += system/core/include/cutils
LOCAL_SHARED_LIBRARIES := libcutils
include $(BUILD_SHARED_LIBRARY)

