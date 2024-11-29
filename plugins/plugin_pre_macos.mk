
include $(TKS_ROOT)/install_macos.mk

ifeq ($(TARGET_DIR),)
TARGET_DIR:=$(TKS_ROOT)/tks-projects/eureka/voice_plugins/
endif

CFLAGS+= -Wno-unused-variable -Wno-unused-function
CPPFLAGS+= -Wno-unused-variable -Wno-unused-function
EXTRALIBS+=

