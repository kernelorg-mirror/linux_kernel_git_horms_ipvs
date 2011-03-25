/*
 * Function        : V4L2 driver for APE5R
 *
 * Copyright (C) 2011 Renesas Electronics Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/version.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/vmalloc.h>
#include <linux/device.h>
#include <linux/workqueue.h>
#include <linux/kthread.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/hardirq.h>

#include <media/v4l2-common.h>

#include <mach/hardware.h>
#include <media/v4l2-ioctl.h>
#include <media/videobuf-core.h>

#define _V4L2_RT_THREAD 1 /* 1:enable  0:disable */
#if _V4L2_RT_THREAD
#include <linux/sched.h>
#endif

#include "ape5r_overlay.h"
#include "ape5r_v4l2_disp.h"

/* place holder to handle debug interface */
#define REGIST_PROCINTERFACE
#define UNREGIST_PROCINTERFACE

/*===============================================================*/
/* debug parameters						 */
/*===============================================================*/
#define _EN_RGB565 0
#define _LCK_DBG   0 /* debug spinlock */         /* 0: disable  1: enable */
#define _LOG_DBG   0 /* debug log      */         /* 0: disable  1: enable */
#define _THR_DBG   0 /* debug thread   */         /* 0: disable  1: enable */
#define _ERR_DBG   0

#define printk_err(fmt, arg...) \
	do { \
		printk(KERN_ERR DEV_NAME ": %s: " fmt, __func__, ## arg); \
	} while (0)

#define printk_wrn(fmt, arg...) \
	do { \
		printk(KERN_WARNING DEV_NAME ": %s: " fmt, __func__, ## arg); \
	} while (0)

#define printk_info(fmt, arg...) \
	do { \
		printk(KERN_INFO DEV_NAME ": " fmt, ## arg);	\
	} while (0)

#define printk_dbg(level, fmt, arg...) \
	do { \
		if (level > 0) \
			printk(KERN_INFO DEV_NAME ": %s: " fmt, \
				__func__, ## arg); \
	} while (0)

#define dbg_lock(lock, fmt, arg...) \
	do { \
		if (_LCK_DBG > 0) \
			if (spin_is_locked(lock)) \
				printk(KERN_DEBUG DEV_NAME ": %s: " fmt, \
				 __func__, ## arg); \
	} while (0)


static long ape5r_v4l2_core_ioctl(struct file *file, unsigned int cmd, \
	unsigned long arg);
static inline unsigned int ape5r_v4l2_core_poll(struct file *file,     \
	struct poll_table_struct *wait);
static int ape5r_v4l2_core_open(struct file *file);
static int ape5r_v4l2_core_release(struct file *file);
static int ape5r_v4l2_core_mmap(struct file *file, struct vm_area_struct *vma);

static int ape5r_v4l2_thread_postmessage(struct ape5r_v4l2_device *dev, \
  int message, int (*confirm_condition)(struct ape5r_v4l2_device *dev));

static void ape5r_v4l2_core_resetdefault(struct ape5r_v4l2_device *dev);

/*===============================================================*/
/* file_operations						 */
/*===============================================================*/

static struct v4l2_file_operations ape5r_v4l2_core_fops = {
	.owner		= THIS_MODULE,
	.ioctl		= ape5r_v4l2_core_ioctl,
	.mmap		= ape5r_v4l2_core_mmap,
	.poll		= ape5r_v4l2_core_poll,
	.open		= ape5r_v4l2_core_open,
	.release	= ape5r_v4l2_core_release,
};


/*===============================================================*/
/* device_driver                                                 */
/*===============================================================*/
static struct platform_driver ape5r_v4l2_core_driver = {
	.probe		= NULL,
	.remove		= NULL,
	.shutdown	= NULL,
	.suspend	= NULL,
	.resume		= NULL,
	.driver.name	= DEV_NAME,
	.driver.bus	= &platform_bus_type,
};

static struct platform_device ape5r_v4l2_core_device = {
	.name	= DEV_NAME,
	.dev	= {
			.release	= NULL,
		  },
	.id	= 0,
};


/*===============================================================*/
/* per-device data structure                                     */
/*===============================================================*/

struct ape5r_v4l2_fh {
	struct ape5r_v4l2_device	*dev;
	enum   v4l2_buf_type	type;
	struct videobuf_queue	vbq;
};



static struct ape5r_v4l2_device *ape5r_dev;
static int video_nr = -1;	/* video device minor (-1 ==> auto assign) */
static LIST_HEAD(devlist);

/*------------------------------------------------
** dump data
------------------------------------------------*/
#if _LOG_DBG
static void dump_buffer(struct ape5r_v4l2_videobuffer *emvb)
{
	struct videobuf_buffer *vb = &emvb->vb;

	printk_dbg(_LOG_DBG,                                          \
		"=== buffer info() =======================================\n");
	printk_dbg(_LOG_DBG, " timestamp: sec(%ld), usec(%ld)\n",     \
		vb->ts.tv_sec, vb->ts.tv_usec);
	printk_dbg(_LOG_DBG, " i        : %d\n",                  vb->i);
	printk_dbg(_LOG_DBG, " w x h    : %d x %d\n",                 \
		vb->width, vb->height);
	printk_dbg(_LOG_DBG, " bytesperline: %d\n",                   \
		vb->bytesperline);
	printk_dbg(_LOG_DBG, " size        : 0x%x\n",          (int)vb->size);
	printk_dbg(_LOG_DBG, " input       : %d\n",               vb->input);
	printk_dbg(_LOG_DBG, " field       : %d\n",               vb->field);
	printk_dbg(_LOG_DBG, " state       : %d\n",               vb->state);
	printk_dbg(_LOG_DBG, " bsize       : 0x%x\n",               vb->bsize);
	printk_dbg(_LOG_DBG, " boff        : 0x%x\n",               vb->boff);
	printk_dbg(_LOG_DBG, " baddr       : 0x%x\n",          (int)vb->baddr);
	printk_dbg(_LOG_DBG, " field_count : %d\n",                   \
		vb->field_count);
	printk_dbg(_LOG_DBG, " src:   width(%4d), height(%4d), pitch(%4d)\n", \
		emvb->pix.width, emvb->pix.height, emvb->pix.bytesperline);
	printk_dbg(_LOG_DBG, " window:left(%4d), top(%4d), width(%4d),"
		" height(%4d)\n",                                \
		emvb->win.w.left, emvb->win.w.top,               \
		emvb->win.w.width, emvb->win.w.height);
	printk_dbg(_LOG_DBG,                                          \
		"=========================================================\n");
}

#endif

#if RTAPI_MEMORY_INTERFACE
static void v4l2_callback(void *arg)
{
	unsigned long flags;

	struct ape5r_v4l2_videobuffer *emvb =          \
		(struct ape5r_v4l2_videobuffer *)arg;
	struct videobuf_queue *vbq     = emvb->vbq;
	struct ape5r_v4l2_device *dev = vbq->priv_data;

	printk_dbg(_LOG_DBG, "callback from display\n");

	dbg_lock(vbq->irqlock, "vbq_lock\n");
	spin_lock_irqsave(vbq->irqlock, flags);

	if (emvb->vb.state == VIDEOBUF_ACTIVE) {
		list_del(&emvb->work_queue);
		INIT_LIST_HEAD(&emvb->work_queue);
		if (dev->vbq_worknum > 0) {
			/* decrement work_queue count */
			dev->vbq_worknum--;
		}
		do_gettimeofday(&emvb->vb.ts);
		emvb->vb.state = VIDEOBUF_DONE;
		wake_up(&emvb->vb.done);
	}
#if _LOG_DBG
	dump_buffer(emvb);  /* debug log */
#endif

	spin_unlock_irqrestore(vbq->irqlock, flags);
	ape5r_v4l2_thread_postmessage(dev, EVENT_REQUESTNEXT, NULL);
}
#endif
/*****************************************************************************
* MODULE   : find_pixelformat_information
* FUNCTION :
* RETURN   :	   0: success
*	   : -EINVAL: error
* NOTE	   : This function return pixel informations.
*	   : param[0-2]: linebytes per plane.
*	   : param[3-5]: width     per plane.
*	   : param[6-8]: height    per plane.
******************************************************************************/
static int find_pixelformat_information(struct v4l2_pix_format *pix,   \
	unsigned int param[9])
{
	static struct {
		unsigned int fmt;
		int h_smp;
		int v_smp;
		int y_bpp;
		int u_bpp;
		int v_bpp;
	} table[] = {{ V4L2_PIX_FMT_YUV420, 2, 2, 1, 1, 1 },
		    {  V4L2_PIX_FMT_NV12,   2, 2, 1, 2, 0 },
		    {  V4L2_PIX_FMT_NV16,   2, 1, 1, 2, 0 },
		    {  V4L2_PIX_FMT_UYVY,   2, 1, 2, 0, 0 },
#if _EN_RGB565
		    {  V4L2_PIX_FMT_RGB565, 1, 1, 2, 0, 0 },
#endif
		    {  0,                   0, 0, 0, 0, 0 } };
	int i, w, h;

	w = pix->width;
	h = pix->height;
	for (i = 0; table[i].fmt != 0; i++) {
		if (table[i].fmt == pix->pixelformat)
			break;
	}
	if (table[i].fmt == 0) {
		printk_err("pixel format %x not supported.\n",        \
			pix->pixelformat);
		return -EINVAL;
	}

	/* width */
	param[3] = w;
	param[4] = (table[i].u_bpp) ? (w / (table[i].h_smp)) : 0;
	param[5] = (table[i].v_bpp) ? (w / (table[i].h_smp)) : 0;

	/* height */
	param[6] = h;
	param[7] = (table[i].u_bpp) ? (h / (table[i].v_smp)) : 0;
	param[8] = (table[i].v_bpp) ? (h / (table[i].v_smp)) : 0;

	/* line bytes */
	param[0] = param[3] * (table[i].y_bpp);
	param[1] = param[4] * (table[i].u_bpp);
	param[2] = param[5] * (table[i].v_bpp);

	printk_dbg(_LOG_DBG, "pixelformat: %x  w:%d h:%d\n",       \
		pix->pixelformat, w, h);
	printk_dbg(_LOG_DBG, "  plane1:  %d x %d .. %d\n",         \
		param[3], param[6], param[0]);
	printk_dbg(_LOG_DBG, "  plane2:  %d x %d .. %d\n",         \
		param[4], param[7], param[1]);
	printk_dbg(_LOG_DBG, "  plane3:  %d x %d .. %d\n",         \
		param[5], param[8], param[2]);
	return 0;
}


/*****************************************************************************
* MODULE   : confirm_device_parameters
* FUNCTION :
* RETURN   :	   0: success
*	   : -EINVAL: error
* NOTE	   : This function confirm parameters for display LCD
******************************************************************************/
static int confirm_device_parameters(struct ape5r_v4l2_device *dev)
{
	unsigned int    param[9];
	struct v4l2_rect *crop_rect = &dev->crop.c;
	struct v4l2_rect *win_rect  = &dev->win.w;
	struct v4l2_pix_format *pix = &dev->pix;

	char *errmsg = NULL;
	/* check custom parameter(output) */
	switch (dev->output) {
	case V4L2_OUTPUT_LCD1:
	case V4L2_OUTPUT_LCD2:
		break;
	default:
		errmsg = "VIDIOC_S_CTRL(id V4L2_CID_DISPLAY_OUTPUT)";
		goto err_ret;
	}
	/* check custom parameter(rotate) */
	if (dev->rotate < 0 || dev->rotate > 3) {
		errmsg = "VIDIOC_S_CTRL(id V4L2_CID_ROTATE)";
		goto err_ret;
	}
	/* check custom parameter(mirror) */
	if (dev->mirror < 0 || dev->mirror > 2) {
		errmsg = "VIDIOC_S_CTRL(id V4L2_CID_DISPLAY_MIRROR)";
		goto err_ret;
	}
	/* check field */
	switch (dev->pix.field) {
	case V4L2_FIELD_NONE:
	case V4L2_FIELD_ANY:
		break;
	default:
		errmsg = "VIDIOC_S_FMT(V4L2_BUF_TYPE_VIDEO_OUTPUT) field";
		goto err_ret;
	}
	/* check colorspace parameter */
	switch (dev->pix.colorspace) {
	case V4L2_COLORSPACE_BT709_COMPRESS:
	case V4L2_COLORSPACE_BT709_FULLSCALE:
	case V4L2_COLORSPACE_BT601_COMPRESS:
	case V4L2_COLORSPACE_BT601_FULLSCALE:
		break;
	default:
		errmsg = "VIDIOC_S_FMT(V4L2_BUF_TYPE_VIDEO_OUTPUT) colorspace";
		goto err_ret;
	}
	/* check format */
	if (find_pixelformat_information(pix , param)) {
		errmsg = "VIDIOC_S_FMT(V4L2_BUF_TYPE_VIDEO_OUTPUT) pixelformat";
		goto err_ret;
	}
	/* check pix parameters */
	if ((pix->width & 1) != 0 ||
	   pix->width < 16 || pix->width > 1920) {
		errmsg = "VIDIOC_S_FMT(V4L2_BUF_TYPE_VIDEO_OUTPUT) width";
		goto err_ret;
	}
	if ((pix->height & 1) != 0 ||
	   pix->height < 16 || pix->height > 1920) {
		errmsg = "VIDIOC_S_FMT(V4L2_BUF_TYPE_VIDEO_OUTPUT) height";
		goto err_ret;
	}
	if ((pix->height * pix->width) > 1920*1080) {
		errmsg = "VIDIOC_S_FMT"
			 "(V4L2_BUF_TYPE_VIDEO_OUTPUT) width*height";
		goto err_ret;
	}
	if (pix->bytesperline != 0) {
		if (pix->bytesperline < param[0]) {
			errmsg = "VIDIOC_S_FMT"
				 "(V4L2_BUF_TYPE_VIDEO_OUTPUT) bytesperline";
			goto err_ret;
		}
	}
	/* check crop parameters */
	if ((crop_rect->left & 1) != 0 ||
	     crop_rect->left >= pix->width) {
		errmsg = "VIDIOC_S_CROP(V4L2_BUF_TYPE_VIDEO_OVERLAY) left";
		goto err_ret;
	}
	if ((crop_rect->top & 1) != 0 ||
	     crop_rect->top >= pix->height) {
		errmsg = "VIDIOC_S_CROP(V4L2_BUF_TYPE_VIDEO_OVERLAY) top";
		goto err_ret;
	}
	if ((crop_rect->width & 1) != 0 ||
	    (crop_rect->left+crop_rect->width) > pix->width) {
		errmsg = "VIDIOC_S_CROP(V4L2_BUF_TYPE_VIDEO_OVERLAY) width";
		goto err_ret;
	}
	if ((crop_rect->height & 1) != 0 ||
	    (crop_rect->top+crop_rect->height) > pix->height) {
		errmsg = "VIDIOC_S_CROP(V4L2_BUF_TYPE_VIDEO_OVERLAY) height";
		goto err_ret;
	}
	/* check window parameters */
	if ((win_rect->left & 1) != 0 ||
	     win_rect->left > 1919) {
		errmsg = "VIDIOC_S_FMT(V4L2_BUF_TYPE_VIDEO_OVERLAY) left";
		goto err_ret;
	}
	if ((win_rect->top & 1) != 0 ||
	     win_rect->top > 1079) {
		errmsg = "VIDIOC_S_FMT(V4L2_BUF_TYPE_VIDEO_OVERLAY) top";
		goto err_ret;
	}
	if ((win_rect->width & 1) != 0 ||
	     win_rect->width < 16 || (win_rect->left+win_rect->width) > 1920) {
		errmsg = "VIDIOC_S_FMT(V4L2_BUF_TYPE_VIDEO_OVERLAY) width";
		goto err_ret;
	}
	if ((win_rect->height & 1) != 0 ||
	     win_rect->height < 16 || (win_rect->top+win_rect->height) > 1080) {
		errmsg = "VIDIOC_S_FMT(V4L2_BUF_TYPE_VIDEO_OVERLAY) height";
		goto err_ret;
	}
	/* check scaler configuration */
	if (win_rect->width != crop_rect->width) {
		if (win_rect->width    > (crop_rect->width*8) ||
		   (win_rect->width*8) < crop_rect->width) {
			errmsg = "VIDIOC_S_FMT"
				 "(V4L2_BUF_TYPE_VIDEO_OVERLAY) scaler_w";
			goto err_ret;
		}
	}
	if (win_rect->height != crop_rect->height) {
		if (win_rect->height    > (crop_rect->height*8) ||
		   (win_rect->height*8) < crop_rect->height) {
			errmsg = "VIDIOC_S_FMT"
				 "(V4L2_BUF_TYPE_VIDEO_OVERLAY) scaler_h";
			goto err_ret;
		}
	}
err_ret:
	if (errmsg) {
		printk_dbg(_ERR_DBG, "parameter error detected:[%s]\n", errmsg);
		return -1;
	} else {
		return 0;
	}
}

/*****************************************************************************
* MODULE   : confirm_device_parameters_HDMI
* FUNCTION :
* RETURN   :	   0: success
*	   : -EINVAL: error
* NOTE	   : This function confirm parameters for display LCD
******************************************************************************/
static int confirm_device_parameters_HDMI(struct ape5r_v4l2_device *dev)
{
	unsigned int    param[9];
	struct v4l2_rect *crop_rect = &dev->crop.c;
	struct v4l2_rect *win_rect  = &dev->win.w;
	struct v4l2_pix_format *pix = &dev->pix;

	char *errmsg = NULL;
	/* check custom parameter(output) */
	switch (dev->output) {
	case V4L2_OUTPUT_HDMI_1080I:
	case V4L2_OUTPUT_HDMI_720P:
	case V4L2_OUTPUT_HDMI_480P:
	case V4L2_OUTPUT_HDMI_1080P:
		break;
	default:
		errmsg = "VIDIOC_S_CTRL(id V4L2_CID_DISPLAY_OUTPUT)";
		goto err_ret;
	}
	/* check custom parameter(rotate) */
	if (dev->rotate < 0 || dev->rotate > 3) {
		errmsg = "VIDIOC_S_CTRL(id V4L2_CID_ROTATE)";
		goto err_ret;
	}
	/* check custom parameter(mirror) */
	if (dev->mirror < 0 || dev->mirror > 2) {
		errmsg = "VIDIOC_S_CTRL(id V4L2_CID_DISPLAY_MIRROR)";
		goto err_ret;
	}
	/* check field */
	switch (dev->pix.field) {
	case V4L2_FIELD_NONE:
	case V4L2_FIELD_ANY:
		break;
	default:
		errmsg = "VIDIOC_S_FMT(V4L2_BUF_TYPE_VIDEO_OUTPUT) field";
		goto err_ret;
	}
	/* check colorspace parameter */
	switch (dev->pix.colorspace) {
	case V4L2_COLORSPACE_BT709_COMPRESS:
	case V4L2_COLORSPACE_BT709_FULLSCALE:
	case V4L2_COLORSPACE_BT601_COMPRESS:
	case V4L2_COLORSPACE_BT601_FULLSCALE:
		break;
	default:
		errmsg = "VIDIOC_S_FMT(V4L2_BUF_TYPE_VIDEO_OUTPUT) colorspace";
		goto err_ret;
	}
	/* check format */
	if (find_pixelformat_information(pix , param)) {
		errmsg = "VIDIOC_S_FMT(V4L2_BUF_TYPE_VIDEO_OUTPUT) pixelformat";
		goto err_ret;
	}
	/* check pix parameters */
	if ((pix->width & 1) != 0 ||
	   pix->width < 16 || pix->width > 1920) {
		errmsg = "VIDIOC_S_FMT(V4L2_BUF_TYPE_VIDEO_OUTPUT) width";
		goto err_ret;
	}
	if ((pix->height & 1) != 0 ||
	   pix->height < 16 || pix->height > 1920) {
		errmsg = "VIDIOC_S_FMT(V4L2_BUF_TYPE_VIDEO_OUTPUT) height";
		goto err_ret;
	}
	if ((pix->height * pix->width) > 1920*1080) {
		errmsg = "VIDIOC_S_FMT"
			 "(V4L2_BUF_TYPE_VIDEO_OUTPUT) width*height";
		goto err_ret;
	}
	if (pix->bytesperline != 0) {
		if (pix->bytesperline < param[0]) {
			errmsg = "VIDIOC_S_FMT"
				 "(V4L2_BUF_TYPE_VIDEO_OUTPUT) bytesperline";
			goto err_ret;
		}
	}
	/* check crop parameters */
	if ((crop_rect->left & 1) != 0 ||
	     crop_rect->left >= pix->width) {
		errmsg = "VIDIOC_S_CROP(V4L2_BUF_TYPE_VIDEO_OVERLAY) left";
		goto err_ret;
	}
	if ((crop_rect->top & 1) != 0 ||
	     crop_rect->top >= pix->height) {
		errmsg = "VIDIOC_S_CROP(V4L2_BUF_TYPE_VIDEO_OVERLAY) top";
		goto err_ret;
	}
	if ((crop_rect->width & 1) != 0 ||
	    (crop_rect->left+crop_rect->width) > pix->width) {
		errmsg = "VIDIOC_S_CROP(V4L2_BUF_TYPE_VIDEO_OVERLAY) width";
		goto err_ret;
	}
	if ((crop_rect->height & 1) != 0 ||
	    (crop_rect->top+crop_rect->height) > pix->height) {
		errmsg = "VIDIOC_S_CROP(V4L2_BUF_TYPE_VIDEO_OVERLAY) height";
		goto err_ret;
	}
	/* check window parameters */
	if ((win_rect->left & 1) != 0 ||
	     win_rect->left > 1919) {
		errmsg = "VIDIOC_S_FMT(V4L2_BUF_TYPE_VIDEO_OVERLAY) left";
		goto err_ret;
	}
	if ((win_rect->top & 1) != 0 ||
	     win_rect->top > 1080) {
		errmsg = "VIDIOC_S_FMT(V4L2_BUF_TYPE_VIDEO_OVERLAY) top";
		goto err_ret;
	}
	if ((win_rect->width & 1) != 0 ||
	     win_rect->width < 16 || (win_rect->left+win_rect->width) > 1920) {
		errmsg = "VIDIOC_S_FMT(V4L2_BUF_TYPE_VIDEO_OVERLAY) width";
		goto err_ret;
	}
	if ((win_rect->height & 1) != 0 ||
	     win_rect->height < 16 || (win_rect->top+win_rect->height) > 1080) {
		errmsg = "VIDIOC_S_FMT(V4L2_BUF_TYPE_VIDEO_OVERLAY) height";
		goto err_ret;
	}
	/* check scaler configuration */
	if (win_rect->width != crop_rect->width) {
		if (win_rect->width    > (crop_rect->width*8) ||
		   (win_rect->width*8) < crop_rect->width) {
			errmsg = "VIDIOC_S_FMT"
				 "(V4L2_BUF_TYPE_VIDEO_OVERLAY) scaler_w";
			goto err_ret;
		}
	}
	if (win_rect->height != crop_rect->height) {
		if (win_rect->height    > (crop_rect->height*8) ||
		   (win_rect->height*8) < crop_rect->height) {
			errmsg = "VIDIOC_S_FMT"
				 "(V4L2_BUF_TYPE_VIDEO_OVERLAY) scaler_h";
			goto err_ret;
		}
	}
err_ret:
	if (errmsg) {
		printk_dbg(_ERR_DBG, "parameter error detected:[%s]\n", errmsg);
		return -1;
	} else {
		return 0;
	}
}

/*****************************************************************************
* MODULE   : ape5r_v4l2_ioc_querycap
* FUNCTION : sub function of ape5r_v4l2_core_do_ioctl()
* RETURN   :	   0: success
* NOTE	   : this function is substance of the ioctl system call.
*	   :	VIDIOC_QUERYCAP
******************************************************************************/
static inline int ape5r_v4l2_ioc_querycap(struct ape5r_v4l2_device *dev,
 struct v4l2_capability *cap)
{
	memset(cap, 0, sizeof(*cap));
	strlcpy(cap->driver, DEV_NAME, sizeof(cap->driver));
	cap->version = DEV_VERSION;
	cap->capabilities = V4L2_CAP_STREAMING;
	return 0;
}
/*****************************************************************************
* MODULE   : ape5r_v4l2_ioc_enumfmt
* FUNCTION : sub function of ape5r_v4l2_core_do_ioctl()
* RETURN   :	   0: success
* NOTE	   : this function is substance of the ioctl system call.
*	   :	VIDIOC_ENUM_FMT
******************************************************************************/
static inline int ape5r_v4l2_ioc_enumfmt(struct ape5r_v4l2_device *dev,
 struct v4l2_fmtdesc *fmt)
{
	enum v4l2_buf_type type  = fmt->type;
	__u32              index = fmt->index;
	char               *desc;
	__u32              format;
	if (type != V4L2_BUF_TYPE_VIDEO_OUTPUT)
		return -EINVAL;

	switch (index) {
	case 0:
		format = V4L2_PIX_FMT_YUV420; desc = "YUV 4:2:0(PL)";
		break;
	case 1:
		format = V4L2_PIX_FMT_NV12;   desc = "YUV 4:2:0(SP)";
		break;
	case 2:
		format = V4L2_PIX_FMT_NV16;   desc = "YUV 4:2:2(SP)";
		break;
	case 3:
		format = V4L2_PIX_FMT_UYVY;   desc = "YUV 4:2:2(packed)";
		break;
	default:
		return -EINVAL;
	}
	memset(fmt, 0, sizeof(*fmt));
	fmt->index = index;
	fmt->type = type;
	fmt->pixelformat = format;
	strlcpy(fmt->description, desc, sizeof(fmt->description)-1);
	return 0;
}
/*****************************************************************************
* MODULE   : ape5r_v4l2_ioc_g_fmt
* FUNCTION : sub function of ape5r_v4l2_core_do_ioctl()
* RETURN   :	   0: success
*	   : -EINVAL: misstype fmt->type
* NOTE	   : this function is substance of the ioctl system call.
*	   :	VIDIOC_G_FMT
******************************************************************************/
static int ape5r_v4l2_ioc_g_fmt(struct ape5r_v4l2_device *dev,
 struct v4l2_format *fmt)
{
	struct v4l2_workbuffer *wbuf;
	int retval = 0;
	unsigned long flags;

	wbuf = (struct v4l2_workbuffer *)fmt->fmt.raw_data;

	dbg_lock(&dev->vbq_lock, "vbq_lock\n");
	spin_lock_irqsave(&dev->vbq_lock, flags);
	if (fmt->type == V4L2_BUF_TYPE_VIDEO_OUTPUT) {
		/* get the current format */
		fmt->fmt.pix = dev->pix;
	} else if (fmt->type == V4L2_BUF_TYPE_VIDEO_OVERLAY) {
		/* get overlay info */
		fmt->fmt.win = dev->win;
	} else if (fmt->type == V4L2_BUF_TYPE_PRIVATE) {
		/* get workbuffer info */
		memcpy(wbuf, &dev->workbuf, sizeof(struct v4l2_workbuffer));
	} else {
		retval = -EINVAL;
	}
	spin_unlock_irqrestore(&dev->vbq_lock, flags);
	return retval;
}

/*****************************************************************************
* MODULE   : ape5r_v4l2_ioc_s_fmt
* FUNCTION : sub function of ape5r_v4l2_core_do_ioctl()
* RETURN   :	   0: success
*          : -EINVAL: misstype fmt->type
* NOTE	   : this function is substance of the ioctl system call.
*          :	VIDIOC_S_FMT
******************************************************************************/
static int ape5r_v4l2_ioc_s_fmt(struct ape5r_v4l2_device *dev,
 struct v4l2_format *fmt)
{
	int retval = 0;
#if RTAPI_MEMORY_INTERFACE
	int update_mem = 0;
#endif
	unsigned long flags;

	dbg_lock(&dev->vbq_lock, "vbq_lock\n");
	spin_lock_irqsave(&dev->vbq_lock, flags);
	if (fmt->type == V4L2_BUF_TYPE_VIDEO_OUTPUT) {
		if (dev->streaming) {
			printk_err("s_fmt failed [streaming]\n");
			retval = -EINVAL;
		} else {
			/* set image info */
			dev->pix.width	      = fmt->fmt.pix.width;
			dev->pix.height	      = fmt->fmt.pix.height;
			dev->pix.pixelformat  = fmt->fmt.pix.pixelformat;
			dev->pix.bytesperline = fmt->fmt.pix.bytesperline;
			dev->pix.field	      = fmt->fmt.pix.field;
			dev->pix.colorspace   = fmt->fmt.pix.colorspace;

			dev->status |= STAT_CONFIG_SFMTOUT;
			if ((dev->status & STAT_CONFIG_CROPOVL) == 0) {
				dev->crop.c.left   = 0;
				dev->crop.c.top    = 0;
				dev->crop.c.width  = fmt->fmt.pix.width;
				dev->crop.c.height = fmt->fmt.pix.height;
			}
		}
	} else if (fmt->type == V4L2_BUF_TYPE_VIDEO_OVERLAY) {
		/* set overlay info */
		dev->win.w.left     = fmt->fmt.win.w.left;
		dev->win.w.top      = fmt->fmt.win.w.top;
		dev->win.w.width    = fmt->fmt.win.w.width;
		dev->win.w.height   = fmt->fmt.win.w.height;
		/* set alpha and key-color */
		dev->win.chromakey     = fmt->fmt.win.chromakey;
		dev->win.global_alpha  = fmt->fmt.win.global_alpha;
		/* to do: auto correct parameters. */
		dev->status |= STAT_CONFIG_SFMTOVL;
#if RTAPI_MEMORY_INTERFACE
	} else if (fmt->type == V4L2_BUF_TYPE_PRIVATE &&     \
		dev->mMem.mRefCount == 0) {
		struct v4l2_workbuffer *wbuf;

		wbuf = (struct v4l2_workbuffer *)fmt->fmt.raw_data;
		if (wbuf->apmem_addr == 0) {
			/* clear configuration flag */
			dev->status &= ~STAT_CONFIG_SFMTPRV;
		} else {
			/* set configuration flag */
			dev->status |= STAT_CONFIG_SFMTPRV;
		}

		/* set workbuffer info */
		if (dev->workbuf.apmem_id   != wbuf->apmem_id ||
		    dev->workbuf.apmem_addr != wbuf->apmem_addr) {
			/* registered information changed. */
			update_mem = 1;
		}
		dev->workbuf.apmem_id   = wbuf->apmem_id;
		dev->workbuf.apmem_addr = wbuf->apmem_addr;
#endif
	} else {
		retval = -EINVAL;
	}
	spin_unlock_irqrestore(&dev->vbq_lock, flags);

#if RTAPI_MEMORY_INTERFACE
	if (update_mem && retval == 0 && fmt->type == V4L2_BUF_TYPE_PRIVATE) {
		/* remove buffer management I/F */
		if (videobuf_queue_ape5r_interface_config(dev) != 0) {
			printk_err("memory allocation failed.\n");

			dbg_lock(&dev->vbq_lock, "vbq_lock\n");
			spin_lock_irqsave(&dev->vbq_lock, flags);
			/* clear configuration flag */
			dev->status &= ~STAT_CONFIG_SFMTPRV;
			/* clear registered information. */
			memset(&dev->workbuf, 0, sizeof(dev->workbuf));
			spin_unlock_irqrestore(&dev->vbq_lock, flags);
		}
	}
#endif

	return retval;
}
/*****************************************************************************
* MODULE   : ape5r_v4l2_ioc_g_crop
* FUNCTION : sub function of ape5r_v4l2_core_do_ioctl()
* RETURN   :	   0: success
*	   : -EINVAL: misstype vc->type
* NOTE	   : this function is substance of the ioctl system call.
*	   :	VIDIOC_G_CROP
******************************************************************************/
static int ape5r_v4l2_ioc_g_crop(struct ape5r_v4l2_device *dev,
 struct v4l2_crop *vc)
{
	int retval = 0;
	unsigned long flags;

	dbg_lock(&dev->vbq_lock, "vbq_lock\n");
	spin_lock_irqsave(&dev->vbq_lock, flags);
	if (vc->type == V4L2_BUF_TYPE_VIDEO_OVERLAY) {
		/* get crop data of original movie */
		*vc = dev->crop;
	} else {
		retval = -EINVAL;
	}
	spin_unlock_irqrestore(&dev->vbq_lock, flags);
	return retval;
}


/*****************************************************************************
* MODULE   : ape5r_v4l2_ioc_s_crop
* FUNCTION : sub function of ape5r_v4l2_core_do_ioctl()
* RETURN   :	   0: success
*	   : -EINVAL: misstype vc->type
* NOTE	   : this function is substance of the ioctl system call.
*	   :	VIDIOC_S_CROP
******************************************************************************/
static int ape5r_v4l2_ioc_s_crop(struct ape5r_v4l2_device *dev,
 struct v4l2_crop *vc)
{
	int retval = 0;
	unsigned long flags;

	dbg_lock(&dev->vbq_lock, "vbq_lock\n");
	spin_lock_irqsave(&dev->vbq_lock, flags);
	if (vc->type == V4L2_BUF_TYPE_VIDEO_OVERLAY) {
		/* set crop data of original movie */
		dev->crop   = *vc;
		/* to do: auto correct parameters. */
		dev->status |= STAT_CONFIG_CROPOVL;
	} else {
		retval = -EINVAL;
	}
	spin_unlock_irqrestore(&dev->vbq_lock, flags);
	return retval;
}
/*****************************************************************************
* MODULE   : ape5r_v4l2_ioc_g_ctrl
* FUNCTION : sub function of ape5r_v4l2_core_do_ioctl()
* RETURN   :	   0: success
*	   : -EINVAL: misstype vc->id
* NOTE	   : this function is substance of the ioctl system call.
*	   :	VIDIOC_G_CTRL
******************************************************************************/
static int ape5r_v4l2_ioc_g_ctrl(struct ape5r_v4l2_device *dev,
 struct v4l2_control *vc)
{
	int retval = 0;
	unsigned long flags;

	dbg_lock(&dev->vbq_lock, "vbq_lock\n");
	spin_lock_irqsave(&dev->vbq_lock, flags);
	if (vc->id == V4L2_CID_ROTATE) {
		/* get current rotation angle */
		vc->value = dev->rotate;
	} else if (vc->id == V4L2_CID_DISPLAY_OUTPUT) {
		/* get current output device */
		vc->value = dev->output;
	} else if (vc->id == V4L2_CID_DISPLAY_MIRROR) {
		/* get current mirror */
		vc->value = dev->mirror;
	} else if (vc->id == V4L2_CID_BG_COLOR) {
		/* get current background color */
		vc->value = dev->bg_color;
	} else {
		retval = -EINVAL;
	}
	spin_unlock_irqrestore(&dev->vbq_lock, flags);
	return retval;
}
/*****************************************************************************
* MODULE   : ape5r_v4l2_ioc_s_ctrl
* FUNCTION : sub function of ape5r_v4l2_core_do_ioctl()
* RETURN   :	   0: success
*	   : -EINVAL: misstype vc->id
* NOTE	   : this function is substance of the ioctl system call.
*	   :	VIDIOC_S_CTRL
******************************************************************************/
static int ape5r_v4l2_ioc_s_ctrl(struct ape5r_v4l2_device *dev,
 struct v4l2_control *vc)
{
	int retval = 0;
	unsigned long flags;

	dbg_lock(&dev->vbq_lock, "vbq_lock\n");
	spin_lock_irqsave(&dev->vbq_lock, flags);
	if (vc->id == V4L2_CID_ROTATE) {
		/* set rotation angle */
		dev->rotate = vc->value;
	} else if (vc->id == V4L2_CID_DISPLAY_MIRROR) {
		/* set mirror */
		dev->mirror = vc->value;
	} else if (vc->id == V4L2_CID_DISPLAY_OUTPUT) {
		/* set display output */
		if (dev->streaming) {
			printk_err("now streaming.");
			retval = -EBUSY;
		} else {
			dev->output = vc->value;
			switch (vc->value) {
			case V4L2_OUTPUT_LCD1:
			case V4L2_OUTPUT_LCD2:
				dev->check_param = confirm_device_parameters;
				break;
			case V4L2_OUTPUT_HDMI_1080I:
			case V4L2_OUTPUT_HDMI_720P:
			case V4L2_OUTPUT_HDMI_480P:
			case V4L2_OUTPUT_HDMI_1080P:
				dev->check_param = \
					confirm_device_parameters_HDMI;
				break;
			default:
				retval = -EINVAL;
				break;
			}
		}
	} else if (vc->id == V4L2_CID_BG_COLOR) {
		if (dev->streaming) {
			printk_err("now streaming.");
			retval = -EBUSY;
		} else {
			/* get current background color */
			dev->bg_color = vc->value;
		}
	} else {
		retval = -EINVAL;
	}
	spin_unlock_irqrestore(&dev->vbq_lock, flags);
	return retval;
}
/*****************************************************************************
* MODULE   : ape5r_v4l2_ioc_reqbufs
* FUNCTION : sub function of ape5r_v4l2_core_do_ioctl()
* RETURN   :	   0: success
* NOTE	   : this function is substance of the ioctl system call.
*	   :	VIDIOC_REQBUFS
******************************************************************************/
static inline int ape5r_v4l2_ioc_reqbufs(struct ape5r_v4l2_device *dev, \
	struct videobuf_queue *vbq, struct v4l2_requestbuffers *req)
{
	/* request memory for queue */
	return videobuf_reqbufs(vbq, req);
}
/*****************************************************************************
* MODULE   : ape5r_v4l2_ioc_querybuf
* FUNCTION : sub function of ape5r_v4l2_core_do_ioctl()
* RETURN   :	   0: success
*	   : -EINVAL: invalid argument
* NOTE	   : this function is substance of the ioctl system call.
*	   :	VIDIOC_QUERYBUF
******************************************************************************/
static inline int ape5r_v4l2_ioc_querybuf(struct videobuf_queue *q,    \
	struct v4l2_buffer *b)
{
	/* query buffer */
	return videobuf_querybuf(q, b);
}

/*****************************************************************************
* MODULE   : ape5r_v4l2_ioc_qbuf
* FUNCTION : sub function of ape5r_v4l2_core_do_ioctl()
* RETURN   :	   0: success
* NOTE	   : this function is substance of the ioctl system call.
*	   :	VIDIOC_QBUF
******************************************************************************/
static inline int ape5r_v4l2_ioc_qbuf(struct ape5r_v4l2_device *dev,
 struct videobuf_queue *vbq, struct v4l2_buffer *vb)
{
	/* enqueue videoframe-buffer */
	printk_dbg(_LOG_DBG, "queue buffer index:%d\n", vb->index);
	return videobuf_qbuf(vbq, vb);
}


/*****************************************************************************
* MODULE   : ape5r_v4l2_ioc_dqbuf
* FUNCTION : sub function of ape5r_v4l2_core_do_ioctl()
* RETURN   :	   0: success
* NOTE	   : this function is substance of the ioctl system call.
*	   :	VIDIOC_DQBUF
******************************************************************************/
static inline int ape5r_v4l2_ioc_dqbuf(struct ape5r_v4l2_device *dev,
 struct videobuf_queue *vbq, struct v4l2_buffer *vb, int f_flags)
{
	/* dequeue videoframe-buffer */
	int ret = videobuf_dqbuf(vbq, vb, f_flags & O_NONBLOCK);
	printk_dbg(_LOG_DBG, "dequeue buffer index:%d\n", vb->index);
	return ret;
}
/*****************************************************************************
* MODULE   : ape5r_v4l2_ioc_streamon
* FUNCTION : sub function of ape5r_v4l2_core_do_ioctl()
* RETURN   :	   0: success
*	   :  -EBUSY:
* NOTE	   : this function is substance of the ioctl system call.
*	   :	VIDIOC_STREAMON
******************************************************************************/
static int ape5r_v4l2_ioc_streamon_wait(struct ape5r_v4l2_device *dev)
{
	return (dev->status & STAT_STREAMINGON) != 0;
}
static int ape5r_v4l2_ioc_streamon(struct ape5r_v4l2_fh *fh,
 struct ape5r_v4l2_device *dev)
{
	int retval = 0;
	unsigned long flags;
	down(&dev->sem_lock);

	dbg_lock(&dev->vbq_lock, "vbq_lock\n");
	spin_lock_irqsave(&dev->vbq_lock, flags);
	if (dev->streaming) {
		/* already started */
		retval = -EBUSY;
	} else if ((dev->status & STAT_CONFIG_SFMTOUT) == 0) {
		/* this condition, can not create overlay. */
		retval = -EINVAL;
	} else {
		/* initialize stream parameter */
		dev->streaming = fh;
	}
	spin_unlock_irqrestore(&dev->vbq_lock, flags);

	if (retval == 0) {
		retval = videobuf_streamon(&fh->vbq);
		if (retval == 0) {
			retval = ape5r_v4l2_thread_postmessage(dev,    \
			    EVENT_START, ape5r_v4l2_ioc_streamon_wait);
			if (retval != 0) {
				printk_err(                             \
					"ape5r_v4l2_thread_postmessage fail");
				retval = -EBUSY;
				/* this error should not happened. because
				   V4L2 driver will dead-locked at streamoff. */
			}
		} else {
			/* stream on failed. */
			dev->streaming = NULL;
		}
		if (retval != 0) {
			/* report error */
			printk_dbg(_LOG_DBG, "streamon failed.\n");
		} else {
			printk_dbg(_LOG_DBG, "streamon %p %p\n",        \
				dev->streaming, fh);
		}
	}
	up(&dev->sem_lock);
	return retval;
}
/*****************************************************************************
* MODULE   : ape5r_v4l2_ioc_streamoff
* FUNCTION : sub function of ape5r_v4l2_core_do_ioctl()
* RETURN   :	   0: success
*	   :	 err:
* NOTE	   : this function is substance of the ioctl system call.
*	   :	VIDIOC_STREAMOFF
******************************************************************************/
static int ape5r_v4l2_ioc_streamoff_wait(struct ape5r_v4l2_device *dev)
{
	return (dev->status & STAT_STREAMINGOFF) != 0;
}

static int ape5r_v4l2_ioc_streamoff(struct ape5r_v4l2_fh *fh,
 struct ape5r_v4l2_device *dev)
{
	int retval = 0;
	down(&dev->sem_lock);

	if (dev->streaming == NULL) {
		printk_dbg(_ERR_DBG, "streamoff ignored.\n");
		retval = -EINVAL;
	} else {
		/* send message and wait complete. */
		retval = ape5r_v4l2_thread_postmessage(dev, EVENT_STOP, \
			    ape5r_v4l2_ioc_streamoff_wait);
		if (retval != 0) {
			/* report error */
			printk_err("ape5r_v4l2_thread_postmessage fail");
		}

		retval = videobuf_streamoff(&dev->streaming->vbq);
		if (retval < 0) {
			printk_err("streamoff return error\n");
		} else {
			printk_dbg(_LOG_DBG, "streamoff %p %p\n",       \
				dev->streaming, fh);

			/* reset internal variable. */
			dev->streaming   = NULL;
			dev->sequence    = 0;
		}
	}
	up(&dev->sem_lock);
	return retval;
}

/*****************************************************************************
* MODULE   : ape5r_v4l2_core_vbq_setup
* FUNCTION :
* RETURN   : 0: success
* NOTE	   : Limit the number of available kernel image capture buffers based
*	   : on the number requested, the currently selected image size,
*	   : and the maximum amount of memory permitted for kernel buffers.
******************************************************************************/
static int ape5r_v4l2_core_vbq_setup(struct videobuf_queue *q,
 unsigned int *cnt, unsigned int *size)
{
	int retval = 0;
	unsigned int param[9];
	struct ape5r_v4l2_device *dev = q->priv_data;
	unsigned long flags;
	uint32_t      dev_pix_bytesperline;

	dbg_lock(&dev->vbq_lock, "vbq_lock\n");
	spin_lock_irqsave(&dev->vbq_lock, flags);

	/* do not check *cnt, because available memory is not restricted. */
	if (find_pixelformat_information(&dev->pix, param)) {
		printk_err("pixel format not correct.\n");
		retval = -EINVAL; /* this error will not occured. */
	}
	dev_pix_bytesperline = dev->pix.bytesperline;
	spin_unlock_irqrestore(&dev->vbq_lock, flags);

	if (retval == 0) {
		unsigned int linebyte, uv_linebyte, total_size;

		linebyte = param[0];
		if (dev_pix_bytesperline != 0) {
			if (linebyte > dev_pix_bytesperline) {
				printk_err("bytesperline not correct.\n");
				return -EINVAL;
			}
			linebyte = dev_pix_bytesperline;
		}

		total_size  = linebyte * param[6];
		uv_linebyte = linebyte * param[1] / param[0];
		total_size += uv_linebyte * param[7];
		uv_linebyte = linebyte * param[2] / param[0];
		total_size += uv_linebyte * param[8];

		*size = total_size;
	}
	return retval;
}


/*****************************************************************************
* MODULE   : ape5r_v4l2_core_vbq_prepare
* FUNCTION :
* RETURN   :	   0: success
*	   : -EINVAL: error
* NOTE	   : This function registers information to video buffer.
******************************************************************************/
static int ape5r_v4l2_core_vbq_prepare(struct videobuf_queue *q,
 struct videobuf_buffer *vb, enum v4l2_field field)
{
	struct ape5r_v4l2_device *dev = q->priv_data;
	struct ape5r_v4l2_videobuffer *emvb;
	unsigned long flags;
	unsigned int  retval = 0;
	unsigned int  param[9];

	emvb = (struct ape5r_v4l2_videobuffer *)vb;

	if (find_pixelformat_information(&dev->pix, param)) {
		printk_dbg(_ERR_DBG, "parameter error\n");
		return -EINVAL; /* this error will not occured. */
	}

	if ((unsigned int)vb->baddr & 31) {
		printk_dbg(_ERR_DBG, "userptr require 32 byte align.\n");
		return -EINVAL; /* this error will not occured. */
	}

	dbg_lock(&dev->vbq_lock, "vbq_lock\n");
	spin_lock_irqsave(&dev->vbq_lock, flags);

	/* check buffer arguments. */
	if (dev->check_param(dev)) {
		printk_dbg(_ERR_DBG, "parameter error\n");
		retval = -EINVAL;
	} else {

		/* create buffer parameter */
		vb->field	= field;            /* assign field parameter */
		emvb->vb.field_count = dev->sequence*2;
		emvb->rotate         = dev->rotate;
		emvb->mirror         = dev->mirror;
		vb->width        = dev->pix.width;
		vb->height       = dev->pix.height;

		/* set linebyte */
		vb->bytesperline = param[0];
		if (dev->pix.bytesperline > 0) {
			/* assign user-specified parameter. */
			vb->bytesperline = dev->pix.bytesperline;
		}

		emvb->pix  = dev->pix;        /* may be not necessary. */
		emvb->win  = dev->win;
		emvb->crop = dev->crop;

		/* initialize variables */
		INIT_LIST_HEAD(&vb->stream);
		INIT_LIST_HEAD(&vb->queue);
		init_waitqueue_head(&vb->done);

		INIT_LIST_HEAD(&emvb->work_queue);
		emvb->vbq = q;
	}
	spin_unlock_irqrestore(&dev->vbq_lock, flags);


	if (retval == 0) {
		/* lock buffers. */
		if (vb->state == VIDEOBUF_NEEDS_INIT) {
			retval = videobuf_iolock(q, vb, NULL);
			if (retval < 0) {
				printk_err("videobuf_iolock return error\n");
				return retval;
			}
		}
		vb->state = VIDEOBUF_PREPARED;

		/* update device information. */
		dev->sequence++;
	}


	return retval;
}


/*****************************************************************************
* MODULE   : ape5r_v4l2_core_vbq_queue
* FUNCTION :
* RETURN   : -
* NOTE	   : This function executes video buffer information.
******************************************************************************/
static void ape5r_v4l2_core_vbq_queue(struct videobuf_queue *q,
 struct videobuf_buffer *vb)
{
	struct ape5r_v4l2_device *dev = q->priv_data;
	struct ape5r_th_object *th = &dev->th_lcd;
	int    nummsg;

	/* spin_lock was acquired from videobuf-core.c */

	vb->state = VIDEOBUF_QUEUED;
	list_add_tail(&vb->queue, &dev->vbq_in);

	/* get remaining, thread message. */
	nummsg = (th->wp - th->rp)&7;
	if (nummsg < 4) {
		/* post message to thread */
		ape5r_v4l2_thread_postmessage(dev, EVENT_UPDATEQUEUE, NULL);
	}
}
/*****************************************************************************
* MODULE   : ape5r_v4l2_core_vbq_release
* FUNCTION :
* RETURN   : -
* NOTE	   : This function is called a videobuf_buffer release.
******************************************************************************/
static inline void ape5r_v4l2_core_vbq_release(struct videobuf_queue *q,
 struct videobuf_buffer *vb)
{
	unsigned long flags;
	struct ape5r_v4l2_device *dev = q->priv_data;
	struct ape5r_v4l2_videobuffer *emvb;
	struct list_head       *list;

	printk_dbg(_LOG_DBG, "ape5r_v4l2_core_vbq_release enter\n");
	videobuf_waiton(vb, 0, 0);
	videobuf_ape5rbuf_free(q, vb);

	dbg_lock(&dev->vbq_lock, "vbq_lock\n");
	spin_lock_irqsave(&dev->vbq_lock, flags);

	/* is queue used. */
	if (vb->state == VIDEOBUF_QUEUED) {
		list_del(&vb->queue);
		INIT_LIST_HEAD(&vb->queue);
	}


	/* is work-queue used. */
	emvb = (struct ape5r_v4l2_videobuffer *)vb;
	if (vb->state == VIDEOBUF_ACTIVE) {
		list_del(&emvb->work_queue);
		INIT_LIST_HEAD(&emvb->work_queue);
		if (dev->vbq_worknum > 0) {
			/* decrement work_queue count */
			dev->vbq_worknum--;
		}
	}

	/* fail safe, free work-queue. */
	list_for_each(list, &dev->vbq_work)
	{
		emvb = list_entry(list, struct ape5r_v4l2_videobuffer, \
			work_queue);
		if ((void *)emvb == (void *)vb) {

			list_del(&emvb->work_queue);
			INIT_LIST_HEAD(&emvb->work_queue);
			if (dev->vbq_worknum > 0) {
				/* decrement work_queue count */
				dev->vbq_worknum--;
			}
			break;
		}
	}
	vb->state = VIDEOBUF_NEEDS_INIT;
	spin_unlock_irqrestore(&dev->vbq_lock, flags);

	printk_dbg(_LOG_DBG, "ape5r_v4l2_core_vbq_release leave\n");
}

/*****************************************************************************
* MODULE   : ape5r_v4l2_core_do_ioctl
* FUNCTION :
* RETURN   :	   0: success
*	   : -EINVAL: Argument error
* NOTE	   : this function is substance of the ioctl system call.
*	   : @@ supported ioctl command
*	   :	VIDIOC_S_FMT,	    VIDIOC_G_FMT,	VIDIOC_S_CROP,
*	   :	VIDIOC_G_CROP,	    VIDIOC_S_CTRL,	VIDIOC_G_CTRL,
*	   :	VIDIOC_REQBUFS,	    VIDIOC_S_EFFECT,	VIDIOC_G_EFFECT,
*	   :	VIDIOC_STREAMON,    VIDIOC_STREAMOFF,	VIDIOC_QBUF,
*	   :	VIDIOC_DQBUF
*	   : @@ not supported ioctl command
*	   :	VIDIOC_ENUMINPUT,   VIDIOC_G_INPUT,	VIDIOC_S_INPUT,
*	   :	VIDIOC_ENUM_FMT,    VIDIOC_TRY_FMT,	VIDIOC_QUERYCTRL,
*	   :	VIDIOC_G_FBUF,	    VIDIOC_S_FBUF,	VIDIOC_OVERLAY,
*	   :	VIDIOC_ENUMSTD,	    VIDIOC_G_STD,	VIDIOC_S_STD,
*	   :	VIDIOC_QUERYSTD,    VIDIOC_G_AUDIO,	VIDIOC_S_AUDIO
*	   :	VIDIOC_G_AUDOUT,    VIDIOC_S_AUDOUT,	VIDIOC_G_JPEGCOMP,
*	   :	VIDIOC_S_JPEGCOMP,  VIDIOC_G_TUNER,	VIDIOC_S_TUNER,
*	   :	VIDIOC_G_MODULATOR, VIDIOC_S_MODULATOR, VIDIOC_G_FREQUENCY,
*	   :	VIDIOC_S_FREQUENCY, VIDIOC_QUERYCAP,	VIDIOC_ENUMOUTPUT,
*	   :	VIDIOC_G_OUTPUT,    VIDIOC_S_OUTPUT,	VIDIOC_QUERYBUF
******************************************************************************/
static long ape5r_v4l2_core_do_ioctl(struct file *file, unsigned int cmd,
 void *arg)
{
	struct ape5r_v4l2_fh	 *fh  = file->private_data;
	struct ape5r_v4l2_device *dev = fh->dev;


	switch (cmd) {

	case VIDIOC_G_OUTPUT:	/* FALL THROUGH */
	case VIDIOC_S_OUTPUT:	/* FALL THROUGH */
		/* not supported */
		return -EINVAL;

	case VIDIOC_G_FMT:
		return ape5r_v4l2_ioc_g_fmt(dev, arg);

	case VIDIOC_S_FMT:
		return ape5r_v4l2_ioc_s_fmt(dev, arg);


	case VIDIOC_G_CROP:
		return ape5r_v4l2_ioc_g_crop(dev, arg);

	case VIDIOC_S_CROP:
		return ape5r_v4l2_ioc_s_crop(dev, arg);


	case VIDIOC_G_CTRL:
		return ape5r_v4l2_ioc_g_ctrl(dev, arg);

	case VIDIOC_S_CTRL:
		return ape5r_v4l2_ioc_s_ctrl(dev, arg);



	case VIDIOC_REQBUFS:
		return ape5r_v4l2_ioc_reqbufs(dev, &fh->vbq, arg);


	case VIDIOC_QBUF:
		return ape5r_v4l2_ioc_qbuf(dev, &fh->vbq, arg);


	case VIDIOC_DQBUF:
		return ape5r_v4l2_ioc_dqbuf(dev, &fh->vbq, arg, file->f_flags);


	case VIDIOC_STREAMON:
		return ape5r_v4l2_ioc_streamon(fh, dev);


	case VIDIOC_STREAMOFF:
		return ape5r_v4l2_ioc_streamoff(fh, dev);


	case VIDIOC_QUERYBUF:
		return ape5r_v4l2_ioc_querybuf(&fh->vbq, arg);

	case VIDIOC_QUERYCAP:
		return ape5r_v4l2_ioc_querycap(dev, arg);

	case VIDIOC_ENUM_FMT:	/* FALL THROUGH */
		return ape5r_v4l2_ioc_enumfmt(dev, arg);

	/*--- not supported ---*/

	case VIDIOC_ENUMINPUT:	/* FALL THROUGH */
	case VIDIOC_G_INPUT:	/* FALL THROUGH */
	case VIDIOC_S_INPUT:	/* FALL THROUGH */
	case VIDIOC_TRY_FMT:	/* FALL THROUGH */
	case VIDIOC_QUERYCTRL:
		/* not supported */
		return -EINVAL;

	case VIDIOC_G_FBUF:
	/* FALL THROUGH set the frame buffer parameters */
	case VIDIOC_S_FBUF:
	case VIDIOC_OVERLAY:
		/* not supported */
		return -EINVAL;

	case VIDIOC_ENUMSTD:	/* FALL THROUGH */
	case VIDIOC_G_STD:	/* FALL THROUGH */
	case VIDIOC_S_STD:	/* FALL THROUGH */
	case VIDIOC_QUERYSTD:
		/* we don't have an analog video standard,
		 * so we don't need to implement these ioctls.
		 */
		 return -EINVAL;

	case VIDIOC_G_AUDIO:	/* FALL THROUGH */
	case VIDIOC_S_AUDIO:	/* FALL THROUGH */
	case VIDIOC_G_AUDOUT:	/* FALL THROUGH */
	case VIDIOC_S_AUDOUT:
		/* we don't have any audio inputs or outputs */
		return -EINVAL;

	case VIDIOC_G_JPEGCOMP:	/* FALL THROUGH */
	case VIDIOC_S_JPEGCOMP:
		/* JPEG compression is not supported */
		return -EINVAL;

	case VIDIOC_G_TUNER:		/* FALL THROUGH */
	case VIDIOC_S_TUNER:		/* FALL THROUGH */
	case VIDIOC_G_MODULATOR:	/* FALL THROUGH */
	case VIDIOC_S_MODULATOR:	/* FALL THROUGH */
	case VIDIOC_G_FREQUENCY:	/* FALL THROUGH */
	case VIDIOC_S_FREQUENCY:
		/* we don't have a tuner or modulator */
		return -EINVAL;

	case VIDIOC_ENUMOUTPUT:
		/* not supported */
		return -EINVAL;

	default:
		/* unrecognized ioctl */
		return -ENOIOCTLCMD;
	}
	return 0;
}


/*****************************************************************************
* MODULE   : ape5r_v4l2_core_ioctl
* FUNCTION : a stub function of ioctl
* RETURN   :	   0: success
*	   : -EINVAL: Argument error
* NOTE	   :
******************************************************************************/
static long ape5r_v4l2_core_ioctl(struct file *file, unsigned int cmd,
 unsigned long arg)
{
	return (int)(video_usercopy(file, cmd, arg, ape5r_v4l2_core_do_ioctl));
}

/*****************************************************************************
* MODULE   : ape5r_v4l2_thread
* FUNCTION : start to run request in RTIF.  (TMR thread)
* RETURN   : 0
* NOTE	   :
******************************************************************************/
static int ape5r_v4l2_core_mmap(struct file *file, struct vm_area_struct *vma)
{
	struct ape5r_v4l2_fh *fh = file->private_data;

	return videobuf_mmap_mapper(&fh->vbq, vma);
}

/*****************************************************************************
* MODULE   : ape5r_v4l2_core_poll
* FUNCTION : a system call function of poll & select
* RETURN   :	   0: success
*	   : -EINVAL: Argument error
* NOTE	   :
******************************************************************************/
static inline unsigned int ape5r_v4l2_core_poll(struct file *file,
 struct poll_table_struct *wait)
{
	struct ape5r_v4l2_fh *fh = file->private_data;
	struct ape5r_v4l2_device *dev = fh->dev;
	int retval = 0;

	down(&dev->sem_lock);
	if (dev->streaming == NULL) {
		printk_dbg(_ERR_DBG, "streamoff.\n");
		retval = POLLERR;
	}
	up(&dev->sem_lock);
	if (retval == 0) {
		/* execute videobuf_poll_stream */
		retval = videobuf_poll_stream(file, &fh->vbq, wait);
	}

	return retval;
}


/*****************************************************************************
* MODULE   : ape5r_v4l2_core_release
* FUNCTION : v4l2 driver's close processing function.
* RETURN   : 0: success
* NOTE	   :
******************************************************************************/
static int ape5r_v4l2_core_release(struct file *file)
{
	struct ape5r_v4l2_fh     *fh  = file->private_data;
	struct ape5r_v4l2_device *dev = fh->dev;
	unsigned long flags;
	int err;

	ape5r_v4l2_ioc_streamoff(fh, dev);

	dbg_lock(&dev->vbq_lock, "vbq_lock\n");
	spin_lock_irqsave(&dev->vbq_lock, flags);

	kfree(fh->vbq.read_buf);
	fh->vbq.read_buf = NULL;

	spin_unlock_irqrestore(&dev->vbq_lock, flags);
	err = videobuf_mmap_free(&fh->vbq);
#if RTAPI_MEMORY_INTERFACE
	/* remove buffer management I/F */
	videobuf_queue_ape5r_interface_release(dev);
#endif
	spin_lock_irqsave(&dev->vbq_lock, flags);
	if (err)
		printk_err("videobuf_free failed.\n\n");

	/* free fd dependent information. */
#if 1
	/* reset current configuration. */
	ape5r_v4l2_core_resetdefault(dev);
	dev->active  = 0;                             /* v4l2 unactive */
#endif

	file->private_data = NULL;
	kfree(fh);
	spin_unlock_irqrestore(&dev->vbq_lock, flags);

	return 0;
}


/*****************************************************************************
* MODULE   : ape5r_v4l2_core_open
* FUNCTION : v4l2 driver's open processing function.
* RETURN   :	   0: success
*	   : -ENODEV: corresponding device doesn't exist.
*	   : -ENOMEM: memory that can be used for the kernel is insufficient.
*	   : -EPERM : no privilege in the call origin.
* NOTE	   :
******************************************************************************/

static int ape5r_v4l2_core_open(struct file *file)
{
	int minor = video_devdata(file)->minor;
	struct ape5r_v4l2_device *dev;
	struct ape5r_v4l2_fh	 *fh;
	unsigned long flags;

	if (ape5r_dev && ape5r_dev->vfd && minor == ape5r_dev->vfd->minor)
		dev = ape5r_dev;
	else
		return -ENODEV;

	if (!dev || !dev->vfd)
		return -ENODEV;


	/* allocate per-filehandle data */
	fh = kmalloc(sizeof(*fh), GFP_KERNEL);
	if (NULL == fh)
		return -ENOMEM;

	file->private_data = fh;
	fh->dev = dev;
	fh->type = V4L2_BUF_TYPE_VIDEO_OUTPUT;

	dbg_lock(&dev->vbq_lock, "vbq_lock\n");
	spin_lock_irqsave(&dev->vbq_lock, flags);
	if (dev->active == 1) {
		printk_err("v4l2 device Active\n");
		spin_unlock_irqrestore(&dev->vbq_lock, flags);
		kfree(fh);
		return -EBUSY;
	}
	dev->active = 1;
	/* following definition assumed, to use dma_alloc_coherent */
	dev->dev.coherent_dma_mask  = 0xffffffffll;
	spin_unlock_irqrestore(&dev->vbq_lock, flags);

	videobuf_queue_ape5rbuf_init(
	  &fh->vbq,       /* struct videobuf_queue *q, */
	  &dev->vbq_ops,  /* struct videobuf_queue_ops *ops,*/
	  &dev->dev,      /* struct device *dev */
	  &dev->vbq_lock, /* spinlock_t *irqlock, */
	  fh->type,       /* enum v4l2_buf_type type,*/
	  V4L2_FIELD_NONE,/* enum v4l2_field field, */
	  sizeof(struct ape5r_v4l2_videobuffer),    /* message size */
	  dev);                                      /* private data */

#if RTAPI_MEMORY_INTERFACE
	/* register buffer management I/F */
	videobuf_queue_ape5r_interface_init(dev);
#endif
	return 0;
}


/*****************************************************************************
* MODULE   : ape5r_v4l2_thread_postmessage
* FUNCTION :
* RETURN   : -
* NOTE	   : This function executes post message .
******************************************************************************/
static int ape5r_v4l2_thread_postmessage_wait(struct ape5r_v4l2_device *dev)
{
	struct ape5r_th_object *th = &dev->th_lcd;
	return ((th->wp - th->rp)&7) <= 4;
}

static int ape5r_v4l2_thread_postmessage(struct ape5r_v4l2_device *dev,
  int message, int (*confirm_condition)(struct ape5r_v4l2_device *dev))
{
	struct ape5r_th_object *th = &dev->th_lcd;
	int           nummsg;
	unsigned long flags;
	int           retval = 0;

	/* check queue-full */

	nummsg = (th->wp - th->rp)&7;
	if (confirm_condition != NULL) {
		if (nummsg > 4) {
			dbg_lock(&(th->th_ack).lock, "th_ack_lock\n");
			spin_lock_irqsave(&(th->th_ack).lock, flags);
			th->flag++;         /* set notify flag */
			wait_event_interruptible_locked_irq(            \
				th->th_ack,                             \
				ape5r_v4l2_thread_postmessage_wait(dev));
			th->flag--;         /* clear notify flag */
			spin_unlock_irqrestore(&(th->th_ack).lock, flags);
		}
	} else if (nummsg >= 7) {
		printk_err("post message 0x%x failed.\n", message);
		retval = -EBUSY;
	}

	if (retval == 0) {
		/* post message to thread */
		th->events[th->wp] = message;
		th->wp = (th->wp+1) & 7;
		if (confirm_condition != NULL) {
			dbg_lock(&dev->vbq_lock, "th_ack_lock\n");
			spin_lock_irqsave(&(th->th_ack).lock, flags);
			th->flag++;         /* set notify flag */
		}

		wake_up_interruptible(&th->th_wait);

		if (confirm_condition != NULL) {
			wait_event_interruptible_locked_irq(                \
				th->th_ack, (*confirm_condition)(dev));
			th->flag--;         /* clear notify flag */
			spin_unlock_irqrestore(&(th->th_ack).lock, flags);
		}
	}
	return retval;
}

/*****************************************************************************
* MODULE   : ape5r_v4l2_thread_getmessage
* FUNCTION :
* RETURN   : -
* NOTE	   : This function get event code.
******************************************************************************/
static int ape5r_v4l2_thread_getmessage(struct ape5r_v4l2_device *dev,
	int wait_enable)
{
	struct ape5r_th_object *th_lcd = &dev->th_lcd;
	int event = EVENT_NOP;
	unsigned long flags;

	if (kthread_should_stop()) {
		/* return caller */
		printk_err("getmessage aborted.");
	} else {
		dbg_lock(&(th_lcd->th_wait).lock, "th_wait_lock\n");
		spin_lock_irqsave(&(th_lcd->th_wait).lock, flags);

		if (wait_enable) {
			wait_event_interruptible_locked_irq(th_lcd->th_wait, \
				(th_lcd->rp != th_lcd->wp));
		}

		if (th_lcd->rp != th_lcd->wp) {
			event = th_lcd->events[th_lcd->rp];
			th_lcd->rp = (th_lcd->rp+1) & 7;
		}
		spin_unlock_irqrestore(&(th_lcd->th_wait).lock, flags);
	}
	return event;
}

/*****************************************************************************
* MODULE   : ape5r_v4l2_thread_streamoff
* FUNCTION : thread loop on streaming off.
* RETURN   : none
* NOTE	   :
******************************************************************************/
static inline void ape5r_v4l2_thread_streamoff(
	struct ape5r_v4l2_device *dev, struct ape5r_th_object *th_lcd)
{
	int           event;
	unsigned long flags;
	int           id, format;
	int           wait_enable;

	/* confirm stream off condition. */
	BUG_ON((dev->status & STAT_STREAMINGON) != 0);

	/* waiting STREAM-ON. */
	while (!kthread_should_stop()) {

		wait_enable = 1; /* always enabled. */

		event = ape5r_v4l2_thread_getmessage(dev,
			 wait_enable);
		if (kthread_should_stop()) {
			/* return caller */
			return;
		}

		printk_dbg(_THR_DBG, "process event: %d\n", event);

		if (th_lcd->flag) {
			/* wake up waiting thread. */
			wake_up_all(&dev->th_lcd.th_ack);
		}

		if (event == EVENT_START)
			break;
		else if (event != EVENT_UPDATEQUEUE) {
			/* report error */
			printk_err("event 0x%x ignored.", event);
		}
	}

	/* execute STREAM-ON procedure. */

	if (dev == ape5r_dev) {
		/* select overlay ID 1 */
		id = 1;
	} else {
		/* select overlay ID 2 */
		id = 2;
	}
	format = -1;
	switch (dev->output) {
	case V4L2_OUTPUT_LCD1:
	case V4L2_OUTPUT_LCD2:
		break;
	case V4L2_OUTPUT_HDMI_1080I:
		format = RT_DISPLAY_1920_1080I60;
		break;
	case V4L2_OUTPUT_HDMI_720P:
		format = RT_DISPLAY_1280_720P60;
		break;
	case V4L2_OUTPUT_HDMI_480P:
		format = RT_DISPLAY_720_480P60;
		break;
	case V4L2_OUTPUT_HDMI_1080P:
		format = RT_DISPLAY_1920_1080P24;
		break;
	default:
		printk_err("invalid output.");
	}
	if (format >= 0) {
		if (ape5r_v4l2_display_startHDMI(RESOURCE_V4L2, \
			format, dev->bg_color)) {
			/* start-HDMI failed. */
			printk_err("can not start HDMI.");
			id = 0; /* no-overlay device */
		}
	}

	/* allocate device  */
	if (id > 0) {
#if RTAPI_MEMORY_INTERFACE
		/* create overlay interface. */
		videobuf_queue_ape5r_interface_createoverlay(dev, id, \
			v4l2_callback);
#endif
	}

	dbg_lock(&dev->vbq_lock, "vbq_lock\n");
	spin_lock_irqsave(&dev->vbq_lock, flags);

	if (id == 0) {
		/* current state is HDMI STOP */
		dev->status |= STAT_HDMI_STOP;
	}

	/* current status changed. */
	dev->status = (dev->status & ~(STAT_STREAMINGOFF)) | \
			STAT_STREAMINGON;

	spin_unlock_irqrestore(&dev->vbq_lock, flags);

	if (th_lcd->flag) {
		/* wake up waiting thread. */
		wake_up_all(&dev->th_lcd.th_ack);
	}
}


/*****************************************************************************
* MODULE   : ape5r_v4l2_thread_streamon
* FUNCTION : thread loop on streaming on.
* RETURN   : none
* NOTE	   :
******************************************************************************/
static inline void ape5r_v4l2_thread_streamon(
	struct ape5r_v4l2_device *dev, struct ape5r_th_object *th_lcd)
{
	int           event = 0;
	unsigned long flags;
	int           wait_enable;

restart:
	/* confirm stream on condition. */
	BUG_ON((dev->status & STAT_STREAMINGON) == 0);

	/* waiting STREAM-OFF or buffer. */
	while (!kthread_should_stop()) {

		dbg_lock(&dev->vbq_lock, "vbq_lock\n");
		spin_lock_irqsave(&dev->vbq_lock, flags);
		wait_enable = 1;
		if (dev->vbq_worknum < 2 && !list_empty(&dev->vbq_in)) {
			/* no-need waiting. */
			wait_enable = 0;
		}
		spin_unlock_irqrestore(&dev->vbq_lock, flags);

		event = ape5r_v4l2_thread_getmessage(dev, wait_enable);
		if (kthread_should_stop()) {
			/* return caller */
			return;
		}

		printk_dbg(_THR_DBG, "process event: %d\n", event);

		if (th_lcd->flag) {
			/* wake up waiting thread. */
			wake_up_all(&dev->th_lcd.th_ack);
		}

		if (event == EVENT_STOP)
			break;
		else if (event == EVENT_STOPHDMI)
			break;
		else if (event != EVENT_UPDATEQUEUE &&
			event != EVENT_REQUESTNEXT &&
			event != EVENT_NOP) {
			/* report error */
			printk_err("event 0x%x ignored.", event);
			if (wait_enable)
				continue;
		}

		if (dev->vbq_worknum < 2) {
			struct videobuf_buffer *vb;
			struct ape5r_v4l2_videobuffer *emvb;
#if RTAPI_MEMORY_INTERFACE
			int    rc = 0;
#endif

			dbg_lock(&dev->vbq_lock, "vbq_lock\n");
			spin_lock_irqsave(&dev->vbq_lock, flags);

			if (list_empty(&dev->vbq_in)) {
				/* no buffer available. */
				spin_unlock_irqrestore(&dev->vbq_lock, flags);
				continue;
			}

			vb = list_entry(dev->vbq_in.next,       \
					struct videobuf_buffer, queue);

#if 0	/* for TimeMark display */
		{
			struct timeval current;
			int    delay_sec, delay_usec, msec;
			do_gettimeofday(&current);
			delay_sec  = current.tv_sec  - vb->ts.tv_sec;
			delay_usec = current.tv_usec - vb->ts.tv_usec;
			if (delay_usec < 0) {
				delay_usec += 1000000;
				delay_sec  -= 1;
			}
			msec = delay_sec * 1000 + delay_usec / 1000;

			if (msec < -10) {
				/*delay output.*/
				msleep((-msec/8) * 8);
			} else if (msec > 30) {
				/*skip output.*/
				list_del(&vb->queue);
				INIT_LIST_HEAD(&vb->queue);
				vb->state = VIDEOBUF_ERROR;
				spin_unlock_irqrestore(&dev->vbq_lock, flags);
				wake_up(&vb->done);
				continue;
			}
		}
#endif
			if (dev->status & STAT_HDMI_STOP) {
				/* video buffer state change to error */
				list_del(&vb->queue);
				INIT_LIST_HEAD(&vb->queue);
				vb->state = VIDEOBUF_ERROR;
				wake_up(&vb->done);
			} else {
				/* video buffer state change to active */
				list_del(&vb->queue);
				INIT_LIST_HEAD(&vb->queue);
				vb->state = VIDEOBUF_ACTIVE;
				dev->vbq_worknum++;

				emvb  = (struct ape5r_v4l2_videobuffer *)vb;
				list_add_tail(&emvb->work_queue,        \
						&dev->vbq_work);
			}

			spin_unlock_irqrestore(&dev->vbq_lock, flags);
			if (vb->state == VIDEOBUF_ERROR) {
				/* passed buffer not displayed. */
				continue;
			}
#if RTAPI_MEMORY_INTERFACE
			rc = videobuf_queue_ape5r_interface_postimage( \
					emvb, dev);
			if (rc != 0) {
				spinlock_t   *lock = &dev->vbq_lock;
				printk_err("postimage failed.");

				/* remove buffer from work-queue */

				dbg_lock(lock, "vbq_lock\n");
				spin_lock_irqsave(lock, flags);
				list_del(&emvb->work_queue);
				INIT_LIST_HEAD(&emvb->work_queue);
				vb->state = VIDEOBUF_ERROR;
				if (dev->vbq_worknum > 0) {
					/* decrement work_queue count */
					dev->vbq_worknum--;
				}
				spin_unlock_irqrestore(lock, flags);
				wake_up(&vb->done);
			}

			if (rc == -ERESTARTSYS && dev->vbq_worknum == 0) {
				int id;
				/* re-create overlay */
				printk_err("fatal error, re-create overlay.");

				/* destroy overlay */
				videobuf_queue_ape5r_interface_destroyoverlay(\
					dev);

				/* create overlay */
				if (dev == ape5r_dev) {
					/* select overlay ID 1 */
					id = 1;
				} else {
					/* select overlay ID 2 */
					id = 2;
				}
				videobuf_queue_ape5r_interface_createoverlay(\
					dev, id, v4l2_callback);
			}
#endif
		}
	}

#if 0
	/* wait all work-queue completed.       */
	if (dev->vbq_worknum > 0) {
		printk_dbg(_LOG_DBG, "wait all active buffer complete.\n");
		dbg_lock(&dev->vbq_lock, "vbq_lock\n");
		spin_lock_irqsave(&dev->vbq_lock, flags);
		while (dev->vbq_worknum > 0) {
			struct videobuf_buffer *vb;
			int    local_event;
			spin_unlock_irqrestore(&dev->vbq_lock, flags);

			wait_enable = 1;
			local_event = ape5r_v4l2_thread_getmessage(dev, \
				wait_enable);
			printk_dbg(_THR_DBG, "0x%x ignored.\n", local_event);
			if (local_event == EVENT_STOP) {
				/* set current event as STOP */
				event = EVENT_STOP;
			}

			if (th_lcd->flag) {
				/* wake up waiting thread. */
				wake_up_all(&dev->th_lcd.th_ack);
			}

			/* all buffer will be error. */
			dbg_lock(&dev->vbq_lock, "vbq_lock\n");
			spin_lock_irqsave(&dev->vbq_lock, flags);
			if (list_empty(&dev->vbq_in)) {
				/* no buffer available. */
				continue;
			}

			vb = list_entry(dev->vbq_in.next,       \
					struct videobuf_buffer, queue);

			list_del(&vb->queue);
			INIT_LIST_HEAD(&vb->queue);
			vb->state = VIDEOBUF_ERROR;
			wake_up(&vb->done);
		}
		spin_unlock_irqrestore(&dev->vbq_lock, flags);
	}
#endif

	/* execute STREAM-OFF procedure. */
	if (dev->status & STAT_HDMI_STOP) {
		/* already destroyed. */
		printk_dbg(_THR_DBG, "skip destroy overlay.\n");
	} else {
#if RTAPI_MEMORY_INTERFACE
		/* destroy overlay */
		videobuf_queue_ape5r_interface_destroyoverlay(dev);
#endif
	}

#if 1
	/* flash all work-queue after destroy overlay.       */
	if (dev->vbq_worknum > 0) {
		printk_dbg(_LOG_DBG, "fource release active buffer.\n");
		dbg_lock(&dev->vbq_lock, "vbq_lock\n");

		msleep(200);

		spin_lock_irqsave(&dev->vbq_lock, flags);
		while (dev->vbq_worknum > 0) {
			struct ape5r_v4l2_videobuffer *emvb;

			emvb = list_entry(dev->vbq_work.next,       \
				struct ape5r_v4l2_videobuffer, work_queue);

			list_del(&emvb->work_queue);
			INIT_LIST_HEAD(&emvb->work_queue);
			if (dev->vbq_worknum > 0) {
				/* decrement work_queue count */
				dev->vbq_worknum--;
			}
			do_gettimeofday(&emvb->vb.ts);
			emvb->vb.state = VIDEOBUF_ERROR;
			wake_up(&emvb->vb.done);
		}
		spin_unlock_irqrestore(&dev->vbq_lock, flags);
	}
#endif

	if (event == EVENT_STOPHDMI) {
		dbg_lock(&dev->vbq_lock, "vbq_lock\n");
		spin_lock_irqsave(&dev->vbq_lock, flags);
		/* set HDMI stop flag. */
		dev->status = dev->status | STAT_HDMI_STOP;
		spin_unlock_irqrestore(&dev->vbq_lock, flags);

		/* restart HDMI */

		goto restart;
	}



	switch (dev->output) {
	case V4L2_OUTPUT_LCD1:
	case V4L2_OUTPUT_LCD2:
		break;
	case V4L2_OUTPUT_HDMI_1080I:
	case V4L2_OUTPUT_HDMI_720P:
	case V4L2_OUTPUT_HDMI_480P:
	case V4L2_OUTPUT_HDMI_1080P:
		ape5r_v4l2_display_stopHDMI(RESOURCE_V4L2);
		break;
	default:
		printk_err("invalid output.");
	}

	dbg_lock(&dev->vbq_lock, "vbq_lock\n");
	spin_lock_irqsave(&dev->vbq_lock, flags);

	/* current status changed. */
	dev->status = (dev->status & ~(STAT_HDMI_STOP));
	dev->status = (dev->status & ~(STAT_STREAMINGON));
	dev->status = (dev->status | STAT_STREAMINGOFF);

	spin_unlock_irqrestore(&dev->vbq_lock, flags);

	if (th_lcd->flag) {
		/* wake up waiting thread. */
		wake_up_all(&dev->th_lcd.th_ack);
	}
}

/*****************************************************************************
* MODULE   : ape5r_v4l2_thread
* FUNCTION : start to run request in RTIF.  (TMR thread)
* RETURN   : 0
* NOTE	   :
******************************************************************************/
static inline int ape5r_v4l2_thread(void *arg)
{
	struct ape5r_v4l2_device *dev = (struct ape5r_v4l2_device *)arg;
	struct ape5r_th_object *th_lcd = &dev->th_lcd;

	/* set thread priority */
#if _V4L2_RT_THREAD /* RT thread */
	struct sched_param param = {.sched_priority = V4L2_THREAD_PRIORITY};
	sched_setscheduler(current, SCHED_FIFO, &param);
#else /* Normal thread */
	set_user_nice(current, V4L2_THREAD_NICE);
#endif

	/* dev->th_events already initialized 0. */
	while (!kthread_should_stop()) {
		ape5r_v4l2_thread_streamoff(dev, th_lcd);

		if (kthread_should_stop())
			break;

		ape5r_v4l2_thread_streamon(dev, th_lcd);

		if (kthread_should_stop())
			break;
	}
	return 0;
}


/*****************************************************************************
* MODULE   : ape5r_v4l2_HDMIstate_comflict
* FUNCTION : when cleanup module
* RETURN   :	   0: success
*	   : -ENODEV: corresponding device doesn't exist.
* NOTE	   : v4l2 driver cleanup function, when to driver ends.
******************************************************************************/
static int ape5r_v4l2_HDMIstate_comflict_wait(struct ape5r_v4l2_device *dev)
{
	return ((dev->status & STAT_HDMI_STOP) != 0) ||   \
			((dev->status & STAT_STREAMINGON) == 0);
}
int ape5r_v4l2_HDMIstate_comflict(void)
{
	struct ape5r_v4l2_device *dev;
#if RTAPI_MEMORY_INTERFACE
	unsigned long flags;
#endif
	int           retval = 0;

	/* check device conflict. */
	list_for_each_entry(dev, &devlist, devlist)
	{
#if RTAPI_MEMORY_INTERFACE
		int  ret;

		spin_lock_irqsave(&dev->vbq_lock, flags);
		if (!dev->active ||
			dev->streaming == NULL ||
			dev->mDev.output_mode != RT_OVERLAY_HDMI) {
			/* display target is not HDMI. */
			spin_unlock_irqrestore(&dev->vbq_lock, flags);
			continue;
		}
		spin_unlock_irqrestore(&dev->vbq_lock, flags);

		printk_dbg(_ERR_DBG, "waiting HDMI stop.\n");
		ret = ape5r_v4l2_thread_postmessage(dev, EVENT_STOPHDMI, \
			    ape5r_v4l2_HDMIstate_comflict_wait);
		if (ret != 0) {
			/* report error */
			printk_err("ape5r_v4l2_thread_postmessage fail");
			retval = -1;
		}
#endif
	}
	return retval;
}

/*****************************************************************************
* MODULE   : ape5r_v4l2_core_cleanup
* FUNCTION : cleanup module
* RETURN   :	   0: success
*	   : -ENODEV: corresponding device doesn't exist.
* NOTE	   : v4l2 driver cleanup function, when to driver ends.
******************************************************************************/
void ape5r_v4l2_core_cleanup(void)
{
	struct ape5r_v4l2_device *dev;
	struct video_device	 *vfd;

	/* cleanup kthread resource. */
	list_for_each_entry(dev, &devlist, devlist)
	{
		dev->th_lcd.rp = dev->th_lcd.wp = 0;
		wake_up_interruptible(&dev->th_lcd.th_wait);
		if (dev->th_lcd.th)
			kthread_stop(dev->th_lcd.th);

		kfree(dev->th_lcd.th_name);
	}

	/* cleanup V4L2 resource. */
	list_for_each_entry(dev, &devlist, devlist)
	{
		struct ape5r_v4l2_device *dev = ape5r_dev;

		list_del(&dev->devlist);
		vfd = dev->vfd;
		if (vfd) {
			if (vfd->minor == -1) {
				/* The device never got registered, so
				** release the video_device struct directly
				*/
				video_device_release(vfd);
			} else {
				/* The unregister function will release the
				** video_device struct as well as
				** unregistering it.
				*/
				video_unregister_device(vfd);
			}
			dev->vfd = NULL;
		}
	}

	/* cleanup device resource. */
	platform_driver_unregister(&ape5r_v4l2_core_driver);
	platform_device_unregister(&ape5r_v4l2_core_device);
	UNREGIST_PROCINTERFACE;

	/* cleanup object. */
	list_for_each_entry(dev, &devlist, devlist)
	{
		kfree(dev);
	}
	ape5r_dev  = NULL;

	return;
}

/*****************************************************************************
* MODULE   : ape5r_v4l2_core_reset_defaultparam
* FUNCTION : initialize parameters
* RETURN   :	   0: success
*	   : -ENODEV: corresponding device doesn't exist.
* NOTE	   : v4l2 driver initialize function, when to driver starts.
******************************************************************************/
static void ape5r_v4l2_core_resetdefault(struct ape5r_v4l2_device *dev)
{
	memset(&dev->pix,  0, sizeof(dev->pix));
	memset(&dev->win,  0, sizeof(dev->win));
	memset(&dev->crop, 0, sizeof(dev->crop));
	memset(&dev->workbuf, 0, sizeof(dev->workbuf));
	dev->pix.pixelformat  = V4L2_PIX_FMT_NV12;
	dev->pix.colorspace   = V4L2_COLORSPACE_BT601_FULLSCALE;
	dev->pix.field        = V4L2_FIELD_NONE;
	dev->win.chromakey    = 0x0000;
	dev->win.global_alpha = 0xFF;

	dev->output      = V4L2_OUTPUT_LCD1;
	dev->rotate      = 0;
	dev->mirror      = V4L2_MIRROR_NONE;
	dev->check_param = confirm_device_parameters;
	dev->bg_color    = 0x000000; /* for HDMI */
	dev->streaming   = NULL;
	dev->status      = 0;
}

/*****************************************************************************
* MODULE   : ape5r_v4l2_core_init
* FUNCTION : initialize module
* RETURN   :	   0: success
*	   : -ENODEV: corresponding device doesn't exist.
* NOTE	   : v4l2 driver initialize function, when to driver starts.
******************************************************************************/
static struct ape5r_v4l2_device *__ape5r_v4l2_allocate_device(void)
{
	struct ape5r_v4l2_device *dev;
	struct video_device	 *vfd;

	dev = kmalloc(sizeof(struct ape5r_v4l2_device), GFP_KERNEL);
	if (!dev) {
		printk_err("could not allocate memory\n");
		goto init_error;
	}
	memset(dev, 0, sizeof(struct ape5r_v4l2_device));
	INIT_LIST_HEAD(&dev->devlist);


	/* initialize the video_device struct */
	vfd = dev->vfd = video_device_alloc();
	if (!vfd) {
		printk_err("could not allocate video device struct\n");
		goto init_error;
	}
	vfd->release = video_device_release;
	strlcpy(vfd->name, DEV_NAME, sizeof(vfd->name));
	vfd->minor    = -1;
	vfd->fops     = &ape5r_v4l2_core_fops;
	video_set_drvdata(vfd, dev);

	/* initialize the semafore */
	sema_init(&dev->sem_lock, 1);

	/* initialize the spinlock used to image parameters */
	spin_lock_init(&dev->vbq_lock);

	/* initialize the videobuf queue ops */
	dev->vbq_ops.buf_setup	 = ape5r_v4l2_core_vbq_setup;
	dev->vbq_ops.buf_prepare = ape5r_v4l2_core_vbq_prepare;
	dev->vbq_ops.buf_queue	 = ape5r_v4l2_core_vbq_queue;
	dev->vbq_ops.buf_release = ape5r_v4l2_core_vbq_release;

	if (video_register_device(vfd, VFL_TYPE_GRABBER, video_nr) < 0) {
		printk_err("could not register Video for Linux device\n");
		goto init_error;
	}

	/* initialize queue. */
	INIT_LIST_HEAD(&dev->vbq_in);
	INIT_LIST_HEAD(&dev->vbq_work);
	dev->vbq_worknum = 0;

	/* initialize thread. */
	{
		struct ape5r_th_object *th = &dev->th_lcd;

		init_waitqueue_head(&th->th_wait);
		init_waitqueue_head(&th->th_ack);
		th->th_name = vfd->name;
		th->rp      = 0;
		th->wp      = 0;
		th->flag    = 0;

		th->th  = kthread_run(ape5r_v4l2_thread,
				     dev,
				     th->th_name);
		if (IS_ERR(th->th)) {
			printk_err("could not create kernel thread\n");
			goto init_error;
		}
	}

	/* set default parameter */
	ape5r_v4l2_core_resetdefault(dev);

	/* Now that everything is fine, let's add it to device list */
	list_add_tail(&dev->devlist, &devlist);

	printk(KERN_INFO DEV_NAME ": registered device video%d [v4l2]\n",
	 (video_nr == -1) ? vfd->minor : video_nr);
	return dev;

init_error:
	if (dev) {
		if (dev->vfd) {
			if (dev->vfd->minor == -1) {
				/* The device never got registered,
				** so release the video_device struct directly
				*/
				video_device_release(dev->vfd);
			} else {
				/* The unregister function will release the
				** video_device struct as well as
				** unregistering it.
				*/
				video_unregister_device(dev->vfd);
			}
		}
		kfree(dev);
	}
	return NULL;
}
/*****************************************************************************
* MODULE   : ape5r_v4l2_core_init
* FUNCTION : initialize module
* RETURN   :	   0: success
*	   : -ENODEV: corresponding device doesn't exist.
* NOTE	   : v4l2 driver initialize function, when to driver starts.
******************************************************************************/
int __init ape5r_v4l2_core_init(void)
{
	/* allocate device */
	ape5r_dev = __ape5r_v4l2_allocate_device();
	if (!ape5r_dev) {
		printk_err("error in __ape5r_v4l2_allocate_device\n");
		goto init_error;
	}

	/* register device list header */
	dev_set_drvdata(&ape5r_v4l2_core_device.dev, (void *)&devlist);

	if (platform_device_register(&ape5r_v4l2_core_device) < 0) {
		printk_err("could not register platform_device\n");
		goto init_error;
	}
	if (platform_driver_register(&ape5r_v4l2_core_driver) < 0) {
		printk_err("could not register driver\n");
		platform_device_unregister(&ape5r_v4l2_core_device);
		goto init_error;
	}
	REGIST_PROCINTERFACE;
	return 0;

init_error:
	ape5r_v4l2_core_cleanup();
	return -ENODEV;
}




MODULE_AUTHOR("Renesas Electronics");
MODULE_DESCRIPTION("APE5R V4L2 overlay");
MODULE_LICENSE("GPL");
module_param(video_nr, int, 0);
MODULE_PARM_DESC(video_nr,
 "Minor number for video device (-1 ==> auto assign)");

module_init(ape5r_v4l2_core_init);
module_exit(ape5r_v4l2_core_cleanup);


