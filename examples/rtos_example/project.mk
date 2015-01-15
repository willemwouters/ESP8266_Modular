#
# Copyright (c) 2015, Lambertus Gorter <l.gorter@gmail.com>
# All rights reserved.
#
# The project configuration
#

# What to build
BUILD_TYPE      ?= debug
PROJECT_MODULES := rtos_example
SDK_MODULES     := esp_rtos/lwip esp_rtos/espressif esp_rtos/freertos esp_rtos/udhcp  xtensa esp_rtos/core esp_rtos/linker
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

