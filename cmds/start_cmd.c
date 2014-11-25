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

#include "tty_usb.h"
#include <time.h>
static uint8_t cmd[] = {0xA0, 0x0A, 0x50, 0x05};
int start_cmd(tty_usb_handle *h)
{
    int i=0;
    struct timespec t={0, 100000000};

    while(i<sizeof(cmd))
    {
        uint8_t v8;
        tty_usb_w8(h, cmd[i]);
        if((v8 = tty_usb_r8(h)) != (uint8_t)(~cmd[i]))
            i=0;
        else
            i++;
    }

    nanosleep(&t, NULL);
    tty_usb_flush(h);

    return 0;
}
