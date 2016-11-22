LOCAL_PATH:= $(call my-dir)

# before build
####################################################################################
# ffmpeg's library
include $(CLEAR_VARS)
LOCAL_MODULE    := avformat
LOCAL_SRC_FILES := ./ffmpeg/libavformat.a
include $(PREBUILT_STATIC_LIBRARY) 

include $(CLEAR_VARS)
LOCAL_MODULE    := avcodec
LOCAL_SRC_FILES := ./ffmpeg/libavcodec.a
include $(PREBUILT_STATIC_LIBRARY) 

include $(CLEAR_VARS)
LOCAL_MODULE    := avutil
LOCAL_SRC_FILES := ./ffmpeg/libavutil.a
include $(PREBUILT_STATIC_LIBRARY) 

include $(CLEAR_VARS)
LOCAL_MODULE    := swscale
LOCAL_SRC_FILES := ./ffmpeg/libswscale.a
include $(PREBUILT_STATIC_LIBRARY) 


####################################################################################
# pjproject's library
include $(CLEAR_VARS)
LOCAL_MODULE    := pj
LOCAL_SRC_FILES := ./pjproject-2.3/lib/libpj-arm-unknown-linux-androideabi.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := pjlib
LOCAL_SRC_FILES := ./pjproject-2.3/lib/libpjlib-util-arm-unknown-linux-androideabi.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := pjnath
LOCAL_SRC_FILES := ./pjproject-2.3/lib/libpjnath-arm-unknown-linux-androideabi.a
include $(PREBUILT_STATIC_LIBRARY)


####################################################################################
# udtc's library
include $(CLEAR_VARS)
LOCAL_MODULE    := udt
LOCAL_SRC_FILES := ./ffmpeg/libudt4.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := udtc
LOCAL_SRC_FILES := ./ffmpeg/libudtc.a
include $(PREBUILT_STATIC_LIBRARY)


####################################################################################
# build configure.

include $(CLEAR_VARS)

LOCAL_CFLAGS := -D__STDC_CONSTANT_MACROS -Wno-sign-compare -Wno-switch -Wno-pointer-sign -DHAVE_NEON=1 \
      -mfpu=neon -mfloat-abi=softfp -fPIC -DANDROID \
      -DLINUX -finline-functions -fexceptions -fno-strict-aliasing \
      -g -static -Wall -Wextra # -fvisibility=hidden 
      
LOCAL_C_INCLUDES := $(LOCAL_PATH)/ffmpeg/include \
					$(LOCAL_PATH)/pjproject-2.3/include \
					$(LOCAL_PATH)/udtc/jni/udtc

LOCAL_WHOLE_STATIC_LIBRARIES := libstlport_static
				
LOCAL_LDLIBS := -L$(NDK_PLATFORMS_ROOT)/$(TARGET_PLATFORM)/arch-arm/usr/lib \
				-llog -ljnigraphics -lz -ldl -lstdc++ -lm
#	-lnice -lrt -lz -ldl -lresolv -lm -lpthread -static

LOCAL_STATIC_LIBRARIES := avformat avcodec swscale avutil \
						udtc udt pjnath pjlib pj 

LOCAL_SRC_FILES :=	clientNetlib.c interface.c playlist.c playlib.c p2p_cross.c

LOCAL_MODULE    := netplayerlib

include $(BUILD_SHARED_LIBRARY)