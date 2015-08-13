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

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <getopt.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <linux/hiddev.h>
#include <linux/input.h>
#include <endian.h>
#include <ctype.h>

#include "mcp22xx.h"

void display(unsigned char *buf,int lines) {
	int i,j;

	for (i = 0; i < (lines*16); i += 16) {
		printf("%04X: ", i);
		for (j = 0; j < 16; j++)
			printf("%02X ", (unsigned char)buf[i+j]);
		printf("\t");
		for (j = 0; j < 16; j++) {
			if (isprint(buf[i+j])) {
				printf("%c", buf[i+j]);
			} else {
				printf(".");
			}
		}
		printf("\n");
	}
}

void dumpcommand(char *msg, unsigned char *cmd, int len) {

	int i;
	printf(msg);
	for(i=0;i<len;i++) {
		printf("%02X ",cmd[i]);
	}
	printf("\n");
}

void mcpidstring(int fd, char *str, char type) {

	union hidp command;
	int c,l;

	memset(&command,0,sizeof(command));

	c = 0;
	l = strlen(str);

	command.vndstr.cmd = CMD_VND_CONFIG;
	command.vndstr.cfg = type;
	command.vndstr.dc11 = 0xFF;
	command.vndstr.dc12 = 0xFF;
	command.vndstr.dc13 = 0xFF;
	command.vndstr.dc14 = 0xFF;
	command.vndstr.dc15 = 0xFF;

	while(c < 63){
		memset(&command.vndstr.c1,0,8);
		command.vndstr.sequence = (c+1)>>2;
		if(c == 0){
			command.vndstr.c1 = l*2+2;
			command.vndstr.pad1 = 0x03;
		} else {
			if( c <= l ){
				command.vndstr.c1 = str[c];
			}
			c++;
		}
		if( c <= l ){
			command.vndstr.c2 = str[c];
		}
		c++;
		if( c <= l ){
			command.vndstr.c3 = str[c];
		}
		c++;
		if( c <= l ){
			command.vndstr.c4 = str[c];
		}
		c++;
		dumpcommand("mcpidstring: ",command.bytes,16);
		mcpsendcommand(fd, &command, 16, NULL, 0, -1);
	}
}

int main(int argc, char **argv){

	char path[255]="/dev/usb/hiddev0";
	int vid=0, pid=0, cvid=0, cpid=0;
	unsigned char alt=0, altopts=0;
	int bmap=-1, def=-1, calt=-1, caltopts=-1, baud=-1, dconf=0, deeprom=0;
	int set=-1 ,clr=-1 ,sspnd=-1 ,usbcfg=-1 ,rxled=-1 ,txled=-1 ,rxtgl=-1;
	int txtgl=-1 ,ledx=-1 ,invert=-1 ,hwflow=-1, list=0, info=0, gpio=-1;
	int wc=0, sc=0, vc=0, rg=0, re=-1, we=0;
	int debug=0;
	char prod[64];
	char mfg[64];

	char *subopts, *value;
	int errfnd=0;

	enum {
		ADDR = 0,
		VAL,
	};
	char *const token[] = {
		[ADDR]   = "a",
		[VAL]   = "v",
		NULL
	};

	int addr=0,val=0x55;

	int c;
	int option_index;
	int fd;

	struct hiddev_devinfo dev;

	struct option long_options[] =
		{
			{"help",	no_argument,		0, 1},
			{"h",		no_argument,		0, 1},

			{"debug",	no_argument,		0, 2},

			{"list",	no_argument,		0, 3},
			{"l",		no_argument,		0, 3},
			{"info",	no_argument,		0, 4},
			{"i",		no_argument,		0, 4},

			{"devicefile",	required_argument,	0, 5},
			{"f",		required_argument,	0, 5},
			{"vid",		required_argument,	0, 6},
			{"pid",		required_argument,	0, 7},
			{"manuf",	required_argument,	0, 8},
			{"prod",	required_argument,	0, 9},

			{"bmap",	required_argument,	0, 10},
			{"b",		required_argument,	0, 10},
			{"defbmap",	required_argument,	0, 11},
			{"d",		required_argument,	0, 11},
			{"alt",		required_argument,	0, 12},
			{"a",		required_argument,	0, 12},
			{"altopts",	required_argument,	0, 13},
			{"o",		required_argument,	0, 13},
			{"baud",	required_argument,	0, 14},
			{"s",		required_argument,	0, 14},

			{"dumpconfig",	no_argument,		0, 15},
			{"c",		no_argument,		0, 15},
			{"dumpeeprom",	no_argument,		0, 16},
			{"e",		no_argument,		0, 16},

			{"setgpio",	required_argument,	0, 17},
			{"cleargpio",	required_argument,	0, 18},

			{"sspnd",	required_argument,	0, 19},
			{"usbcfg",	required_argument,	0, 20},
			{"rxled",	required_argument,	0, 21},
			{"txled",	required_argument,	0, 22},

			{"rxtgl",	required_argument,	0, 23},
			{"txtgl",	required_argument,	0, 24},
			{"ledx",	required_argument,	0, 25},
			{"invert",	required_argument,	0, 26},
			{"hwflow",	required_argument,	0, 27},

			{"gpio",	required_argument,	0, 28},
			{"g",		required_argument,	0, 28},

			{"readgpio",	no_argument,		0, 29},
			{"rg",		no_argument,		0, 29},

			{"readeeprom",	required_argument,	0, 30},
			{"re",		required_argument,	0, 30},
			{"writeeeprom",	required_argument,	0, 31},
			{"we",		required_argument,	0, 31},



			{0, 0, 0, 0}
		};

	memset(prod,0,sizeof(prod));
	memset(mfg,0,sizeof(mfg));

	while ((c = getopt_long_only(argc, argv, "",long_options,&option_index)) != -1){
		switch (c) {
			case 1:
				printf("Usage: mcpconfig <arguments>\n"
				"arguments:\n"
				"-h,  --help                     Prints this help\n"
				"-l,  --list                     List available hiddev devices\n"
				"-f,  --devicefile               Selects the HID device to use (/dev/usb/hiddev0)\n"
				"-i,  --info                     show info on selected device\n"
				"-e,  --dumpeeprom               dump eeprom contents\n"
				"-c,  --dumpconfig               dump device configuration\n"
				"     --vid <n>                  Sets the vendor ID of the device\n"
				"     --pid <n>                  Sets the product ID of the device\n"
				"     --manuf <string>           Sets manufacturer string (max 64 chars)\n"
				"     --prod  <string>           Sets product string (max 64 chars)\n"
				"-b,  --bmap  <n>                Sets GPIO direction\n"
				"-d,  --defbmap <n>              Sets default GPIO direction\n"
				"-a,  --alt <n>                  Sets alternativ configuration pin settings\n"
				"-o,  --altopts <n>              Sets alternative function options\n"
				"-s,  --baud <n>                 Sets default baudrate\n"
				"     --setgpio <n>              Sets the corresponding gpio bits\n"
				"     --cleargpio <n>            Clear the corresponding gpio bits\n"
				"-g,  --gpio <n>                 Set all gpio bits\n"
				"-rg, --readgpio                 Print current gpio state in hex\n"
				"-re, --readeeprom <n>           Print contents of eeprom location in hex\n"
				"-we, --writeeeprom a=addr,v=val Write eeprom location\n"
				"     --sspnd <0|1>              SSPND bit\n"
				"     --usbcfg <0|1>             USBCFG bit\n"
				"     --rxled <0|1>              RxLED bit\n"
				"     --txled <0|1>              TxLED bit\n"
				"     --rxtgl <0|1>              RxTGL bit\n"
				"     --txtgl <0|1>              TxTGL bit\n"
				"     --ledx <0|1>               LEDX bit\n"
				"     --invert <0|1>             INVERT bit\n"
				"     --hwflow <0|1>             HW_FLOW bit\n"
				"\n");
				exit(1);
				break;
			case 2:
				debug=1;
				break;
			case 3:		// list
				list=1;
				break;
			case 4:		// info
				info=1;
				break;
			case 5:		// devicefile
				strncpy(path,optarg,255);
				break;
			case 6:		// set vendor id
				vid = strtol(optarg, NULL, 0);
				cvid=1;
				break;
			case 7:		// set product id
				pid = strtol(optarg, NULL, 0);
				cpid=1;
				break;
			case 8:		// set manufacturer string
				strncpy(mfg, optarg, 64);
				break;
			case 9:		// set product string
				strncpy(prod, optarg, 64);
				break;
			case 10:		// set IO_bmap
				bmap = strtol(optarg, NULL, 0);
				break;
			case 11:	// set IO_Default_Val_bmap
				def = strtol(optarg, NULL, 0);
				break;
			case 12:	// set Config_Alt_Pins
				alt = strtol(optarg, NULL, 0);
				calt=1;
				break;
			case 13:	// set Config_Alt_Options
				altopts = strtol(optarg, NULL, 0);
				caltopts=1;
				break;
			case 14:	// set default baud
				baud = strtol(optarg, NULL, 0);
				baud = 12000000UL/baud - 1;
				break;
			case 15:	// dump config
				dconf=1;
				break;
			case 16:	// dump eeprom
				deeprom=1;
				break;
			case 17:	// set gpio
				set = strtol(optarg, NULL, 0);
				break;
			case 18:	// clear gpio
				clr = strtol(optarg, NULL, 0);
				break;
			case 19:	// sspnd
				sspnd = strtol(optarg, NULL, 0) ? 1 : 0;
				break;
			case 20:	// usbcfg
				usbcfg = strtol(optarg, NULL, 0) ? 1 : 0;
				break;
			case 21:	// RxLED
				rxled = strtol(optarg, NULL, 0) ? 1 : 0;
				break;
			case 22:	// TxLED
				txled = strtol(optarg, NULL, 0) ? 1 : 0;
				break;
			case 23:	// RxTGL
				rxtgl = strtol(optarg, NULL, 0) ? 1 : 0;
				break;
			case 24:	// TxTGL
				txtgl = strtol(optarg, NULL, 0) ? 1 : 0;
				break;
			case 25:	// LEDX
				ledx = strtol(optarg, NULL, 0) ? 1 : 0;
				break;
			case 26:	// INVERT
				invert = strtol(optarg, NULL, 0) ? 1 : 0;
				break;
			case 27:	// HW_FLOW
				hwflow = strtol(optarg, NULL, 0) ? 1 : 0;
				break;
			case 28:	//gpio
				gpio = strtol(optarg, NULL, 0);
				break;
			case 29:
				rg=1;
				break;
			case 30:
				re = strtol(optarg, NULL, 0);
				break;
			case 31:
				we=1;
				subopts = optarg;
				while (*subopts != '\0' && !errfnd) {
					switch (getsubopt(&subopts, token, &value)) {
						case ADDR:
							if (value == NULL) {
								fprintf(stderr, "Missing value for suboption '%s'\n", token[ADDR]);
								errfnd = 1;
								continue;
							}
							addr = strtol(value, NULL, 0);
							break;
						case VAL:
							if (value == NULL) {
								fprintf(stderr, "Missing value for suboption '%s'\n", token[VAL]);
								errfnd = 1;
								continue;
							}
							val = strtol(value, NULL, 0);
							break;
						default:
							errfnd = 1;
					}
				}
				if(errfnd) {
					fprintf(stderr,"malformed options for eeprom write\n");
					exit(1);
				}
				break;
		}
	}


	struct hiddev_string_descriptor mstr,pstr;
	mstr.index=1;
	mstr.value[0]=0;
	pstr.index=2;
	pstr.value[0]=0;

	if(list) {
		DIR *d;
		int fd;
		struct dirent *dir;
		d = opendir("/dev/usb/");
		if(d) {
			char temppath[255];
			while ((dir = readdir(d)) != NULL) {
				if(strncmp(dir->d_name,"hiddev", 6) == 0) {
					strcpy(temppath, "/dev/usb/");
					strcat(temppath, dir->d_name);
					if((fd = open(temppath, O_RDONLY)) >= 0) {
						ioctl(fd, HIDIOCGSTRING, &mstr);
						ioctl(fd, HIDIOCGSTRING, &pstr);
						printf("Path: %s\nManufacturer: %s\nProduct: %s\n",temppath,mstr.value,pstr.value);
						if(info) {
							ioctl(fd, HIDIOCGDEVINFO, &dev);
							printf("VID: 0x%04hX  PID: 0x%04hX  version: 0x%04hX\n",dev.vendor, dev.product, dev.version);
							printf("Applications: %i\n", dev.num_applications);
							printf("Bus: %d Devnum: %d Ifnum: %d\n",dev.busnum, dev.devnum, dev.ifnum);
						}
						close(fd);
					}
				}
			}
			closedir(d);
		}
		exit(0);
	}


	if((fd = open(path, O_RDONLY)) >= 0) {
		ioctl(fd, HIDIOCGSTRING, &mstr);
		ioctl(fd, HIDIOCGSTRING, &pstr);
		ioctl(fd, HIDIOCGDEVINFO, &dev);
	} else {
		printf("Could not open %s\n",path);
		exit(1);
	}

	if(info) {
		printf("Path: %s\nManufacturer: %s\nProduct: %s\n",path,mstr.value,pstr.value);
		printf("VID: 0x%04hX  PID: 0x%04hX  version: 0x%04hX\n",dev.vendor, dev.product, dev.version);
		printf("Applications: %i\n", dev.num_applications);
		printf("Bus: %d Devnum: %d Ifnum: %d\n",dev.busnum, dev.devnum, dev.ifnum);
	}

	union hidp command,defv,rsp;

	memset(&command, 0, sizeof(command));
	memset(&defv, 0, sizeof(defv));

	command.mcp2200.cmd = CMD_READ_ALL;
	if(debug) {
		dumpcommand("CMD_READ_ALL: ",command.bytes,16);
	}
	defv.bytes[1]=0;
	defv.bytes[2]=0;
	defv.bytes[3]=0;
	defv.bytes[10]=0;
	defv.bytes[11]=0;
	defv.bytes[12]=0;
	defv.bytes[13]=0;
	defv.bytes[14]=0;
	defv.bytes[15]=0;

	/*
	 * not doing this twice tends to mean we get a blank report after
	 * writing something to the device. Since we have to write ALL
	 * config values in one go, we're reliant on a read-modify-write
	 * cycle to avoid wiping everything we don't want to touch.
	 * So blank reports at this point are bad.
	 */
	mcpsendcommand(fd, &command, 16, &defv, 16, 1000);
	mcpsendcommand(fd, &command, 16, &defv, 16, 1000);


	if(debug) {
		dumpcommand("response    : ",defv.bytes,16);
	}

	if(rg) {
		printf("%02X\n",defv.mcp2200.io_port_val_bmap);
		close(fd);
		exit(0);
	}

	if(re != -1) {

		command.mcp2200.cmd=CMD_READ_EE;
		command.mcp2200.eep_addr=re;
		if(debug) {
			dumpcommand("CMD_READ_EE: ",command.bytes,16);
		}
		mcpsendcommand(fd, &command, 16, &rsp, 16, 1000);
		if(debug) {
			dumpcommand("response    : ",rsp.bytes,16);
		}

		printf("%02X\n",rsp.mcp2200.eep_rd_val);
		close(fd);
		exit(0);
	}

	if(we) {
		command.mcp2200.cmd=CMD_WRITE_EE;
		command.mcp2200.eep_addr=addr;
		command.mcp2200.eep_wr_val=val;
		if(debug) {
			dumpcommand("CMD_WRITE_EE: ",command.bytes,16);
		}
		mcpsendcommand(fd, &command, 16, NULL, 0, -1);
	}


	if(sspnd != -1) {
		defv.mcp2200.config_alt_pins.bits.sspnd=sspnd;
		wc++;
	}
	if(usbcfg != -1) {
		defv.mcp2200.config_alt_pins.bits.usbcfg=usbcfg;
		wc++;
	}
	if(rxled != -1) {
		defv.mcp2200.config_alt_pins.bits.rxled=rxled;
		wc++;
	}
	if(txled != -1) {
		defv.mcp2200.config_alt_pins.bits.txled=txled;
		wc++;
	}
	if(rxtgl != -1) {
		defv.mcp2200.config_alt_options.bits.rxtgl=rxtgl;
		wc++;
	}
	if(txtgl != -1) {
		defv.mcp2200.config_alt_options.bits.txtgl=txtgl;
		wc++;
	}
	if(ledx != -1) {
		defv.mcp2200.config_alt_options.bits.ledx=ledx;
		wc++;
	}
	if(invert != -1) {
		defv.mcp2200.config_alt_options.bits.invert=invert;
		wc++;
	}
	if(hwflow != -1) {
		defv.mcp2200.config_alt_options.bits.hwflow=hwflow;
		wc++;
	}
	if(bmap != -1) {
		defv.mcp2200.io_bmap=bmap;
		wc++;
	}
	if(def != -1) {
		defv.mcp2200.io_default_val_bmap=def;
		wc++;
	}
	if(calt != -1) {
		defv.mcp2200.config_alt_pins.value=alt;
		wc++;
	}
	if(caltopts != -1) {
		defv.mcp2200.config_alt_options.value=altopts;
		wc++;
	}

	if(baud != -1){
		defv.mcp2200.baud = htobe16(baud);
		wc++;
	}

	if(wc) {
		defv.mcp2200.cmd=CMD_CONFIGURE;
		if(debug) {
			dumpcommand("CMD_CONFIGURE : ",defv.bytes,16);
		}
		mcpsendcommand(fd, &defv, 16, &rsp, 16, 1000);
		if(debug) {
			dumpcommand("response    : ",defv.bytes,16);
		}

	}

	if(dconf) {
		printf("04 IO_Bmap             : %02X\n",defv.mcp2200.io_bmap);
		printf("05 Config_Alt_Pins     : %02X\n",defv.mcp2200.config_alt_pins.value);
		printf("        SSPND          : %d\n",defv.mcp2200.config_alt_pins.bits.sspnd);
		printf("        USBCFG         : %d\n",defv.mcp2200.config_alt_pins.bits.usbcfg);
		printf("        RxLED          : %d\n",defv.mcp2200.config_alt_pins.bits.rxled);
		printf("        TxLED          : %d\n",defv.mcp2200.config_alt_pins.bits.txled);
		printf("06 IO_Default_Val_bmap : %02X\n",defv.mcp2200.io_default_val_bmap);
		printf("07 Config_Alt_Options  : %02X\n",defv.mcp2200.config_alt_options.value);
		printf("        RxTGL          : %d\n",defv.mcp2200.config_alt_options.bits.rxtgl);
		printf("        TxTGL          : %d\n",defv.mcp2200.config_alt_options.bits.txtgl);
		printf("        LEDX           : %d\n",defv.mcp2200.config_alt_options.bits.ledx);
		printf("        INVERT         : %d\n",defv.mcp2200.config_alt_options.bits.invert);
		printf("        HW_FLOW        : %d\n",defv.mcp2200.config_alt_options.bits.hwflow);
		printf("08 Baud                : %04X\n",be16toh(defv.mcp2200.baud));
		printf("08 Baud                : %lu\n",12000000UL/(be16toh(defv.mcp2200.baud)+1));
		printf("0A IO_Port_Val_bmap    : %02X\n",defv.mcp2200.io_port_val_bmap);
	}
	if(deeprom) {
		unsigned char eep[256];

		int f;
		for(f=0;f<256;f++) {
			command.mcp2200.cmd=CMD_READ_EE;
			command.mcp2200.eep_addr=f;
			mcpsendcommand(fd, &command, 16, &rsp, 16, 1000);
			eep[f]=rsp.mcp2200.eep_rd_val;
		}
		display(eep,16);

	}
	if(set != -1) {
		command.mcp2200.sbmap=set;
		sc++;
	}
	if(clr != -1) {
		command.mcp2200.cbmap=clr;
		sc++;
	}
	if(gpio != -1) {
		sc++;
		command.mcp2200.sbmap=gpio;
		command.mcp2200.cbmap=~command.mcp2200.sbmap;
	}
	if(sc) {
		command.mcp2200.cmd=CMD_SET_CLEAR_OUT;
		if(debug) {
			dumpcommand("CMD_SET_CLEAR_OUT : ",command.bytes,16);
		}
		mcpsendcommand(fd, &command, 16, NULL, 0, -1);
	}

	if(mfg[0] != 0) {
		mcpidstring(fd,mfg,CFG_MANU);
	}
	if(prod[0] != 0) {
		mcpidstring(fd,prod,CFG_PROD);
	}
	if(cvid) {

		command.vndpid.vid=vid;

		vc++;
	}
	if(cpid) {

		command.vndpid.pid=pid;

		vc++;
	}

	if(vc) {
		command.vndpid.cmd=CMD_VND_CONFIG;
		command.vndpid.cfg=CFG_PID_VID;

		if(! cpid) {
			command.vndpid.pid=dev.product;
		}
		if(! cvid) {
			command.vndpid.vid=dev.vendor;
		}
		if(debug) {
			dumpcommand("CFG_PID_VID : ",command.bytes,16);
		}
		mcpsendcommand(fd, &command, 16, NULL, 0, -1);
	}

	close(fd);
	return 0;
}
