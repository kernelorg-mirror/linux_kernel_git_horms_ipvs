/*
 * KODOH RTC Driver
 *
 * Copyright (C) 2010 Renesas Electronics Corporation
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

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/rtc.h>
#include <linux/bcd.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/i2c.h>

/* CONTROL1 register bit define */
#define CNT1_WALE	0x80
#define CNT1_DALE	0x40
#define CNT1_1224	0x20
#define CNT1_SCRATCH2	0x10
#define CNT1_CT_SEC	0x04
#define CNT1_CT_MIN	0x05
#define CNT1_CT_HOUR	0x06
#define CNT1_CT_MONTH	0x07
#define CNT1_CT_MASK	0x07

/* CONTROL2 register bit define */
#define CNT2_VDET	0x40
#define CNT2_XST	0x20
#define CNT2_PON	0x10
#define CNT2_SCRATCH1	0x08
#define CNT2_CTFG	0x04
#define CNT2_WAFG	0x02
#define CNT2_DAFG	0x01

static unsigned long rtcalarm_time;

static int kodoh_rtc_read_time(struct device *dev, struct rtc_time *tm)
{
	struct i2c_client *client = to_i2c_client(dev);
	unsigned char val[8];
	int ret;

	ret = i2c_smbus_read_i2c_block_data(client, 0xf << 4, 8, val);
	if (ret < 0)
		return ret;

	tm->tm_sec  = bcd2bin(val[1] & 0x7f);
	tm->tm_min  = bcd2bin(val[2] & 0x7f);
	tm->tm_hour = bcd2bin(val[3] & 0x3f);
	tm->tm_wday = bcd2bin(val[4] & 0x07);
	tm->tm_mday = bcd2bin(val[5] & 0x3f);
	tm->tm_mon  = bcd2bin(val[6] & 0x1f);
	tm->tm_year = bcd2bin(val[7]);

	if (val[6] & 0x80)
		tm->tm_year += 100;
	tm->tm_mon--;

	return 0;
}

static int kodoh_rtc_set_time(struct device *dev, struct rtc_time *tm)
{
	struct i2c_client *client = to_i2c_client(dev);
	unsigned char mon, day, wday;
	unsigned int yrs;
	unsigned char val[7];

	yrs = tm->tm_year + 1900;
	mon = tm->tm_mon + 1;	/* tm_mon starts at zero */
	day = tm->tm_mday;

	/* setting day of week */
	if (mon < 3)
		wday = ((yrs-1) + (yrs-1)/4 - (yrs-1)/100 +
			(yrs-1)/400 + (13*(mon+12)+8)/5 + day) % 7;
	else
		wday = (yrs + yrs/4 - yrs/100 +
			yrs/400 + (13*mon+8)/5 + day) % 7;

	val[0] = bin2bcd(tm->tm_sec);
	val[1] = bin2bcd(tm->tm_min);
	val[2] = bin2bcd(tm->tm_hour);
	val[3] = bin2bcd(wday);;
	val[4] = bin2bcd(day);;
	if (yrs >= 2000) {
		val[5] = bin2bcd(mon) | 0x80;
		val[6] = bin2bcd(yrs - 2000);
	} else {
		val[5] = bin2bcd(mon);
		val[6] = bin2bcd(yrs - 1900);
	}

	return i2c_smbus_write_i2c_block_data(client, 0, 7, val);
}

static int
kodoh_virtual_alarm_set(struct device *dev, struct rtc_wkalrm *alarm)
{
	unsigned long now = get_seconds();

	if (!alarm->enabled) {
		rtcalarm_time = 0;
		return 0;
	} else
		rtc_tm_to_time(&alarm->time, &rtcalarm_time);

	if (now > rtcalarm_time) {
		printk(KERN_ERR "%s: Attempt to set alarm in the past\n",
		       __func__);
		rtcalarm_time = 0;
		return -EINVAL;
	}
	return 0;
}

static struct rtc_class_ops kodoh_rtcops = {
	.read_time  = kodoh_rtc_read_time,
	.set_time   = kodoh_rtc_set_time,
	.set_alarm  = kodoh_virtual_alarm_set,
};

static int
kodoh_rtc_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct rtc_device *rtc;
	int ret;

	rtcalarm_time = 0;

	if (!i2c_check_functionality(client->adapter,
			I2C_FUNC_SMBUS_I2C_BLOCK)) {
		return -ENODEV;
	}

	rtc = rtc_device_register(client->name,
			&client->dev, &kodoh_rtcops, THIS_MODULE);
	if (IS_ERR(rtc))
		return PTR_ERR(rtc);

	/* H/W init */
	ret = i2c_smbus_write_byte_data(client, 0xe << 4, CNT1_1224);
	if (ret)
		goto err;

	ret = i2c_smbus_write_byte_data(client, 0xf << 4, 0);
	if (ret)
		goto err;

	i2c_set_clientdata(client, rtc);

	return 0;

err:
	rtc_device_unregister(rtc);
	return ret;
}

static int kodoh_rtc_remove(struct i2c_client *client)
{
	struct rtc_device *rtc = i2c_get_clientdata(client);

	if (rtc)
		rtc_device_unregister(rtc);

	return 0;
}

static struct i2c_device_id kodoh_rtc_idtable[] = {
	{"kodoh-rtc", 0},
	{ }
};

static struct i2c_driver kodoh_rtcdrv = {
	.driver		= {
		.name	= "kodoh-rtc",
	},
	.probe		= kodoh_rtc_probe,
	.remove		= kodoh_rtc_remove,
	.id_table	= kodoh_rtc_idtable,
};

static __init int kodoh_rtc_init(void)
{
	return i2c_add_driver(&kodoh_rtcdrv);
}
module_init(kodoh_rtc_init);

static __exit void kodoh_rtc_exit(void)
{
	i2c_del_driver(&kodoh_rtcdrv);
}
module_exit(kodoh_rtc_exit);

MODULE_DESCRIPTION("KODOH RTC Driver");
MODULE_LICENSE("GPL");
