#
# Copyright (c) 2015, Lambertus Gorter <l.gorter@gmail.com>
# All rights reserved.
#
# The project configuration
#

# What to build
BUILD_TYPE      ?= debug
PROJECT_MODULES := user
SDK_MODULES     := lwip espressif freertos udhcp core linker
SDK_MODULES := $(addprefix sdk_rtos/,$(SDK_MODULES))
OTHER_MODULES   := 
INCDIR  += $(abspath modules/xtensa/include)

# Build types
ifeq ($(BUILD_TYPE),debug)
CFLAGS        := -Og -g -Wpointer-arith -Wundef -Werror 
LDFLAGS       := 
else ifeq ($(BUILD_TYPE),release)
CFLAGS        := -O2 -Wpointer-arith -Wundef -Werror
LDFLAGS       := 
else
$(error build type $(BUILD_TYPE) not supported)
endif

