/*
 * Copyright (C) 2021 Pengutronix
 *
 * Michael Grzeschik <mgr@pengutronix.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <errno.h>
#include <stdio.h>
#include <sys/sysmacros.h>
#include <linux/usb/ch9.h>
#include <usbg/usbg.h>
#include <usbg/function/uvc.h>

/**
 * @file gadget-uvc.c
 * @example gadget-uvc.c
 * This is an example of how to create an UVC gadget device.
 */

#define VENDOR		0x1d6b
#define PRODUCT		0x0104

int main(void)
{
	usbg_state *s;
	usbg_gadget *g;
	usbg_config *c;
	usbg_function *f_uvc;
	int ret = -EINVAL;
	int usbg_ret;

	struct usbg_gadget_attrs g_attrs = {
		.bcdUSB = 0x0200,
		.bDeviceClass =	USB_CLASS_PER_INTERFACE,
		.bDeviceSubClass = 0x00,
		.bDeviceProtocol = 0x00,
		.bMaxPacketSize0 = 64, /* Max allowed ep0 packet size */
		.idVendor = VENDOR,
		.idProduct = PRODUCT,
		.bcdDevice = 0x0001, /* Verson of device */
	};

	struct usbg_gadget_strs g_strs = {
		.serial = "0123456789", /* Serial number */
		.manufacturer = "Foo Inc.", /* Manufacturer */
		.product = "Bar Gadget" /* Product string */
	};

	struct usbg_config_strs c_strs = {
		.configuration = "UVC"
	};

	struct usbg_f_uvc_frame_attrs uvc_frame_attrs_array[] = {
		{
			.bFrameIndex = 1,
			.dwFrameInterval = 333333,
			.wHeight = 480,
			.wWidth = 640,
		}, {
			.bFrameIndex = 2,
			.dwFrameInterval = 333333,
			.wHeight = 720,
			.wWidth = 1280,
		}, {
			.bFrameIndex = 3,
			.dwFrameInterval = 333333,
			.wHeight = 1080,
			.wWidth = 1920,
		}, {
			.bFrameIndex = 4,
			.dwFrameInterval = 333333,
			.wHeight = 2160,
			.wWidth = 3840,
		}
	};

	struct usbg_f_uvc_frame_attrs *uvc_frame_mjpeg_attrs[] = {
		&uvc_frame_attrs_array[0],
		&uvc_frame_attrs_array[1],
		&uvc_frame_attrs_array[2],
		&uvc_frame_attrs_array[3],
		NULL,
	};

	struct usbg_f_uvc_frame_attrs *uvc_frame_uncompressed_attrs[] = {
		&uvc_frame_attrs_array[0],
		&uvc_frame_attrs_array[1],
		&uvc_frame_attrs_array[2],
		&uvc_frame_attrs_array[3],
		NULL,
	};

	struct usbg_f_uvc_frame_attrs *uvc_frame_uncompressed_framebased_attrs[] = {
		&uvc_frame_attrs_array[0],
		&uvc_frame_attrs_array[1],
		&uvc_frame_attrs_array[2],
		&uvc_frame_attrs_array[3],
		NULL,
	};

	struct usbg_f_uvc_format_attrs uvc_format_attrs_array[] = {
		{
			.frames = uvc_frame_mjpeg_attrs,
			.format = "mjpeg/m",
			.bDefaultFrameIndex = 3,
		}, {
			.frames = uvc_frame_uncompressed_attrs,
			.format = "uncompressed/u",
			.bDefaultFrameIndex = 3,
		}, {
			.frames = uvc_frame_uncompressed_framebased_attrs,
			.format = "uncompressed/f",
            .bVariableSize = 1,
            .guidFormat = {
                {
                    'H',  '2',  '6',  '4', 0x00, 0x00, 0x10, 0x00,
                    0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71
                },
                16
            },
			.bDefaultFrameIndex = 3,
		}
	};

	struct usbg_f_uvc_format_attrs *uvc_format_attrs[] = {
		&uvc_format_attrs_array[0],
		&uvc_format_attrs_array[1],
		&uvc_format_attrs_array[2],
		NULL,
	};

	struct usbg_f_uvc_attrs uvc_attrs = {
		.formats = uvc_format_attrs,
	};

	usbg_ret = usbg_init("/sys/kernel/config", &s);
	if (usbg_ret != USBG_SUCCESS) {
		fprintf(stderr, "Error on USB gadget init\n");
		fprintf(stderr, "Error: %s : %s\n", usbg_error_name(usbg_ret),
				usbg_strerror(usbg_ret));
		goto out1;
	}

	usbg_ret = usbg_create_gadget(s, "g1", &g_attrs, &g_strs, &g);
	if (usbg_ret != USBG_SUCCESS) {
		fprintf(stderr, "Error on create gadget\n");
		fprintf(stderr, "Error: %s : %s\n", usbg_error_name(usbg_ret),
				usbg_strerror(usbg_ret));
		goto out2;
	}

        usbg_ret = usbg_create_function(g, USBG_F_UVC, "uvc", &uvc_attrs, &f_uvc);
        if(usbg_ret != USBG_SUCCESS)
        {
		fprintf(stderr, "Error creating uvc function\n");
		fprintf(stderr, "Error: %s : %s\n", usbg_error_name(usbg_ret),
				usbg_strerror(usbg_ret));
		goto out2;
	}

	/* NULL can be passed to use kernel defaults */
	usbg_ret = usbg_create_config(g, 1, "cfg", NULL, &c_strs, &c);
	if (usbg_ret != USBG_SUCCESS) {
		fprintf(stderr, "Error creating config\n");
		fprintf(stderr, "Error: %s : %s\n", usbg_error_name(usbg_ret),
				usbg_strerror(usbg_ret));
		goto out2;
	}

        usbg_ret = usbg_add_config_function(c, "uvc.cam", f_uvc);
	if (usbg_ret != USBG_SUCCESS) {
		fprintf(stderr, "Error adding acm.GS0\n");
		fprintf(stderr, "Error: %s : %s\n", usbg_error_name(usbg_ret),
				usbg_strerror(usbg_ret));
		goto out2;
	}

	usbg_ret = usbg_enable_gadget(g, DEFAULT_UDC);
	if (usbg_ret != USBG_SUCCESS) {
		fprintf(stderr, "Error enabling gadget\n");
		fprintf(stderr, "Error: %s : %s\n", usbg_error_name(usbg_ret),
				usbg_strerror(usbg_ret));
		goto out2;
	}

	ret = 0;

out2:
	usbg_cleanup(s);

out1:
	return ret;
}
