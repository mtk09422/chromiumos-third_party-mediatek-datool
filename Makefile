#
# Copyright (c) 2014 MediaTek Inc.
# Author: Tristan Shieh <tristan.shieh@mediatek.com>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#

TARGET := fbtool

OSTYPE := $(shell uname)

SOURCES := \
	fbtool.c\
	cmds/start_cmd.c \
	cmds/get_hw_code.c \
	cmds/get_target_config.c \
	cmds/send_auth.c \
	cmds/send_da.c \
	cmds/jump_da.c \
	utils/file_util.c \
	utils/tty_usb_common.c

ifeq ($(OSTYPE), Linux)
	SOURCES += utils/tty_usb_linux.c
else
	SOURCES += utils/tty_usb_osx.c
endif

ifeq ($(OSTYPE), Linux)
	CFLAGS += -D LINUX
endif

ifeq ($(OSTYPE), Darwin)
	CFLAGS += -framework CoreFoundation -framework IOKit
endif

CFLAGS += -Iinc

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SOURCES)

clean:
	$(RM) $(TARGET)
