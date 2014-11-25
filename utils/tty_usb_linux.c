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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <time.h>
#include <termios.h>
#include <unistd.h>
#include <dirent.h>
#include "tty_usb.h"

#define PATH_USBDEV "/sys/bus/usb/devices"

struct tty_usb_handle
{
    int fd;
};

static uint16_t read_uint16(const char *path)
{
        int fd = open(path, O_RDONLY);
        char buf[5];
        read(fd, buf, sizeof(buf));
        close(fd);
        return (uint16_t)(strtoul(buf, NULL, 16));
}

static int find_dev_nod_path(const char *d_name, char *dev_path, size_t len)
{
    DIR *dir;
    struct dirent *de;
    char name[NAME_MAX];
    char path[PATH_MAX];
    int r = -1;

    snprintf(name, NAME_MAX, "%s:", d_name);
    dir = opendir(PATH_USBDEV);
    while(((de = readdir(dir)) != NULL) && (r != 0))
    {
        DIR *subdir;
        if(strncmp(de->d_name, name, strlen(name)) != 0) continue;

        snprintf(path, PATH_MAX, PATH_USBDEV "/%s/tty", de->d_name);
        if(access(path, R_OK) != 0) continue;

        subdir = opendir(path);
        while(((de = readdir(subdir)) != NULL) && (r != 0))
        {
            if(strncmp(de->d_name, "tty", strlen("tty")) != 0) continue;

            // here is the result
            r = 0;
            snprintf(dev_path, len, "/dev/%s", de->d_name);
        }
        closedir(subdir);
    }
    closedir(dir);

    return r;
}

static tty_usb_handle *find_usb_device(ifc_match_func callback)
{
    DIR *dir;
    struct dirent *de;
    tty_usb_handle *h = NULL;

    dir = opendir(PATH_USBDEV);
    while(((de = readdir(dir)) != NULL) && (h == NULL))
    {
        char path[PATH_MAX];
        usb_ifc_info usb_info;
        int fd;

        snprintf(path, PATH_MAX, PATH_USBDEV "/%s/idVendor", de->d_name);
        if (access(path, F_OK | R_OK) != 0) continue;
        usb_info.vid = read_uint16(path);

        snprintf(path, PATH_MAX, PATH_USBDEV "/%s/idProduct", de->d_name);
        if (access(path, F_OK | R_OK) != 0) continue;
        usb_info.pid = read_uint16(path);

        if(callback(&usb_info) != 0) continue;
        if(find_dev_nod_path(de->d_name, path, PATH_MAX) != 0) continue;
        if((fd = open(path, O_RDWR)) < 0) continue;

        printf("got %s\n", path);
        h = (tty_usb_handle*)malloc(sizeof(tty_usb_handle));
        h->fd = fd;
    }
    closedir(dir);
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
//    _set_tio(h->fd);
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

