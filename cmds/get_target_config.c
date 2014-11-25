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

#include <stdint.h>
#include "tty_usb.h"
#define E_ERROR 0x1000
int get_target_config(tty_usb_handle *h, uint32_t* pdata)
{
    uint16_t status;

    if(tty_usb_w8_echo(h, 0xD8)!=0) return -1;
    *pdata = tty_usb_r32(h);
    status = tty_usb_r16(h);
    if(E_ERROR <= status) return status;

    return 0;
}
