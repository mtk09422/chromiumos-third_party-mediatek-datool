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

#include <CoreFoundation/CoreFoundation.h>
#include <CoreFoundation/CFUUID.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/usb/IOUSBLib.h>
#include <IOKit/IOCFPlugIn.h>
#include <IOKit/IOMessage.h>
#include <IOKit/usb/USB.h>
#include <IOKit/serial/IOSerialKeys.h>
#include <mach/mach_port.h>

#include <sys/types.h>
#include <sys/uio.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <termios.h>
#include <unistd.h>
#include "tty_usb.h"

#define PATH_USBDEV "/sys/bus/usb/devices"

struct tty_usb_handle
{
    int fd;
};

static int find_dev_node_path(io_service_t dev, char *path, size_t len)
{
    CFTypeRef bsdPathAsCFString;
    bsdPathAsCFString = IORegistryEntryCreateCFProperty(dev,
                                                        CFSTR(kIOCalloutDeviceKey),
                                                        kCFAllocatorDefault,
                                                        0);
    if (!bsdPathAsCFString)
    {
        return -1;
    }
    
    if(!CFStringGetCString(bsdPathAsCFString,
                           path,
                           len,
                           kCFStringEncodingUTF8))
    {
        CFRelease(bsdPathAsCFString);
        return -1;
    }
    
    CFRelease(bsdPathAsCFString);
    return 0;
}

static int get_ifc_from_dev(io_service_t device, usb_ifc_info *ifc)
{
    kern_return_t kr;
    IOCFPlugInInterface **p = NULL;
    SInt32 s;
    HRESULT r;
    IOUSBDeviceInterface182 **dev = NULL;
    uint16_t idv=0, idp=0;

    kr = IOCreatePlugInInterfaceForService(device, kIOUSBDeviceUserClientTypeID, kIOCFPlugInInterfaceID, &p, &s);
    if((kr != 0) || (p == NULL))
    {
        goto error;
    }

    r = (*p)->QueryInterface(p, CFUUIDGetUUIDBytes(kIOUSBDeviceInterfaceID), (LPVOID)&dev);
    if((r != 0) || (dev == NULL))
    {
        goto error;
    }

    IODestroyPlugInInterface(p);

    kr = (*dev)->GetDeviceVendor(dev, &idv);
    if (kr != 0) {
        goto error;
    }

    kr = (*dev)->GetDeviceProduct(dev, &idp);
    if (kr != 0) {
        goto error;
    }

    ifc->vid = idv;
    ifc->pid = idp;
    return 0;

    error :
    return -1;
}

static int trace_parent_to_get_ifc(io_service_t n, usb_ifc_info *ifc)
{
    if(get_ifc_from_dev(n, ifc) == 0)
    {
        return 0;
    }
    else
    {
        int r;
        io_service_t p;
        kern_return_t kr;
        
        kr = IORegistryEntryGetParentEntry(n, kIOServicePlane, &p);
        if(kr != KERN_SUCCESS) return -1;
        
        r = trace_parent_to_get_ifc(p, ifc);
        IOObjectRelease(p);
        
        return r;
    }
}

static tty_usb_handle *find_usb_device(ifc_match_func callback)
{
    tty_usb_handle *h = NULL;
    CFMutableDictionaryRef m;
    io_iterator_t i;
    kern_return_t kr;

    m = IOServiceMatching(kIOSerialBSDServiceValue);
    if(m == NULL)
    {
        return NULL;
    }

    kr = IOServiceGetMatchingServices(kIOMasterPortDefault, m, &i);
    if(kr != KERN_SUCCESS)
    {
        return NULL;
    }

    while(h == NULL)
    {
        io_service_t dev;
        usb_ifc_info ifc;
        char path[PATH_MAX];
        int fd;

        if(!IOIteratorIsValid(i))
        {
            IOIteratorReset(i);
            continue;
        }
        
        dev = IOIteratorNext(i);
        if(dev == 0) break;

        if(find_dev_node_path(dev, path, PATH_MAX) != 0)
        {
            IOObjectRelease(dev);
            continue;
        }
        
        if(trace_parent_to_get_ifc(dev, &ifc) != 0)
        {
            IOObjectRelease(dev);
            continue;
        }

        IOObjectRelease(dev);

        if(callback(&ifc) != 0) continue;
        if((fd = open(path, O_RDWR)) < 0) continue;

        printf("got %s\n", path);
        h = (tty_usb_handle*)malloc(sizeof(tty_usb_handle));
        h->fd = fd;
    }
    IOObjectRelease(i);

    return h;
}

static void _set_tio(int fd)
{
    struct termios tio;
    memset(&tio, 0, sizeof(tio));
    tcgetattr(fd, &tio);
    cfsetospeed(&tio, B115200);
    cfsetispeed(&tio, B115200);
    tcsetattr(fd, TCSANOW, &tio);
}

tty_usb_handle *tty_usb_open(ifc_match_func callback)
{
    tty_usb_handle *h = NULL;
    while(NULL == (h = find_usb_device(callback)))
    {
        struct timespec t = {0, 100000000};
        nanosleep(&t, NULL);
    }
    _set_tio(h->fd);
    return h;
}

size_t tty_usb_read(tty_usb_handle *h, void *data, size_t len)
{
    size_t r_len = 0;
    uint8_t *p = (uint8_t*)data;

    while(len > 0)
    {
        ssize_t r = read(h->fd, p, len);
        if(r == -1) exit(EXIT_FAILURE);

        len   = len   - r;
        p     = p     + r;
        r_len = r_len + r;
    }

    return r_len;
}

#define BUF_MAX 1024
size_t tty_usb_write(tty_usb_handle *h, const void *data, size_t len)
{
    size_t r_len = 0;
    uint8_t* p = (uint8_t*)data;

    while(len > 0)
    {
        size_t sz = (len>BUF_MAX) ? BUF_MAX : len;
        ssize_t r = write(h->fd, p, sz);

        if(r == -1) exit(EXIT_FAILURE);

        len   = len   - r;
        p     = p     + r;
        r_len = r_len + r;
    }
    return r_len;
}

void tty_usb_close(tty_usb_handle *h)
{
    close(h->fd);
    free(h);
}

void tty_usb_flush(tty_usb_handle *h)
{
    tcflush(h->fd, TCIOFLUSH);
}

