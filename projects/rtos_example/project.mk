#
# Copyright (c) 2015, Lambertus Gorter <l.gorter@gmail.com>
# All rights reserved.
#
# The project configuration
#

# What to build
BUILD_TYPE      ?= debug
PROJECT_MODULES := blinky
SDK_MODULES     := bsp/esp_rtos/lwip bsp/esp_rtos/espressif bsp/esp_rtos/freertos bsp/esp_rtos/udhcp  bsp/xtensa bsp/esp_rtos/core bsp/esp_rtos/linker
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

