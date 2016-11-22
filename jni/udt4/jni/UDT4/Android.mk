LOCAL_PATH		:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_ARM_MODE	:= arm
LOCAL_CPPFLAGS	:= -fexceptions	-fPIC -Wall -Wextra -DLINUX -finline-functions -O3 \
					-fno-strict-aliasing -fvisibility=hidden
					
LOCAL_WHOLE_STATIC_LIBRARIES := libstlport_static

LOCAL_C_INCLUDES := -L$(NDK_PLATFORMS_ROOT)/sources/cxx-stl/gnu-libstdc++/4.6/include/

LOCAL_MODULE := udt4
LOCAL_SRC_FILES := md5.cpp common.cpp window.cpp list.cpp buffer.cpp packet.cpp channel.cpp \
				queue.cpp ccc.cpp cache.cpp core.cpp epoll.cpp api.cpp 

include $(BUILD_STATIC_LIBRARY)