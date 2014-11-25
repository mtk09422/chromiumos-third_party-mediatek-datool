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

#include <stdio.h>
#include <stdlib.h>

void load_binary(char *path, void **data, size_t *len)
{
    FILE *fp;
    printf("Loading file: %s\n", path);
    fp = fopen(path, "rb");
    if (fp == NULL) {
        printf("Error open file %s\n", path);
        exit(0);
    }
    fseek(fp, 0, SEEK_END);
    *len = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    *data = malloc(*len);
    fread(*data, 1, *len, fp);
    fclose(fp);
}

