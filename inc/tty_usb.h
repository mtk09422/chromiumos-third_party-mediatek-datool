/*
 * Copyright (c) 2014 MediaTek Inc.
 * Author: Tristan Shieh <tristan.shieh@mediatek.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef _TTY_USB_H_
#define _TTY_USB_H_

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

typedef struct tty_usb_handle tty_usb_handle;
typedef struct usb_ifc_info
{
    uint16_t vid;
    uint16_t pid;
}usb_ifc_info;
typedef int (*ifc_match_func)(usb_ifc_info *ifc);

// api @ tty_usb_[win/linux/mac].c
extern tty_usb_handle *tty_usb_open(ifc_match_func callback);
extern size_t tty_usb_read(tty_usb_handle *h, void *data, size_t len);
extern size_t tty_usb_write(tty_usb_handle *h, const void *data, size_t len);
extern void tty_usb_close(tty_usb_handle *h);
extern void tty_usb_flush(tty_usb_handle *h);

// common api @ tty_usb_common.c
extern tty_usb_handle *tty_usb_open_pl(void);
extern tty_usb_handle *tty_usb_open_br(void);
extern tty_usb_handle *tty_usb_open_auto(void);
extern bool tty_usb_is_target_brom(void);

extern void tty_usb_w8(tty_usb_handle *h, uint8_t data);
extern uint8_t tty_usb_r8(tty_usb_handle *h);
extern int tty_usb_w8_echo(tty_usb_handle *h, uint8_t data);

extern void tty_usb_w16(tty_usb_handle *h, uint16_t data);
extern uint16_t tty_usb_r16(tty_usb_handle *h);
extern int tty_usb_w16_echo(tty_usb_handle *h, uint16_t data);

extern void tty_usb_w32(tty_usb_handle *h, uint32_t data);
extern uint32_t tty_usb_r32(tty_usb_handle *h);
extern int tty_usb_w32_echo(tty_usb_handle *h, uint32_t data);


#endif // _TTY_USB_H_

