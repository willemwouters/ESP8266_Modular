#
# Copyright (c) 2015, Lambertus Gorter <l.gorter@gmail.com>
# All rights reserved.
#

include module.mk

SOURCES     := $(wildcard src/api/*.c)
SOURCES     += $(wildcard src/app/*.c)
SOURCES     += $(wildcard src/api/*.c)
SOURCES     += $(wildcard src/core/*.c)
SOURCES     += $(wildcard src/core/ipv4/*.c)
SOURCES     += $(wildcard src/netif/*.c)

LIB         := $(LIBDIR)/lib$(MODULE_NAME).a

OBJS        := $(patsubst %.c, $(MODULE_BUILD_PATH)/%.o, $(SOURCES))
DEPS        := $(patsubst %.o, %.d, $(OBJS))


module: $(LIB)

$(LIB): $(OBJS)
	mkdir -p $(dir $@)
	$(AR) cru $@ $^


-include $(DEPS)

$(MODULE_BUILD_PATH)/%.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_PATH)

.PHONY: clean

