#
# Copyright (c) 2015, Lambertus Gorter <l.gorter@gmail.com>
# All rights reserved.
#
# The project configuration
#

# What to build
BUILD_TYPE      ?= debug
PROJECT_MODULES := iot_demo driver 
SDK_MODULES     := core json linker lwip ssl upgrade
SDK_MODULES := $(addprefix sdk_9_4/,$(SDK_MODULES)) xtensa

OTHER_MODULES   :=


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

