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

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := fbtool

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := EXECUTABLES

LOCAL_SRC_FILES := \
	fbtool.c \
	cmds/start_cmd.c \
	cmds/get_hw_code.c \
	cmds/get_target_config.c \
	cmds/send_auth.c \
	cmds/send_da.c \
	cmds/jump_da.c \
	utils/file_util.c \
	utils/tty_usb_common.c

LOCAL_C_INCLUDES := $(LOCAL_PATH)/inc

ifeq ($(HOST_OS),windows)
LOCAL_SRC_FILES += utils/tty_usb_win.c utils/util_win.c
LOCAL_C_INCLUDES += $(LOCAL_PATH)/inc/win
LOCAL_LDLIBS += -lws2_32
endif

ifeq ($(HOST_OS),linux)
LOCAL_SRC_FILES += utils/tty_usb_linux.c
endif

ifeq ($(HOST_OS),darwin)
LOCAL_SRC_FILES += utils/tty_usb_osx.c
LOCAL_LDLIBS += -framework CoreFoundation -framework IOKit
endif

include $(BUILD_HOST_EXECUTABLE)
