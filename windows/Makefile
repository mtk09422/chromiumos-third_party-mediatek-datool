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

OUT = out
TARGET = $(OUT)/fbtool.exe

INCLUDES = /I..\inc /I..\inc\win

SOURCES = \
	..\fbtool.c \
	..\cmds\start_cmd.c \
	..\cmds\get_hw_code.c \
	..\cmds\get_target_config.c \
	..\cmds\send_auth.c \
	..\cmds\send_da.c \
	..\cmds\jump_da.c \
	..\utils\file_util.c \
	..\utils\tty_usb_win.c \
	..\utils\tty_usb_common.c \
	..\utils\util_win.c

OBJS =

LIBS = Ws2_32.lib

CC = cl.exe /nologo

all: $(TARGET)

clean:
	-rmdir /S /Q $(OUT)

$(TARGET): $(SOURCES)
	-mkdir $(OUT)
	$(CC) $(INCLUDES) /Fo$(OUT)/ /Fe$@ $** $(LIBS)
