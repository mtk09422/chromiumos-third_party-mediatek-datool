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
#include <string.h>
#include "tty_usb.h"
#include "cmds.h"
#include "file_util.h"

typedef struct {
    char            m_identifier[12];   // including '\0'
    uint32_t        m_ver;
    uint32_t        m_dev_rw_unit;      // NAND: in page
                                        // NOR/eMMC/SFlash: in byte
} EMMC_HEADER_v1;

typedef struct {
    uint32_t        m_bl_exist_magic;
    uint8_t         m_bl_dev;               // 1B
    uint16_t        m_bl_type;              // 2B
    uint32_t        m_bl_begin_dev_addr;    // device addr that points to the beginning of BL
                                            // NAND: page addr
                                            //      SEQUENTIAL: phy page addr
                                            //      TTBL: TTBL logic page addr
                                            //      FDM5: FDM5.0 logic page addr
                                            // NOR/eMMC/SFlash: byte addr

    uint32_t        m_bl_boundary_dev_addr; // device addr that points to the boundary of BL
                                            // boundary addr = BL addr + BL length
                                            // NAND: page addr
                                            //      SEQUENTIAL: phy page addr
                                            //      TTBL: TTBL logic page addr
                                            //      FDM5: FDM5.0 logic page addr
                                            // NOR/eMMC/SFlash: byte addr

    uint32_t        m_bl_attribute;         // refer to GFH_BL_INFO
} BL_Descriptor;

typedef struct {
    char            m_identifier[8];        // including '\0'
    uint32_t        m_ver;                  // this structure will directly export to others, version field is necessary
    uint32_t        m_boot_region_dev_addr; // device addr that points to the beginning of the boot region
                                            // NAND: page addr
                                            //      SEQUENTIAL: phy page addr
                                            //      TTBL: TTBL logic page addr
                                            //      FDM5: FDM5.0 logic page addr
                                            // NOR/eMMC/SFlash: byte addr

    uint32_t        m_main_region_dev_addr; // device addr that points to the beginning of the main code region
                                            // NAND: page addr
                                            //      SEQUENTIAL: phy page addr
                                            //      FDM5: FDM5.0 logic page addr
                                            // NOR/eMMC/SFlash: byte addr
    BL_Descriptor   m_bl_desc;
} BR_Layout_v1;

typedef struct GFH_FILE_INFO_v1 {
    uint32_t        m_magic_ver;
    uint16_t        m_size;
    uint16_t        m_type;
    char            m_identifier[12];       // including '\0'
    uint32_t        m_file_ver;
    uint16_t        m_file_type;
    uint8_t         m_flash_dev;
    uint8_t         m_sig_type;
    uint32_t        m_load_addr;
    uint32_t        m_file_len;
    uint32_t        m_max_size;
    uint32_t        m_content_offset;
    uint32_t        m_sig_len;
    uint32_t        m_jump_offset;
    uint32_t        m_attr;
} GFH_FILE_INFO_v1;

void strip_pl_hdr(void *pl, size_t len_pl, void **strip_pl, size_t *len_strip_pl)
{
    EMMC_HEADER_v1 *ehdr = (EMMC_HEADER_v1 *)pl;
    GFH_FILE_INFO_v1 *gfh = (GFH_FILE_INFO_v1 *)pl;
    *strip_pl = pl;
    *len_strip_pl = len_pl;

    // emmc header
    if((strcmp("EMMC_BOOT", ehdr->m_identifier) == 0) &&
       (ehdr->m_ver == 1))
    {
        BR_Layout_v1 *brlyt;

        if(ehdr->m_dev_rw_unit + sizeof(BR_Layout_v1) > len_pl)
        {
            printf("ERROR: EMMC HDR error. dev_rw_unit=%x, brlyt_size=%zx, len_pl=%zx\n",
                        ehdr->m_dev_rw_unit,
                        sizeof(BR_Layout_v1),
                        len_pl);
            exit(EXIT_FAILURE);
        }

        brlyt = (BR_Layout_v1 *)((char *)pl + ehdr->m_dev_rw_unit);
        if((strcmp("BRLYT", brlyt->m_identifier) != 0) ||
           (brlyt->m_ver != 1))
        {
            printf("ERROR: BRLYT error. m_ver=%x, m_identifier=%s\n",
                        brlyt->m_ver,
                        brlyt->m_identifier);
            exit(EXIT_FAILURE);
        }

        gfh = (GFH_FILE_INFO_v1 *)((char *)pl + brlyt->m_bl_desc.m_bl_begin_dev_addr);
    }

    // gfh
    if(((gfh->m_magic_ver & 0x00FFFFFF) == 0x004D4D4D) &&
       (gfh->m_type == 0) &&
       (strcmp("FILE_INFO", gfh->m_identifier) == 0))
    {

        if(gfh->m_file_len < (gfh->m_jump_offset + gfh->m_sig_len))
        {
            // gfh error
            printf("ERROR: GFH error. len_pl=%zx, file_len=%x, jump_offset=%x, sig_len=%x\n",
                        len_pl,
                        gfh->m_file_len,
                        gfh->m_jump_offset,
                        gfh->m_sig_len);
            exit(EXIT_FAILURE);
        }

        *strip_pl = ((char*)gfh + gfh->m_jump_offset);
        *len_strip_pl = gfh->m_file_len - gfh->m_jump_offset - gfh->m_sig_len;
    }
}

enum CHIP_ID {
    CHIP_ID_DEFAULT,
    CHIP_ID_MT8135,
    CHIP_ID_MT6595,
    CHIP_ID_MT8173,
};

typedef struct {
    enum CHIP_ID chip;
    uint32_t pl_load_addr;
    uint32_t lk_wrapper_addr;
} CHIP_DATA;

CHIP_DATA chip_tbl[] = {
    {CHIP_ID_DEFAULT, 0x00201000, 0x40001000},
    {CHIP_ID_MT8135,  0x12001000, 0x80001000},
    {CHIP_ID_MT6595,  0x00201000, 0x40001000},
    {CHIP_ID_MT8173,  0x000C1000, 0x40001000},
};

CHIP_DATA* get_chip_data(uint16_t chip)
{
    if(chip == 0x8135) return &(chip_tbl[1]);
    if(chip == 0x6595) return &(chip_tbl[2]);
    if(chip == 0x8172) return &(chip_tbl[3]);
    return &(chip_tbl[0]);
}

int main(int argc, char *argv[])
{
    tty_usb_handle *h = NULL;
    int r, is_brom = 0;
    void *lk=NULL, *sig_lk=NULL;
    size_t len_lk, len_sig_lk;
    char *auth_path=NULL, *pl_path=NULL, *lk_path=NULL;
    uint16_t chip_code;
    CHIP_DATA* chip;
    char lk_sig_path[4096];

    if((argc!=3) && (argc!=5))
    {
        printf("Version: 1.0\n");
        printf("Usage: %s [-a auth] preloader lk\n", argv[0]);
        return 1;
    }
    if(argc == 3)
    {
        pl_path = argv[1];
        lk_path = argv[2];
    }
    else // argc == 5
    {
        if(strcmp(argv[1], "-a") != 0)
        {
            printf("Usage: %s [-a auth] preloader lk\n", argv[0]);
            return 1;
        }
        auth_path = argv[2];
        pl_path = argv[3];
        lk_path = argv[4];
    }

    h = tty_usb_open_auto();
    is_brom = tty_usb_is_target_brom();

    r = start_cmd(h);
    if(r)
    {
        printf("ERROR: start_cmd(%d)\n", r);
        return 1;
    }
    if(is_brom) printf("connect brom\n");
    else printf("connect preloader\n");

    r = get_hw_code(h, &chip_code);
    printf("chip:%x\n", chip_code);
    chip = get_chip_data(chip_code);

    if(is_brom)
    {
        uint32_t cfg;
        void *pl=NULL, *sig_pl=NULL, *strip_pl=NULL;
        size_t len_pl, len_sig_pl, len_strip_pl;

        r = get_target_config(h, &cfg);
        if(r)
        {
            printf("ERROR: get_target_config(%d)\n", r);
            return 1;
        }
        if(TGT_CFG_DAA_EN & cfg)
        {
            void *auth;
            size_t len_auth;

            if((TGT_CFG_SBC_EN & cfg) == 0)
            {
                printf("ERROR: daa=1, sbc=0\n");
                return 1;
            }

            if(auth_path==NULL)
            {
                printf("ERROR: no auth file\n");
                return 1;
            }

            load_binary(auth_path, &auth, &len_auth);
            r = send_auth(h, auth, len_auth);
            free(auth);
            if(r)
            {
                printf("ERROR: send_auth(%d)\n", r);
                return 1;
            }
            printf("send %s\n", auth_path);
        }

        load_binary(pl_path, &pl, &len_pl);
        strip_pl_hdr(pl, len_pl, &strip_pl, &len_strip_pl);
        if(TGT_CFG_DAA_EN & cfg)
        {
            char sig_path[4096];
            strcpy(sig_path, pl_path);
            strcat(sig_path, ".sign");
            load_binary(sig_path, &sig_pl, &len_sig_pl);
        }
        else
            len_sig_pl = 0;
        r = send_da(h, chip->pl_load_addr, strip_pl, len_strip_pl, sig_pl, len_sig_pl);
        if(pl) free(pl);
        if(sig_pl) free(sig_pl);
        if(r)
        {
            printf("ERROR: send_da(%d)\n", r);
            return 1;
        }
        printf("send %s\n", pl_path);

        r = jump_da(h, chip->pl_load_addr);
        if(r)
        {
            printf("ERROR: jump_da(%d)\n", r);
            return 1;
        }

        tty_usb_close(h);
        h = NULL;

        // open preloader tty
        h = tty_usb_open_pl();

        r = start_cmd(h);
        if(r)
        {
            printf("ERROR: start_cmd(%d)\n", r);
            return 1;
        }
        printf("connect preloader\n");
    }

    // gen fastboot da from lk
    load_binary(lk_path, &lk, &len_lk);
    strcpy(lk_sig_path, lk_path);
    strcat(lk_sig_path, ".sign");
    load_binary(lk_sig_path, &sig_lk, &len_sig_lk);
    r = send_da(h, chip->lk_wrapper_addr, lk, len_lk, sig_lk, len_sig_lk);
    if(lk) free(lk);
    if(sig_lk) free(sig_lk);
    if(r)
    {
        printf("ERROR: send_da(%d)\n", r);
        return 1;
    }
    printf("send %s\n", lk_path);

    r = jump_da(h, chip->lk_wrapper_addr);
    if(r)
    {
        printf("ERROR: jump_da(%d)\n", r);
        return 1;
    }

    tty_usb_close(h);

    return 0;
}

