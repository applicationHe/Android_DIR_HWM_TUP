LOCAL_PATH		:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CPPFLAGS	:= -DLINUX -finline-functions -fexceptions -fno-strict-aliasing \
		-fvisibility=hidden -fPIC -g -static -Wall -Wextra
	
LOCAL_WHOLE_STATIC_LIBRARIES := libstlport_static

LOCAL_C_INCLUDES := $(NDK_PLATFORMS_ROOT)/sources/cxx-stl/gnu-libstdc++/4.6/include/

LOCAL_LDLIBS	:= -llog -lstdc++ -lm

LOCAL_MODULE := udtc

LOCAL_SRC_FILES := udtcLib.cpp

include $(BUILD_STATIC_LIBRARY)