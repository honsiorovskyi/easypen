/*
 *  derived from "input/joystick/zhenhua.c"
 *
 *  Copyright (c) 2012 Denis Gonsiorovsky
 *  Copyright (c) 2008 Martin Kebert
 *  Copyright (c) 2001 Arndt Schoenewald
 *  Copyright (c) 2000-2001 Vojtech Pavlik
 *  Copyright (c) 2000 Mark Fletcher
 *
 */

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/serio.h>
#include <linux/init.h>

#define SERIO_EASYPEN 0x3e /* TODO: move this to serio.h in the kernel*/

#define DRIVER_DESC	"Genius EasyPen 3x4 serial tablet driver"

MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("GPL");

/*
 * Constants.
 */

#define EASYPEN_MAX_PACKET_LENGTH	5
#define EASYPEN_PHASING_BIT			128

/*
 * EasyPen per-device data.
 */

struct easypen {
	struct input_dev *dev;
	int idx;
	unsigned char data[EASYPEN_MAX_PACKET_LENGTH];
	char phys[32];
};

/*
 * easypen_interrupt() is called by the low level driver when characters
 * are ready for us. We then buffer them for further processing, or process
 * the whole packet when it is ready.
 */

static irqreturn_t easypen_interrupt(struct serio *serio, unsigned char data, unsigned int flags)
{
	struct easypen *easypen = serio_get_drvdata(serio);
	struct input_dev *dev = easypen->dev;

	/*
	 * Phasing bit is the 7-th bit of the first byte,
	 * we will use it for synchronization. The 7-th bit
	 * of the all other bytes is 0.
	 */

	if (data & EASYPEN_PHASING_BIT)
		easypen->idx = 0;		/* this byte starts a new packet */
	else if (easypen->idx == 0)	/* first byte is not received yet */
		return IRQ_HANDLED;		/* so skip this byte */

	/* store received byte */
	if (easypen->idx < EASYPEN_MAX_PACKET_LENGTH)
		easypen->data[easypen->idx++] = data;

	/* if the packet is complete, process it */
	if (easypen->idx == EASYPEN_MAX_PACKET_LENGTH) {
		/* Buttons are passed via lowest bits of the first byte */
		input_report_key(dev, BTN_LEFT,   (easypen->data[0] & 1));
		input_report_key(dev, BTN_RIGHT,  (easypen->data[0] & 2));

		/* X coordinate is passed via the 2-nd and the 4-th bytes */
		input_report_abs(dev, ABS_X, (easypen->data[2] << 7) |
				(easypen->data[1] & 0x7f));
		/* Y coordinate is passed via the 3-rd and the 5-th bytes */
		input_report_abs(dev, ABS_Y, (easypen->data[4] << 7) |
				(easypen->data[3] & 0x7f));

		input_sync(dev);

		easypen->idx = 0;
	}

	return IRQ_HANDLED;
}

/*
 * easypen_disconnect() is the opposite of easypen_connect()
 */

static void easypen_disconnect(struct serio *serio)
{
	struct easypen *easypen = serio_get_drvdata(serio);

	serio_close(serio);
	serio_set_drvdata(serio, NULL);
	input_unregister_device(easypen->dev);
	kfree(easypen);
}

/*
 * easypen_connect() is the routine that is called when someone adds a
 * new serio device. It looks for the EasyPen, and if found, registers
 * it as an input device.
 */

static int easypen_connect(struct serio *serio, struct serio_driver *drv)
{
	struct easypen *easypen;
	struct input_dev *input_dev;
	int err = -ENOMEM;

	easypen = kzalloc(sizeof(struct easypen), GFP_KERNEL);
	input_dev = input_allocate_device();
	if (!easypen || !input_dev)
		goto fail1;

	easypen->dev = input_dev;
	snprintf(easypen->phys, sizeof(easypen->phys), "%s/input0", serio->phys);

	input_dev->name = "Genius EasyPen 3x4 serial tablet";
	input_dev->phys = easypen->phys;
	input_dev->id.bustype = BUS_RS232;
	input_dev->id.vendor = SERIO_EASYPEN;
	input_dev->id.product = 0x0001;
	input_dev->id.version = 0x0100;
	input_dev->dev.parent = &serio->dev;

	input_dev->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS);
	input_dev->keybit[BIT_WORD(BTN_MOUSE)] = BIT_MASK(BTN_LEFT) |
			BIT_MASK(BTN_RIGHT);

	input_set_abs_params(input_dev, ABS_X, 0, 2000, 0, 0);
	input_set_abs_params(input_dev, ABS_Y, 0, 1500, 0, 0);

	serio_set_drvdata(serio, easypen);

	err = serio_open(serio, drv);
	if (err)
		goto fail2;

	err = input_register_device(easypen->dev);
	if (err)
		goto fail3;

	return 0;

 fail3:	serio_close(serio);
 fail2:	serio_set_drvdata(serio, NULL);
 fail1:	input_free_device(input_dev);
	kfree(easypen);
	return err;
}

/*
 * The serio driver structure.
 */

static struct serio_device_id easypen_serio_ids[] = {
	{
		.type	= SERIO_RS232,
		.proto	= SERIO_EASYPEN,
		.id	= SERIO_ANY,
		.extra	= SERIO_ANY,
	},
	{ 0 }
};

MODULE_DEVICE_TABLE(serio, easypen_serio_ids);

static struct serio_driver easypen_drv = {
	.driver		= {
		.name	= "easypen",
	},
	.description	= DRIVER_DESC,
	.id_table	= easypen_serio_ids,
	.interrupt	= easypen_interrupt,
	.connect	= easypen_connect,
	.disconnect	= easypen_disconnect,
};

module_serio_driver(easypen_drv);
