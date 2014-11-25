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

#include <Windows.h>
#include <Setupapi.H>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <string.h>
#include "tty_usb.h"
#pragma comment (lib, "Setupapi.lib")

#include <initguid.h>
DEFINE_GUID(GUID_DEVINTERFACE_USB_DEVICE, 0xA5DCBF10L, 0x6530, 0x11D2, 0x90, 0x1F, 0x00, 0xC0, 0x4F, 0xB9, 0x51, 0xED);

struct tty_usb_handle
{
    HANDLE com;
};

static tty_usb_handle *find_usb_device(ifc_match_func callback)
{
    HDEVINFO set;
    SP_DEVINFO_DATA info={0};
    DWORD i;
    tty_usb_handle *ret_h=NULL;

    set = SetupDiGetClassDevs(&GUID_DEVINTERFACE_USB_DEVICE, 
                              NULL, 
                              NULL, 
                              DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
    if(set==INVALID_HANDLE_VALUE) return NULL;

    info.cbSize = sizeof(SP_DEVINFO_DATA);
    for(i=0; SetupDiEnumDeviceInfo(set, i, &info); i++)
    {
        DWORD sz, r_sz;
        int j;
        char* buf;
	uint16_t vid=0, pid=0;
        usb_ifc_info usb_info;
        SP_DEVICE_INTERFACE_DATA dif={0};
        
        // get id size
        if(SetupDiGetDeviceInstanceId(set, &info, NULL, 0, &sz) ||
           (ERROR_INSUFFICIENT_BUFFER != GetLastError()))
        {
            // it should never happen
            continue;
        }

        // get id
        buf = (char*)malloc(sz);
        memset(buf, 0, sz);
        if(!SetupDiGetDeviceInstanceId(set, &info, buf, sz, &r_sz))
        {
            free(buf);
            continue;
        }

        // get vid/pid
        sscanf(buf, "USB\\VID_%04hx&PID_%04hx", &vid, &pid);
        usb_info.vid = vid;
        usb_info.pid = pid;
        free(buf);

        // match
        if(callback(&usb_info)!=0)
            continue;

        dif.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
        for(j=0; 
            SetupDiEnumDeviceInterfaces(set, &info, 
                                        &GUID_DEVINTERFACE_USB_DEVICE,
                                        j, &dif);
            j++)
        {
            SP_DEVICE_INTERFACE_DETAIL_DATA *dtl;
            HANDLE tmp_com;

            // get sym link size
            if(SetupDiGetDeviceInterfaceDetail(set, &dif, NULL, 0, &sz, &info)
               || (ERROR_INSUFFICIENT_BUFFER != GetLastError()))
            {
                // it should never happen
                continue;
            }

            // get sym link
            dtl = (SP_DEVICE_INTERFACE_DETAIL_DATA*)malloc(sz);
            memset(dtl, 0, sz);
            dtl->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
            if(!SetupDiGetDeviceInterfaceDetail(set, &dif, dtl, sz, 
                                                &r_sz, &info))
            {
                free(dtl);
                continue;
            }

            // open tty comport
            tmp_com = CreateFile(dtl->DevicePath,
                                  GENERIC_READ | GENERIC_WRITE,
                                  0,
                                  NULL,
                                  OPEN_EXISTING,
                                  FILE_ATTRIBUTE_NORMAL,
                                  NULL);
            if(tmp_com == INVALID_HANDLE_VALUE)
            {
                free(dtl);
                continue;
            }

            printf("got %s\n", dtl->DevicePath);
            free(dtl);
            ret_h = (tty_usb_handle*)malloc(sizeof(tty_usb_handle));
            ret_h->com = tmp_com;
            break;
        }

        if(ret_h != NULL)
            break;
    }

    if(set)
    {
        SetupDiDestroyDeviceInfoList(set);
    }

    return ret_h;
}

static void _set_dcb(HANDLE com)
{
        DCB dcb;

        ZeroMemory(&dcb, sizeof(dcb));
        dcb.DCBlength = sizeof(dcb);
        dcb.BaudRate = CBR_115200;
        dcb.fBinary = TRUE;
        dcb.fDtrControl = DTR_CONTROL_ENABLE;
        dcb.fRtsControl = RTS_CONTROL_ENABLE;
        dcb.ByteSize = 8;
        if (!SetCommState(com, &dcb))
                exit(EXIT_FAILURE);
}

tty_usb_handle *tty_usb_open(ifc_match_func callback)
{
    tty_usb_handle *h=NULL;
    while(NULL == (h = find_usb_device(callback)))
    {
        Sleep(100);
    }
    _set_dcb(h->com);

    return h;
}

size_t tty_usb_read(tty_usb_handle *h, void *data, size_t len)
{
    DWORD NumberOfBytesRead;
    BOOL ret = ReadFile(h->com, data, len, &NumberOfBytesRead, NULL);
    if(!ret || (NumberOfBytesRead != len)) exit(EXIT_FAILURE);
    return NumberOfBytesRead;
}

#define BUF_MAX 1024
size_t tty_usb_write(tty_usb_handle *h, const void *data, size_t len)
{
    DWORD NumberOfBytesWritten;
    size_t r_len = 0;
    BOOL ret;
    uint8_t* p = (uint8_t*)data;
    while(len > 0)
    {
        size_t sz;
        if(len>BUF_MAX) sz=BUF_MAX;
        else sz=len;

        ret = WriteFile(h->com, p, sz, &NumberOfBytesWritten, NULL);
        if(!ret || (NumberOfBytesWritten != sz)) exit(EXIT_FAILURE);

        len   = len   - sz;
        p     = p     + sz;
        r_len = r_len + sz;
    }
    return r_len;
}

void tty_usb_close(tty_usb_handle *h)
{
    CloseHandle(h->com);
    free(h);
}

#define DFT_FLAG (PURGE_RXABORT|PURGE_RXCLEAR|PURGE_TXABORT|PURGE_TXCLEAR)
void tty_usb_flush(tty_usb_handle *h)
{
    if(!FlushFileBuffers(h->com))
        exit(EXIT_FAILURE);

    if(!PurgeComm(h->com, DFT_FLAG))
        exit(EXIT_FAILURE);
}

