/*
 * helper functions for physically contiguous capture buffers
 *
 * The functions support hardware lacking scatter gather support
 * (i.e. the buffers must be linear in physical memory)
 *
 * Copyright (c) 2008 Magnus Damm
 *
 * Based on videobuf-vmalloc.c,
 * (c) 2007 Mauro Carvalho Chehab, <mchehab@infradead.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2
 */

#include <linux/videodev2.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/mm.h>
#include <linux/pagemap.h>
#include <linux/dma-mapping.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <media/videobuf-dma-contig.h>
#include <linux/delay.h>

#define _MEM_DBG   0 /* debug memory */         /* 0: disable  1: enable */
#define _OVL_DBG   0 /* debug overlay */        /* 0: disable  1: enable */
#define _EN_RGB565 0

#include "ape5r_overlay.h"
#include <video/sh_mobile_lcdc.h>
#include "ape5r_entry.h"

#undef  DEV_NAME
#define DEV_NAME "v4l2buf-mfi"
#if RTAPI_MEMORY_INTERFACE
#include <rtapi/system_memory.h>
#include <rtapi/screen_display.h>
#include <rtapi/screen_overlay.h>
#endif


#define printk_err(fmt, arg...) \
	do { \
		printk(KERN_ERR DEV_NAME ": %s: " fmt, __func__, ## arg); \
	} while (0)

#define printk_dbg(level, fmt, arg...) \
	do { \
		if (level > 0) \
			printk(KERN_INFO DEV_NAME ": %s: " fmt, \
				__func__, ## arg); \
	} while (0)

struct videobuf_mfi_memory {
	u32 magic;
	void *vaddr;
	dma_addr_t dma_handle;
	unsigned long size;
	int is_userptr;
#if RTAPI_MEMORY_INTERFACE
	unsigned long offset;
	void     *handle;
	void     *apmem_handle;
#endif
};

#define MAGIC_DC_MEM 0x0733ac61
#define MAGIC_CHECK(is, mag)						    \
	if (unlikely((is) != (mag)))	{				    \
		printk_err("magic mismatch: %x expected %x\n", (is), (mag));\
		BUG();							    \
	}




#if RTAPI_MEMORY_INTERFACE

static wait_queue_head_t    wait_id[2];
static int                  ovl_state[2];
static int                  result_code[2];

#define OVL_STATE_READY    0x0001
#define OVL_STATE_ERROR    0x0100
#define OVL_STATE_INIT     0x0200

static void function_notify_overlay(
	int result, screen_ovl_notify_return_data *notify_data, int data_size)
{
	printk_dbg(_OVL_DBG,     \
		"function_notify_overlay:%d\n", notify_data->command_type);
	switch (notify_data->command_type) {
	case RT_OVERLAY_NOTIFY_RETURN:
		if (notify_data->overlay_id == RT_OVERLAY_ID1) {
			result_code[0] = result;
			ovl_state[0]  |= OVL_STATE_READY;
			wake_up_interruptible(&wait_id[0]);
		} else if (notify_data->overlay_id == RT_OVERLAY_ID2) {
			result_code[1] = result;
			ovl_state[1]  |= OVL_STATE_READY;
			wake_up_interruptible(&wait_id[1]);
		}
		break;
	case RT_OVERLAY_NOTIFY_RETURN_BUFFER:
		{
			struct ape5r_v4l2_videobuffer *emvb = \
				(struct ape5r_v4l2_videobuffer *) \
				notify_data->return_buffer.user_data;
			struct videobuf_queue *vbq     = emvb->vbq;
			struct ape5r_v4l2_device *dev = vbq->priv_data;

			if (dev->mDev.callback) {
				/* generate callback */
				dev->mDev.callback(emvb);
			}
		}
		break;
	case RT_OVERLAY_NOTIFY_ERROR_INFO:
		if (notify_data->overlay_id == RT_OVERLAY_ID1) {
			result_code[0] = result;
			ovl_state[0]  |= OVL_STATE_ERROR;
			wake_up_interruptible(&wait_id[0]);
		} else if (notify_data->overlay_id == RT_OVERLAY_ID2) {
			result_code[1] = result;
			ovl_state[1]  |= OVL_STATE_ERROR;
			wake_up_interruptible(&wait_id[1]);
		}
		break;
	}
}
#endif


#if RTAPI_MEMORY_INTERFACE
static int check_interface(void)
{
	return !ape5r_check_rtapi_interface();
};
#endif
static int check_available_usermem(struct ape5r_v4l2_device *dev)
{
	if (dev == NULL) {
		printk_err("invalid argument.\n");
		return 0;
	}
#if RTAPI_MEMORY_INTERFACE
	if (dev->mMem.handle == NULL) {
		printk_err("Memory not available.\n");
		return 0;
	}
#endif
	if (dev->workbuf.apmem_addr && dev->workbuf.apmem_id) {
		/* memory useable */
		return 1;
	}
	printk_err("private buffer not registered.\n");
	return 0;
};
#if RTAPI_MEMORY_INTERFACE
static int check_available_overlay(struct ape5r_v4l2_device *dev)
{
	if (!check_available_usermem(dev))
		return 0;
	if (dev->mDev.handle_overlay == NULL) {
		printk_err("Overlay not available.\n");
		return 0;
	}
	/* overlay useable */
	return 1;
};
#endif

/**
 * videobuf_mfi_user_put() - reset pointer to user space buffer
 * @mem: per-buffer private videobuf-dma-contig data
 *
 * This function resets the user space pointer
 */
static void videobuf_mfi_user_put(struct ape5r_v4l2_device *dev,
    struct videobuf_mfi_memory *mem)
{
#if RTAPI_MEMORY_INTERFACE
	if (mem->is_userptr) {
		unsigned long flags;
		spin_lock_irqsave(&dev->vbq_lock, flags);
		dev->mMem.mRefCount--;
		spin_unlock_irqrestore(&dev->vbq_lock, flags);
		printk_dbg(_MEM_DBG,                                    \
			"memory reference counts:%d\n", dev->mMem.mRefCount);
		mem->vaddr = NULL;
	}
	mem->offset       = 0;
	mem->handle       = NULL;
	mem->apmem_handle = NULL;
#endif
	mem->is_userptr = 0;
	mem->dma_handle = 0;
	mem->size = 0;
}

/**
 * videobuf_mfi_user_get() - setup user space memory pointer
 * @mem: per-buffer private videobuf-dma-contig data
 * @vb: video buffer to map
 *
 * This function validates and sets up a pointer to user space memory.
 * Only physically contiguous pfn-mapped memory is accepted.
 *
 * Returns 0 if successful.
 */
static int videobuf_mfi_user_get(struct ape5r_v4l2_device *dev,
	struct videobuf_mfi_memory *mem, struct videobuf_buffer *vb)
{
	int ret = -EINVAL;
#if RTAPI_MEMORY_INTERFACE
	unsigned long user_address = vb->baddr;
	unsigned long offset;

	/* calculate offset */
	if (user_address < dev->workbuf.apmem_addr) {
		printk_err("can not access user memory.\n");
		goto error;
	}
	offset    = user_address - dev->workbuf.apmem_addr;

	/* confirm valid range (allocatable?) */
	{
		system_mem_ap_share_mem  smem;
		unsigned long endoffset = offset + vb->bsize - 1;

		printk_dbg(_MEM_DBG,                                     \
			"handle:%p mem handle:%p offset:%lx\n",              \
			dev->mMem.handle, dev->mMem.apmem_handle, endoffset);

		smem.handle       = dev->mMem.handle;
		smem.apmem_handle = dev->mMem.apmem_handle;
		smem.apmem_offset = endoffset;
		smem.apmem_apaddr = 0;
		if (ape5r_v4l2_get_rtapi_interface()   \
			->system_memory_ap_share_mem(&smem) != 0) {
			printk_err("end offset not accessible.\n");
			goto error;
		}
	}

	/* mapping shared memory */
	{
		unsigned long flags;
		system_mem_ap_share_mem  smem;

		printk_dbg(_MEM_DBG,                                     \
			"handle:%p mem handle:%p offset:%lx\n",              \
			dev->mMem.handle, dev->mMem.apmem_handle, offset);

		smem.handle       = dev->mMem.handle;
		smem.apmem_handle = dev->mMem.apmem_handle;
		smem.apmem_offset = offset;
		smem.apmem_apaddr = 0;
		if (ape5r_v4l2_get_rtapi_interface()   \
			->system_memory_ap_share_mem(&smem) != 0) {
			printk_err("can not access user memory.\n");
			goto error;
		}

		printk_dbg(_MEM_DBG, "memory allocation success:%x\n", \
			smem.apmem_apaddr);
		ret = 0;
		mem->size         =  vb->bsize;
		mem->dma_handle   =  0;
		mem->is_userptr   =  1;
		mem->offset       =  offset;
		mem->handle       =  dev->mMem.handle;
		mem->apmem_handle =  dev->mMem.apmem_handle;
		mem->vaddr        =  (void *)smem.apmem_apaddr;

		printk_dbg(_MEM_DBG, "i:%d   adr:0x%lx  offset:0x%lx\n", \
			vb->i, user_address, offset);

		spin_lock_irqsave(&dev->vbq_lock, flags);
		dev->mMem.mRefCount++;
		spin_unlock_irqrestore(&dev->vbq_lock, flags);
		printk_dbg(_MEM_DBG, "memory reference counts:%d\n",    \
		    dev->mMem.mRefCount);
	}
error:
#endif
	return ret;
}

static struct videobuf_buffer *__videobuf_alloc(size_t size)
{
	struct videobuf_mfi_memory *mem;
	struct videobuf_buffer *vb;

	vb = kzalloc(size + sizeof(*mem), GFP_KERNEL);
	if (vb) {
		mem = vb->priv = ((char *)vb) + size;
		mem->magic = MAGIC_DC_MEM;
	}

	return vb;
}

static void *__videobuf_to_vaddr(struct videobuf_buffer *buf)
{
	struct videobuf_mfi_memory *mem = buf->priv;

	BUG_ON(!mem);
	MAGIC_CHECK(mem->magic, MAGIC_DC_MEM);
	return mem->vaddr;
}

static int __videobuf_iolock(struct videobuf_queue *q,
			     struct videobuf_buffer *vb,
			     struct v4l2_framebuffer *fbuf)
{
	struct videobuf_mfi_memory *mem = vb->priv;

	BUG_ON(!mem);
	MAGIC_CHECK(mem->magic, MAGIC_DC_MEM);

	switch (vb->memory) {
	case V4L2_MEMORY_MMAP:
		printk_dbg(_MEM_DBG, "%s memory method MMAP\n", __func__);
		return -EINVAL;
		break;
	case V4L2_MEMORY_USERPTR:
		printk_dbg(_MEM_DBG, "%s memory method USERPTR\n", __func__);

		/* handle pointer from user space */
		if (vb->baddr) {
			if (check_available_usermem(q->priv_data)) {
				return videobuf_mfi_user_get(           \
					q->priv_data, mem, vb);
			} else {
				printk_err("user memory is not useable.\n");
				return -EINVAL;
			}
		}

		/* not support */
		printk_err("0 not supported.\n");
		return -ENOMEM;

	case V4L2_MEMORY_OVERLAY:
	default:
		printk_dbg(_MEM_DBG, "%s memory method OVERLAY/unknown\n",\
			__func__);
		return -EINVAL;
	}

	return 0;
}

static int __videobuf_mmap_mapper(struct videobuf_queue *q,
				  struct videobuf_buffer *buf,
				  struct vm_area_struct *vma)
{
	return -ENOMEM;
}

static struct videobuf_qtype_ops qops = {
	.magic        = MAGIC_QTYPE_OPS,

	.alloc        = __videobuf_alloc,
	.iolock       = __videobuf_iolock,
	.mmap_mapper  = __videobuf_mmap_mapper,
	.vaddr        = __videobuf_to_vaddr,
};

/********************/
/* Memory Interface */
/********************/
void videobuf_queue_ape5r_interface_init(struct ape5r_v4l2_device *dev)
{
/*allocate interface*/
#if RTAPI_MEMORY_INTERFACE
	if (check_interface()) {
		printk_dbg(_MEM_DBG, "create memory interface\n");
		if (dev->mMem.handle == NULL) {
			/* allocate mfi interface */
			dev->mMem.handle   = \
				ape5r_v4l2_get_rtapi_interface()   \
				->system_memory_info_new();
		}
		dev->mMem.mRefCount = 0;

		dev->mDev.handle_overlay = NULL;
		dev->mDev.overlay_id  = 0;
		dev->mDev.output_mode = 0;
	}
#endif
}

void videobuf_queue_ape5r_interface_release(struct ape5r_v4l2_device *dev)
{
/*release interface*/
#if RTAPI_MEMORY_INTERFACE
	if (check_interface()) {
		if (dev->mDev.handle_overlay) {
			screen_ovl_delete del;
			printk_dbg(_OVL_DBG, "free display\n");
			del.handle = dev->mDev.handle_overlay;
			ape5r_v4l2_get_rtapi_interface()   \
			->screen_overlay_delete(&del);
			dev->mDev.handle_overlay = NULL;

		}
		if (dev->mMem.handle && dev->mMem.apmem_handle) {
			system_mem_ap_close area;
			printk_dbg(_MEM_DBG, "free memory\n");

			area.handle       = dev->mMem.handle;
			area.apmem_handle = dev->mMem.apmem_handle;

			if (ape5r_v4l2_get_rtapi_interface()   \
				->system_memory_ap_close(&area) != 0) {
				/* output error message */
				printk_err("can not free memory\n");
			}
			dev->mMem.apmem_handle = NULL;
		}
		if (dev->mMem.handle) {
			system_mem_info_delete  del;
			printk_dbg(_MEM_DBG, "delete memory interface\n");
			del.handle = dev->mMem.handle;
			ape5r_v4l2_get_rtapi_interface()   \
			->system_memory_info_delete(&del);
			dev->mMem.handle = NULL;
		}
	}
#endif
}

int  videobuf_queue_ape5r_interface_config(struct ape5r_v4l2_device *dev)
{
#if !RTAPI_MEMORY_INTERFACE
	return -EINVAL;
#else
	unsigned long flags;
	void *handle, *mMemory;
	int  Count;

	if (!check_interface() || dev->mMem.handle == NULL) {
		printk_err("Memory not available.");
		return -EINVAL;
	}

	spin_lock_irqsave(&dev->vbq_lock, flags);
	handle  = dev->mMem.handle;
	mMemory = dev->mMem.apmem_handle;
	Count =   dev->mMem.mRefCount;
	spin_unlock_irqrestore(&dev->vbq_lock, flags);

	/* check now using */
	if (Count) {
		printk_dbg(_MEM_DBG, "memory %p now using count:%d\n",\
			mMemory, Count);
		return -EBUSY;
	}
	if (handle) {
		/* free memory */
		if (mMemory) {
			system_mem_ap_close area;
			printk_dbg(_MEM_DBG, "free memory\n");

			area.handle       = handle;
			area.apmem_handle = mMemory;

			if (ape5r_v4l2_get_rtapi_interface()   \
				->system_memory_ap_close(&area) != 0) {
				/* output error message */
				printk_err("can not free memory\n");
			}
			mMemory = NULL;
			dev->mMem.apmem_handle = NULL;
		}
	}

	if (handle && dev->workbuf.apmem_addr) {
		/* next allocate memory */
		system_mem_ap_share_area area;
		printk_dbg(_MEM_DBG, "handle:%p id:%d\n",          \
			handle, dev->workbuf.apmem_id);

		area.handle       = handle;
		area.apmem_id     = dev->workbuf.apmem_id;
		area.apmem_handle = NULL;

		if (ape5r_v4l2_get_rtapi_interface()   \
			->system_memory_ap_share_area(&area) != 0) {
			printk_err("can not allocate memory\n");
			return -EINVAL;
		} else {
			dev->mMem.apmem_handle = area.apmem_handle;
		}
	}
	return 0;
#endif
}

#if RTAPI_MEMORY_INTERFACE
static int create_ovl_parameter(screen_ovl_set_param *param, \
	struct v4l2_crop        *crop,                        \
	struct v4l2_window      *win,                         \
	int                     rotate,                       \
	int                     mirror)
{
	param->src_crop.x  = crop->c.left;
	param->src_crop.y  = crop->c.top;
	param->src_crop.width  = crop->c.width;
	param->src_crop.height = crop->c.height;
	switch (rotate) {
	default:
		printk_err("invalid rotate parameter.");
		return -EINVAL;
	case 0:
	case 2:
		param->dst_width   = win->w.width;
		param->dst_height  = win->w.height;
		break;
	case 1:
	case 3:
		param->dst_width   = win->w.height;
		param->dst_height  = win->w.width;
		break;
	}
	switch (rotate) {
	default:
		printk_err("invalid rotate parameter.");
		return -EINVAL;
	case 0:
		param->rotate  = RT_OVERLAY_ROTATE_0;
		break;
	case 1:
		param->rotate  = RT_OVERLAY_ROTATE_90;
		break;
	case 2:
		param->rotate  = RT_OVERLAY_ROTATE_180;
		break;
	case 3:
		param->rotate  = RT_OVERLAY_ROTATE_270;
		break;
	}
	switch (mirror) {
	default:
		printk_err("invalid mirror parameter.");
		return -EINVAL;
	case V4L2_MIRROR_NONE:
		param->mirror  = RT_OVERLAY_MIRROR_N;  /* no mirror */
		break;
	case V4L2_MIRROR_HFLIP:
		param->mirror  = RT_OVERLAY_MIRROR_H;  /* horizontal mirror */
		break;
	case V4L2_MIRROR_VFLIP:
		param->mirror  = RT_OVERLAY_MIRROR_V;  /* vertical mirror */
		break;
	}
	param->blend_pos_x     = win->w.left;
	param->blend_pos_y     = win->w.top;
	return 0;
}
#endif

int  videobuf_queue_ape5r_interface_createoverlay(              \
	struct ape5r_v4l2_device *dev, int type, void (*callback)(void *))
{
#if !RTAPI_MEMORY_INTERFACE
	return -EINVAL;
#else
	void *handle = NULL;
	int  ret = 0;
	int  output_mode = -1, overlay_id, index = 0;

	if (dev->mDev.handle_overlay != NULL) {
		/* already create overlay */
		return -EBUSY;
	}
	if (!check_interface()) {
		printk_err("can not handle overlay.\n");
		return -EINVAL;
	}

	switch (type) {
	case 1:
		overlay_id = RT_OVERLAY_ID1;
		index      = 0;
		break;
	case 2:
		overlay_id = RT_OVERLAY_ID2;
		index      = 1;
		break;
	default:
		printk_err("type type.\n");
		ret = -EINVAL;
		goto err_exit;
	}

	if ((ovl_state[index] & OVL_STATE_INIT) == 0) {
		init_waitqueue_head(&wait_id[index]);
		ovl_state[index]   = OVL_STATE_INIT;
		result_code[index] = 0;
	}

	switch (dev->output) {
	case V4L2_OUTPUT_LCD1:
		output_mode = RT_DISPLAY_LCD1;
		break;
	case V4L2_OUTPUT_LCD2:
		output_mode = RT_DISPLAY_LCD2;
		break;
	case V4L2_OUTPUT_HDMI_1080I:
	case V4L2_OUTPUT_HDMI_720P:
	case V4L2_OUTPUT_HDMI_480P:
	case V4L2_OUTPUT_HDMI_1080P:
		output_mode = RT_DISPLAY_HDMI;
		break;
	default:
		printk_err("invalid output.");
		ret = -EINVAL;
		goto err_exit;
	}

	/* turn OFF LCD refresh mode. */
	if ((output_mode == RT_DISPLAY_LCD1 ||
		output_mode == RT_DISPLAY_LCD2)) {
		if (sh_mobile_lcdc_refresh(RT_DISPLAY_REFRESH_OFF,  \
			output_mode)) {
			printk_err("can not change refresh mode.");
			ret = -EINVAL;
			goto err_exit;
		}
	}

	/* create Overlay */
	{
		screen_ovl_new ovl;
		ovl.notify_overlay = function_notify_overlay;
		handle = ape5r_v4l2_get_rtapi_interface()   \
			->screen_overlay_new(&ovl);
		if (handle == NULL) {
			printk_err("create_handle error.\n");
			ret = -EINVAL;
			goto err_exit;
		}
	}

	/* initialize overlay */
	{
		screen_ovl_initialize  ini;
		int                    rc;

		ini.handle       = handle;
		ini.overlay_id   = overlay_id;
		ini.output_mode  = output_mode;
		ini.image_width  = dev->pix.width;
		ini.image_height = dev->pix.height;
		switch (dev->pix.pixelformat) {
		case V4L2_PIX_FMT_YUV420:
			ini.image_format = RT_OVERLAY_COLOR_YUV420_PLANAR;
			ini.image_stride = ini.image_width;
			break;
		case V4L2_PIX_FMT_NV12:
			ini.image_format = RT_OVERLAY_COLOR_YUV420_SEMIPLANAR;
			ini.image_stride = ini.image_width;
			break;
		case V4L2_PIX_FMT_NV16:
			ini.image_format = RT_OVERLAY_COLOR_YUV422_SEMIPLANAR;
			ini.image_stride = ini.image_width;
			break;
		case V4L2_PIX_FMT_UYVY:
			ini.image_format = RT_OVERLAY_COLOR_YUV422_PACKED;
			ini.image_stride = ini.image_width;
			break;
#if _EN_RGB565
		case V4L2_PIX_FMT_RGB565:
			ini.image_format = -1;
			ini.image_stride = ini.image_width*2;
			break;
#endif
		default:
			printk_err("invalid pixelformat.");
			ret = -EINVAL;
			goto err_exit;
		}
		if (dev->pix.bytesperline) {
			/* user specified stride. */
			ini.image_stride = dev->pix.bytesperline;
		}
		switch (dev->pix.colorspace) {
		case V4L2_COLORSPACE_BT709_COMPRESS:
			ini.yuv_format  = RT_OVERLAY_COLOR_BT709;
			ini.yuv_range   = RT_OVERLAY_COLOR_COMPRESSED;
			break;
		case V4L2_COLORSPACE_BT709_FULLSCALE:
			ini.yuv_format  = RT_OVERLAY_COLOR_BT709;
			ini.yuv_range   = RT_OVERLAY_COLOR_FULLSCALL;
			break;
		case V4L2_COLORSPACE_BT601_COMPRESS:
			ini.yuv_format  = RT_OVERLAY_COLOR_BT601;
			ini.yuv_range   = RT_OVERLAY_COLOR_COMPRESSED;
			break;
		case V4L2_COLORSPACE_BT601_FULLSCALE:
			ini.yuv_format  = RT_OVERLAY_COLOR_BT601;
			ini.yuv_range   = RT_OVERLAY_COLOR_FULLSCALL;
			break;
		default:
			printk_err("invalid colorspace.");
			ret = -EINVAL;
			goto err_exit;
		}
		printk_dbg(_OVL_DBG, \
		"init handle: %p " \
		"id:%d mode:%d " \
		"width:%d height:%d " \
		"stride:%d format:%d " \
		"yuvfmt:%d yuvrange:%d " \
		"\n",
			ini.handle,
			ini.overlay_id,  ini.output_mode,
			ini.image_width, ini.image_height,
			ini.image_stride, ini.image_format,
			ini.yuv_format,   ini.yuv_range);
		rc = ape5r_v4l2_get_rtapi_interface()   \
			->screen_overlay_initialize(&ini);
		printk_dbg(_OVL_DBG, \
			"screen_overlay_initialize result:%d\n", rc);
		if (rc) {
			printk_err("screen_overlay_initialize failed.");
			ret = -EINVAL;
			goto err_exit;
		}
	}
	/* set parameter */
	{
		screen_ovl_set_param  param;
		int           retval;

		param.handle      = handle;
		param.overlay_id  = overlay_id;
		retval = create_ovl_parameter(&param, &dev->crop, &dev->win, \
			dev->rotate, dev->mirror);
		if (retval) {
			ret = retval;
			goto err_exit2;
		}

		printk_dbg(_OVL_DBG, \
		"init handle: %p id:%d " \
		"crop(%d %d %d %d) " \
		"dst:%d,%d " \
		"rotate:%d mirror %d " \
		"pos:%d,%d "  \
		"\n",
			param.handle,      param.overlay_id,
			param.src_crop.x,  param.src_crop.y,
			param.src_crop.width,  param.src_crop.height,
			param.dst_width,       param.dst_height,
			param.rotate,      param.mirror,
			param.blend_pos_x, param.blend_pos_y);
		retval = ape5r_v4l2_get_rtapi_interface()   \
			->screen_overlay_set_parameters(&param);
		printk_dbg(_OVL_DBG, \
			"screen_overlay_set_parameters result:%d\n", retval);
		if (retval == 0) {
			dev->mDev.prev_param = param;
		} else {
			printk_err("screen_overlay_set_parameters failed.");
			ret = -EINVAL;
			goto err_exit2;
		}
	}
	/* set FB parameter */
	if (output_mode == RT_DISPLAY_LCD1 || \
	    output_mode == RT_DISPLAY_LCD2) {
		printk_dbg(_OVL_DBG, "set_chromakey");
		dev->mDev.prev_chromakey = -1;
		dev->mDev.prev_globalalpha = -1;
		if (sh_mobile_lcdc_keyclr_set(dev->win.chromakey, \
				output_mode) == 0) {
			/* record current config */
			dev->mDev.prev_chromakey = dev->win.chromakey;
		}

		printk_dbg(_OVL_DBG, "set_global_alpha");
		if (sh_mobile_lcdc_alpha_set(255-dev->win.global_alpha, \
			output_mode) == 0) {
			/* record current config */
			dev->mDev.prev_globalalpha = dev->win.global_alpha;
		}
	}
	/* start notify */
	{
		screen_ovl_notify ovl;
		unsigned int  _mask;
		int           retval;

		/*initialize state */
		_mask = OVL_STATE_READY | OVL_STATE_ERROR;
		ovl_state[index]  &= ~_mask;
		result_code[index] = SMAP_LIB_DISPLAY_NG;

		ovl.handle     = handle;
		ovl.overlay_id = overlay_id;
		retval = ape5r_v4l2_get_rtapi_interface()   \
			->screen_overlay_start_notify(&ovl);
		if (retval) {
			printk_err("screen_overlay_start_notify failed.");
			ret = -EINVAL;
			goto err_exit2;
		}

		printk_dbg(_OVL_DBG, "wait start notify\n");
		retval = wait_event_interruptible_timeout(wait_id[index],     \
				((ovl_state[index] & _mask) != 0),  \
				(2 * HZ));
		printk_dbg(_OVL_DBG, "wait start result: 0x%x state: 0x%x\n", \
				result_code[index], ovl_state[index]);
		if ((ovl_state[index] & OVL_STATE_READY) == 0) {
			printk_err("screen_overlay_start_notify failed.");
			ret = -EINVAL;
			goto err_exit2;
		}
	}
	dev->mDev.handle_overlay = handle;
	dev->mDev.overlay_id     = overlay_id;
	dev->mDev.output_mode    = output_mode;
	dev->mDev.callback       = callback;
	dev->mDev.state          = 1;
	return 0;

err_exit2:
	{
		screen_ovl_quit quit;
		quit.handle     = handle;
		quit.overlay_id = overlay_id;
		ape5r_v4l2_get_rtapi_interface()   \
			->screen_overlay_quit(&quit);
	}

err_exit:
	if (handle) {
		screen_ovl_delete del;
		del.handle = handle;
		ape5r_v4l2_get_rtapi_interface()   \
			->screen_overlay_delete(&del);
	}
	/* turn ON LCD refresh mode. */
	if ((output_mode == RT_DISPLAY_LCD1 ||
		output_mode == RT_DISPLAY_LCD2)) {
		/* turn on refresh mode. */
		sh_mobile_lcdc_refresh(RT_DISPLAY_REFRESH_ON, output_mode);
	}

	return -1;
#endif
}

int  videobuf_queue_ape5r_interface_destroyoverlay(             \
	struct ape5r_v4l2_device *dev)
{
#if !RTAPI_MEMORY_INTERFACE
	return -EINVAL;
#else
	screen_ovl_quit quit;
	int             ret = 0;

	if (dev->mDev.handle_overlay == NULL) {
		/* overlay not created. */
		return -EBUSY;
	}
	if (!check_interface()) {
		printk_err("can not handle overlay.\n");
		return -EINVAL;
	}
	quit.handle     = dev->mDev.handle_overlay;
	quit.overlay_id = dev->mDev.overlay_id;
	if (ape5r_v4l2_get_rtapi_interface()   \
		->screen_overlay_quit(&quit)) {
		printk_err("destroy overlay failed.\n");
		ret = -1;
	} else {
		/* turn ON LCD refresh mode. */
		if ((dev->mDev.output_mode == RT_DISPLAY_LCD1 ||
			dev->mDev.output_mode == RT_DISPLAY_LCD2)) {
			if (sh_mobile_lcdc_refresh(RT_DISPLAY_REFRESH_ON, \
				dev->mDev.output_mode)) {
				printk_err("can not change refresh mode.");
				/* continue process to initialize variables. */
			}
		}

		if (dev->mDev.overlay_id == RT_OVERLAY_ID1) {
			/* reset state */
			ovl_state[0]  = 0;
		} else {
			/* reset state */
			ovl_state[1]  = 0;
		}
		dev->mDev.handle_overlay = NULL;
		dev->mDev.overlay_id     = 0;
		dev->mDev.output_mode    = 0;
		dev->mDev.callback       = 0;
		dev->mDev.state          = 0;
	}
	return 0;
#endif
}

int  videobuf_queue_ape5r_interface_postimage(
	struct ape5r_v4l2_videobuffer *buf,
	struct ape5r_v4l2_device *dev)
{
	int ret = 0;
#if RTAPI_MEMORY_INTERFACE
	screen_ovl_write_image  write;
	screen_ovl_set_param    param;
	struct videobuf_mfi_memory *mem;
	int                     index;

	if (!check_available_overlay(dev)) {
		printk_err("can not handle overlay.\n");
		return -EINVAL;
	}

	if (dev->mDev.overlay_id == RT_OVERLAY_ID1) {
		/* overlay ID1 */
		index = 0;
	} else if (dev->mDev.overlay_id == RT_OVERLAY_ID2) {
		/* overlay ID2 */
		index = 1;
	} else {
		printk_err("unknown overlay id.\n");
		return -EINVAL;
	}
	if (ovl_state[index] & OVL_STATE_ERROR) {
		printk_err("fatal error.\n");
		return -ERESTARTSYS;
	}
	if (!(ovl_state[index] & OVL_STATE_READY)) {
		printk_err("no valid device.\n");
		return -EINVAL;
	}

	mem = (struct videobuf_mfi_memory *)buf->vb.priv;

	write.handle        = dev->mDev.handle_overlay;
	write.overlay_id    = dev->mDev.overlay_id;
	write.image_handle  = dev->mMem.apmem_handle;
	write.user_data     = (unsigned long) buf;
	write.image_address = mem->vaddr;
	write.image_size    = mem->size;

	param.handle       = dev->mDev.handle_overlay;
	param.overlay_id   = dev->mDev.overlay_id;
	if (create_ovl_parameter(&param, &buf->crop, &buf->win, \
		buf->rotate, buf->mirror) != 0) {
		/* report error */
		return -1;
	}

	if (memcmp(&param, &dev->mDev.prev_param, sizeof(param)) != 0) {
		printk_dbg(_OVL_DBG, "set_parameters");

		if (ape5r_v4l2_get_rtapi_interface()   \
			->screen_overlay_set_parameters(&param) == 0) {
			/* record current config */
			dev->mDev.prev_param = param;
		} else {
			printk_err("[error] set_parameters\n");
			return -1;
		}
	}

	if (dev->mDev.output_mode == RT_DISPLAY_LCD1 || \
	    dev->mDev.output_mode == RT_DISPLAY_LCD2) {

		if (buf->win.chromakey != dev->mDev.prev_chromakey) {
			printk_dbg(_OVL_DBG, "set_chromakey");
			if (sh_mobile_lcdc_keyclr_set(buf->win.chromakey, \
				dev->mDev.output_mode) == 0) {
				/* record current config */
				dev->mDev.prev_chromakey = buf->win.chromakey;
			}
		}

		if (buf->win.global_alpha != dev->mDev.prev_globalalpha) {
			printk_dbg(_OVL_DBG, "set_global_alpha");
			if (sh_mobile_lcdc_alpha_set( \
				255-buf->win.global_alpha, \
				dev->mDev.output_mode) == 0) {
				/* record current config */
				dev->mDev.prev_globalalpha = \
					buf->win.global_alpha;
			}
		}
	}

	if (ape5r_v4l2_get_rtapi_interface()   \
		->screen_overlay_write_image(&write) != 0) {
		printk_err("[error] write_image\n");
		return -1;
	}
#endif
	return ret;
}


void videobuf_queue_ape5rbuf_init(struct videobuf_queue *q,
				    const struct videobuf_queue_ops *ops,
				    struct device *dev,
				    spinlock_t *irqlock,
				    enum v4l2_buf_type type,
				    enum v4l2_field field,
				    unsigned int msize,
				    void *priv)
{
	videobuf_queue_core_init(q, ops, dev, irqlock, type, field, msize,
				 priv, &qops);
}

void videobuf_ape5rbuf_free(struct videobuf_queue *q,
			      struct videobuf_buffer *buf)
{
	struct videobuf_mfi_memory *mem = buf->priv;

	/* mmapped memory can't be freed here, otherwise mmapped region
	   would be released, while still needed. In this case, the memory
	   release should happen inside videobuf_vm_close().
	   So, it should free memory only if the memory were allocated for
	   read() operation.
	 */
	if (buf->memory != V4L2_MEMORY_USERPTR)
		return;

	if (!mem)
		return;

	MAGIC_CHECK(mem->magic, MAGIC_DC_MEM);

	/* handle user space pointer case */
	if (buf->baddr) {
		printk_dbg(_MEM_DBG, "release user memory\n");
		videobuf_mfi_user_put(q->priv_data, mem);
		return;
	}

	mem->vaddr = NULL;
}

