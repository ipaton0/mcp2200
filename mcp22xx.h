/*
 * Copyright (C) 2015 Iain Paton <ipaton0@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */


#ifndef _MCP2200_H_
#define _MCP2200_H_

#include <stdint.h>
#include <linux/hiddev.h>


#define CMD_SET_CLEAR_OUT	0x08
#define CMD_CONFIGURE		0x10
#define	CMD_READ_EE		0x20
#define	CMD_WRITE_EE		0x40
#define CMD_READ_ALL		0x80

#define CMD_VND_CONFIG		0x01

#define CFG_PID_VID		0x00
#define CFG_MANU		0x01
#define CFG_PROD		0x02


struct vndstr {
	unsigned char cmd;
	unsigned char cfg;
	unsigned char sequence;
	unsigned char c1;
	unsigned char pad1;
	unsigned char c2;
	unsigned char pad2;
	unsigned char c3;
	unsigned char pad3;
	unsigned char c4;
	unsigned char pad4;
	unsigned char dc11;
	unsigned char dc12;
	unsigned char dc13;
	unsigned char dc14;
	unsigned char dc15;
};
struct vndpid {
	unsigned char cmd;
	unsigned char cfg;
	uint16_t vid;
	uint16_t pid;
	unsigned char dc6;
	unsigned char dc7;
	unsigned char dc8;
	unsigned char dc9;
	unsigned char dc10;
	unsigned char dc11;
	unsigned char dc12;
	unsigned char dc13;
	unsigned char dc14;
	unsigned char dc15;
};

union cap {
	struct __attribute__ ((__packed__)) config_alt_pins {
		unsigned :2;
		unsigned txled:1;
		unsigned rxled:1;
		unsigned :2;
		unsigned usbcfg:1;
		unsigned sspnd:1;
	} bits;
	unsigned char value;
};

union cao {
	struct __attribute__ ((__packed__)) config_alt_options {
		unsigned hwflow:1;
		unsigned invert:1;
		unsigned :3;
		unsigned ledx:1;
		unsigned txtgl:1;
		unsigned rxtgl:1;
	} bits;
	unsigned char value;
};

struct mcp2200 {
	unsigned char cmd;
	unsigned char eep_addr;
	unsigned char eep_wr_val;
	unsigned char eep_rd_val;
	unsigned char io_bmap;
	union cap config_alt_pins;
	unsigned char io_default_val_bmap;
	union cao config_alt_options;
	uint16_t baud;
	unsigned char io_port_val_bmap;
	unsigned char sbmap;
	unsigned char cbmap;
	unsigned char dc13;
	unsigned char dc14;
	unsigned char dc15;
};

union hidp {
	struct vndstr vndstr;
	struct vndpid vndpid;
	struct mcp2200 mcp2200;
	unsigned char bytes[1024];
};

void mcpsendcommand(int fd, union hidp* cmd, int csize, union hidp* ret, int rsize, int wait);


#define GP7	0x80
#define GP6	0x40
#define GP5	0x20
#define GP4	0x10
#define GP3	0x08
#define GP2	0x04
#define GP1	0x02
#define GP0	0x01

/* config_alt_pins */
#define SSPND	0x80
#define USBGFG	0x40
#define RXLED	0x08
#define TXLED	0x04

/* config_alt_options */
#define RXTGL	0x80
#define TXTGL	0x40
#define LEDX	0x20
#define INVERT	0x02
#define HW_FLOW	0x01

#endif
