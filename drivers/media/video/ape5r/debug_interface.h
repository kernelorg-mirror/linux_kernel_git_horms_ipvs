/*
 * Function        : Video for Linux driver for PROC I/F core
 *
 * Copyright (C) 2011 Renesas Electronics Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Suite 500, Boston, MA 02110-1335, USA.
 */

#undef REGIST_PROCINTERFACE
#undef UNREGIST_PROCINTERFACE
#define REGIST_PROCINTERFACE     proc_info_regist();
#define UNREGIST_PROCINTERFACE   proc_info_unregist();

static void dump_buffer(struct ape5r_v4l2_videobuffer *emvb);

/****************************************************
 * Dump queue information.
 ***************************************************/
static void dump_streambuffer(char *message, struct videobuf_queue *vbq)
{
	struct list_head       *head;
	struct list_head       *list;
	struct videobuf_buffer *vb;

	if (vbq == NULL)
		return;

	head = &vbq->stream;

	printk_dbg(1, "message:%s ", message);
	list_for_each(list, head)
	{
		vb = list_entry(list, struct videobuf_buffer, stream);
		printk_dbg(1, " %d(%d) ", vb->i, vb->state);
	}
	printk_dbg(1, "\n");
}

static int __proc_open(struct inode *inode, struct file *file)
{
	file->private_data = 0;
	return 0;
}
static int __proc_close(struct inode *inode, struct file *file)
{
	return 0;
}
static int     __create_message(char *buf, size_t len, \
	struct ape5r_v4l2_device *dev)
{
	char *p  = buf;
	int  n   = len;
	int  c;
	int  i;
	struct list_head       *list;
	struct videobuf_buffer *vb;
	/*---------------------------
	- generate proc message.
	---------------------------*/
	c = snprintf(p, n, "  sequence:%ld\n", dev->sequence);
	if (c < n) {
		/* move pointer */
		p += c; n -= c;
	}
	c = snprintf(p, n, "  status:%lx\n", dev->status);
	if (c < n) {
		/* move pointer */
		p += c; n -= c;
	}
	c = snprintf(p, n, "  streaming:%p\n", dev->streaming);
	if (c < n) {
		/* move pointer */
		p += c; n -= c;
	}
	c = snprintf(p, n, "  vbq_worknum:%d\n",
		dev->vbq_worknum);
	if (c < n) {
		/* move pointer */
		p += c; n -= c;
	}
	c = snprintf(p, n, "  mes queue(r:%d,w:%d)\n",
	   dev->th_lcd.rp, dev->th_lcd.wp);
	if (c < n) {
		/* move pointer */
		p += c; n -= c;
	}
	for (i = 0; i < 8; i++) {
		c = snprintf(p, n, "  message%d:%lx\n",
			i, dev->th_lcd.events[i]);
		if (c < n) {
			/* move pointer */
			p += c; n -= c;
		}
	}
	c = snprintf(p, n, "  in-queued list:\n");
	if (c < n) {
		/* move pointer */
		p += c; n -= c;
	}
	list_for_each(list, &dev->vbq_in)
	{
		vb = list_entry(list, struct videobuf_buffer, queue);
		c = snprintf(p, n, " %d(%d) ", vb->i, vb->state);
		if (c < n) {
			/* move pointer */
			p += c; n -= c;
		}
	}
	c = snprintf(p, n, "\n");
	if (c < n) {
		/* move pointer */
		p += c; n -= c;
	}

	c = snprintf(p, n, "  work-queued list:\n");
	if (c < n) {
		/* move pointer */
		p += c; n -= c;
	}
	list_for_each(list, &dev->vbq_work)
	{
		struct ape5r_v4l2_videobuffer *emvb;

		emvb = list_entry(list,
			struct ape5r_v4l2_videobuffer, work_queue);
		c = snprintf(p, n, " %d(%d) ",
			emvb->vb.i, emvb->vb.state);
		if (c < n) {
			/* move pointer */
			p += c; n -= c;
		}
	}
	c = snprintf(p, n, "\n");
	if (c < n) {
		/* move pointer */
		p += c; n -= c;
	}

	c = snprintf(p, n, "  streaming-queued list:\n");
	if (c < n) {
		/* move pointer */
		p += c; n -= c;
	}
	if (dev->streaming) {
		struct videobuf_queue   *vbq;
		vbq = &dev->streaming->vbq;

		list_for_each(list, &vbq->stream)
		{
			vb = list_entry(list,
				struct videobuf_buffer, stream);
			c = snprintf(p, n, " %d(%d) ",
				vb->i, vb->state);
			if (c < n) {
				/* move pointer */
				p += c; n -= c;
			}
		}
		c = snprintf(p, n, "\n");
		if (c < n) {
			/* move pointer */
			p += c; n -= c;
		}
	}
	c = snprintf(p, n, "\n");
	if (c < n) {
		/* move pointer */
		p += c; n -= c;
	}

	/**************/
	/* FMT output */
	c = snprintf(p, n, "  S_FMT[output]\n");
	if (c < n) {
		/* move pointer */
		p += c; n -= c;
	}
	c = snprintf(p, n, "    %dx%d", dev->pix.width, \
		dev->pix.height);
	if (c < n) {
		/* move pointer */
		p += c; n -= c;
	}
	c = snprintf(p, n, " fmt:0x%x", dev->pix.pixelformat);
	if (c < n) {
		/* move pointer */
		p += c; n -= c;
	}
	c = snprintf(p, n, " linebyte:%d field:%d colspace:%d",  \
		dev->pix.bytesperline, dev->pix.field, dev->pix.colorspace);
	if (c < n) {
		/* move pointer */
		p += c; n -= c;
	}
	c = snprintf(p, n, " mirror:%d rotate:%d\n",    \
		dev->mirror, dev->rotate);
	if (c < n) {
		/* move pointer */
		p += c; n -= c;
	}
	/***************/
	/* FMT overlay */
	c = snprintf(p, n, "  S_FMT[overlay]\n");
	if (c < n) {
		/* move pointer */
		p += c; n -= c;
	}
	c = snprintf(p, n, "    (%d,%d)-(%d,%d)",   \
		dev->win.w.left,  dev->win.w.top,   \
		dev->win.w.left + dev->win.w.width, \
		dev->win.w.top  + dev->win.w.height);
	if (c < n) {
		/* move pointer */
		p += c; n -= c;
	}
	c = snprintf(p, n, " key:0x%x alpha:%d\n",  \
		dev->win.chromakey,                 \
		dev->win.global_alpha);
	if (c < n) {
		/* move pointer */
		p += c; n -= c;
	}
	/***************/
	/* FMT private */
#if RTAPI_MEMORY_INTERFACE
	c = snprintf(p, n, "  S_FMT[private]\n");
	if (c < n) {
		/* move pointer */
		p += c; n -= c;
	}
	c = snprintf(p, n, "    id:0x%x addr:0x%x nRef:%d\n",  \
		dev->workbuf.apmem_id,                         \
		dev->workbuf.apmem_addr,                       \
		dev->mMem.mRefCount);
	if (c < n) {
		/* move pointer */
		p += c; n -= c;
	}
#endif
	/*********/
	/* CROP  */
	c = snprintf(p, n, "  CROP[overlay]\n");
	if (c < n) {
		/* move pointer */
		p += c; n -= c;
	}
	c = snprintf(p, n, "    (%d,%d)-(%d,%d)\n",   \
		dev->crop.c.left,  dev->crop.c.top,   \
		dev->crop.c.left + dev->crop.c.width, \
		dev->crop.c.top  + dev->crop.c.height);
	if (c < n) {
		/* move pointer */
		p += c; n -= c;
	}
	return p-buf;
}

static ssize_t __proc_read(struct file *file,
	char __user *buf, size_t len, loff_t *ppos)
{
	char *p  = buf;
	int  n   = len;
	int  c;
	if (file->private_data != 0 || *ppos != 0) {
		/* nothing to do */
		return 0;
	}

	/*---------------------------
	- generate proc message.
	---------------------------*/
	c = snprintf(p, n, "ape5r_dev->\n");
	if (c < n) {
		/* move pointer */
		p += c; n -= c;
	}
	if (ape5r_dev) {
		c = __create_message(p, n, ape5r_dev);
		if (c < n) {
			/* move pointer */
			p += c; n -= c;
		}
	}
	*ppos = p-buf;
	return p-buf;
}

static ssize_t __proc_write(struct file *file,
	const char __user *buf, size_t len, loff_t *ppos)
{
	struct ape5r_v4l2_device	*dev;
	unsigned long flags;

	switch (buf[0]) {
	case '0':
		/* notify of done of video buffer. */
		if (ape5r_dev == NULL) {
			printk_dbg(_LOG_DBG,
				"not started. ape5r_dev(%p)\n", ape5r_dev);
		} else if (ape5r_dev->streaming == NULL) {
			printk_dbg(_LOG_DBG,
				"not started. ape5r_dev(%p) streaming(%p)\n",
				ape5r_dev, ape5r_dev->streaming);
		} else {
			dev  = ape5r_dev->streaming->dev;

			dbg_lock(&dev->vbq_lock, "vbq_lock\n");
			spin_lock_irqsave(&dev->vbq_lock, flags);

			if (list_empty(&dev->vbq_work)) {
				/* there is no active buffer. */
				printk_dbg(_LOG_DBG, "list empty.\n");
			} else {
				struct ape5r_v4l2_videobuffer *emvb;

				printk_dbg(_LOG_DBG, "switch to next queue.\n");

				emvb = list_entry(dev->vbq_work.next,
					struct ape5r_v4l2_videobuffer,
					work_queue);
				emvb->vb.state = VIDEOBUF_DONE;
				do_gettimeofday(&emvb->vb.ts);

				list_del(&emvb->work_queue);
				if (dev->vbq_worknum > 0) {
					/* decrement work_queue counts. */
					dev->vbq_worknum--;
				}
				dump_buffer(emvb);
				wake_up(&emvb->vb.done);
				ape5r_v4l2_thread_postmessage(dev,
					EVENT_REQUESTNEXT, NULL);
			}
			spin_unlock_irqrestore(&dev->vbq_lock, flags);
		}
		break;
	default:
		break;
	}
	return len;
}
static struct proc_dir_entry *proc_entry;
static const struct file_operations proc_fops = {
	.owner		= THIS_MODULE,
	.open		= __proc_open,
	.read		= __proc_read,
	.write		= __proc_write,
	.release	= __proc_close,
};
static void proc_info_regist(void)
{
	proc_entry = proc_create("dbg_video", 0666, NULL, &proc_fops);
}
static void proc_info_unregist(void)
{
	if (proc_entry) {
		remove_proc_entry("dbg_video", proc_entry);
		proc_entry = NULL;
	}
}
