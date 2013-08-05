LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_PRELINK_MODULE := false


LOCAL_SRC_FILES := \
			main.c \
			capture/capture.c \
			decode/decode_api.c \
			render/render.c \

			
LOCAL_C_INCLUDES := $(KERNEL_HEADERS) \
			$(LOCAL_PATH) \
			$(LOCAL_PATH)/capture \
			$(LOCAL_PATH)/decode \
			$(LOCAL_PATH)/render \
			$(LOCAL_PATH)/include \

LOCAL_LDFLAGS += -L$(LOCAL_PATH)

LOCAL_SHARED_LIBRARIES := libcutils libstdc++ libc \
			libutils \
			liblog \

LOCAL_LDLIBS := -ldl 
LOCAL_CFLAGS += -Wfatal-errors -fno-short-enums -D__OS_ANDROID
#LOCAL_CFLAGS += -fno-exceptions
#LOCAL_CFLAGS += -lstdc++
LOCAL_ARM_MODE := arm	

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := enc_test

#LOCAL_SHARED_LIBRARIES := libcutils libsmscontrol
												
LOCAL_LDFLAGS = $(LOCAL_PATH)/android_lib/libh264enc.a \
		$(LOCAL_PATH)/android_lib/libcedarxosal.a \
		$(LOCAL_PATH)/android_lib/libcedarxalloc.a \
		$(LOCAL_PATH)/android_lib/libcedarv.a \

include $(BUILD_EXECUTABLE)
