/*
 * Dummy Touch driver
 *
 * Copyright (C) 2011 Renesas Electronics Corporation
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301, USA.
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/input.h>

static struct input_dev *ts_dev;

static int __init dummy_ts_init(void)
{
	struct input_dev *dev = ts_dev;

	dev = input_allocate_device();

	dev->name = "dummy";
	dev->dev.parent = NULL;
	dev->id.bustype = BUS_HOST;
	dev->id.vendor = 0x10b7;
	dev->id.product = 0x0001;
	dev->id.version = 0x0001;

	dev->evbit[0] = BIT(EV_KEY) | BIT(EV_ABS);
	dev->keybit[BIT_WORD(BTN_TOUCH)] |= BIT_MASK(BTN_TOUCH);
	dev->absbit[0] = BIT(ABS_X) | BIT(ABS_Y) | BIT(ABS_PRESSURE);

	input_set_abs_params(dev, ABS_X,    0,     100, 0, 0);
	input_set_abs_params(dev, ABS_Y,    0,     100, 0, 0);
	input_set_abs_params(dev, ABS_PRESSURE, 0, 100, 0, 0);

	return input_register_device(dev);
}

static void __exit dummy_ts_exit(void)
{
	input_unregister_device(ts_dev);
	input_free_device(ts_dev);
}

module_init(dummy_ts_init);
module_exit(dummy_ts_exit);

MODULE_AUTHOR("Renesas");
MODULE_DESCRIPTION("Dummy touch driver");
MODULE_LICENSE("GPL");
