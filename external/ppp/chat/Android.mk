ifeq ($(TARGET_ARCH),arm)

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= chat.c

CDEF1=	-DTERMIOS			# Use the termios structure
CDEF2=	-DSIGTYPE=void			# Standard definition
CDEF3=	-UNO_SLEEP			# Use the usleep function
CDEF4=	-DFNDELAY=O_NDELAY		# Old name value
CDEFS=	$(CDEF1) $(CDEF2) $(CDEF3) $(CDEF4)

COPTS=	-O2 -g -pipe
LOCAL_CFLAGS := $(COPTS) $(CDEFS)

LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)
LOCAL_MODULE_TAGS := eng
LOCAL_MODULE:= chat

include $(BUILD_EXECUTABLE)

endif
