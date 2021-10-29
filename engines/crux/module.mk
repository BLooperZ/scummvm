MODULE := engines/crux

MODULE_OBJS = \
	crux.o \
	detection.o \
	metaengine.o

# This module can be built as a plugin
ifeq ($(ENABLE_CRUX), DYNAMIC_PLUGIN)
PLUGIN := 1
endif

# Include common rules
include $(srcdir)/rules.mk

# Detection objects
DETECT_OBJS += $(MODULE)/detection.o
