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

#include <string.h>
#include <fcntl.h>
#include <linux/hiddev.h>
#include <linux/input.h>
#include <dirent.h>
#include <stdio.h>
#include <unistd.h>

#include "mcp22xx.h"

void mcpsendcommand(int fd, union hidp* cmd, int csize, union hidp* ret, int rsize, int wait){

	struct hiddev_report_info send_inf,get_inf;
	struct hiddev_usage_ref_multi send_com,get_com;
	int i;

	send_com.uref.report_type=HID_REPORT_TYPE_OUTPUT;
	send_com.uref.report_id=HID_REPORT_ID_FIRST;
	send_com.uref.field_index=0;
	send_com.uref.usage_index=0;
	send_com.num_values=csize;

	send_inf.report_type=HID_REPORT_TYPE_OUTPUT;
	send_inf.report_id=HID_REPORT_ID_FIRST;
	send_inf.num_fields=1;


	get_com.uref.report_type=HID_REPORT_TYPE_INPUT;
	get_com.uref.report_id=HID_REPORT_ID_FIRST;
	get_com.uref.field_index=0;
	get_com.uref.usage_index=0;
	get_com.num_values=rsize;

	get_inf.report_type=HID_REPORT_TYPE_INPUT;
	get_inf.report_id=HID_REPORT_ID_FIRST;
	get_inf.num_fields=1;

	/* don't use memcpy here as 'values' is an array of __s32 */
	for(i=0;i<csize;i++) {
		send_com.values[i] = cmd->bytes[i];
	}

	ioctl(fd, HIDIOCSUSAGES, &send_com);
	ioctl(fd, HIDIOCSREPORT, &send_inf);

	if(wait >= 0){
		if(wait > 0)
			usleep(wait);
		ioctl(fd,HIDIOCGUSAGES, &get_com);
		ioctl(fd,HIDIOCGREPORT, &get_inf);

		/* don't use memcpy here as 'values' is an array of __s32 */
		for(i=0;i<rsize;i++) {
			ret->bytes[i]=get_com.values[i];
		}

		ioctl(fd,HIDIOCGUSAGES, &get_com);
		ioctl(fd,HIDIOCGREPORT, &get_inf);

	}
}

