
include $(TKS_ROOT)/install_msvc.mk

ifeq ($(TARGET_DIR),)
TARGET_DIR:=$(TKS_ROOT)/tks-projects/eureka/voice_plugins/
endif

EXTRALIBS += -DLL -MAP 
