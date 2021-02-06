
include ../../../../install_msvc.mk

ifeq ($(TARGET_DIR),)
TARGET_DIR:=/f/Projects/tks-projects/eureka/voice_plugins/
endif

EXTRALIBS += -DLL -MAP 
