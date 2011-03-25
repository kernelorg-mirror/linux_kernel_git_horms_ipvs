/*
 * Function        : V4L2 driver for APE5R
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

#include <media/v4l2-common.h>
#include <media/videobuf-core.h>
#include <rtapi/screen_overlay.h>

/*===============================================================*/
/* define for switch                                            */
/*===============================================================*/
#define RTAPI_MEMORY_INTERFACE    1

/*===============================================================*/
/* external functions                                            */
/*===============================================================*/
extern int ape5r_v4l2_HDMIstate_comflict(void);

extern void videobuf_queue_ape5rbuf_init(struct videobuf_queue *q,
		const struct videobuf_queue_ops *ops,
		struct device *dev,
		spinlock_t *irqlock,
		enum v4l2_buf_type type,
		enum v4l2_field field,
		unsigned int msize,
		void *priv);

extern void videobuf_ape5rbuf_free(struct videobuf_queue *q,
		struct videobuf_buffer *buf);

#if RTAPI_MEMORY_INTERFACE
struct ape5r_v4l2_device;
struct ape5r_v4l2_videobuffer;
extern void videobuf_queue_ape5r_interface_release(                    \
	struct ape5r_v4l2_device *dev);
extern void videobuf_queue_ape5r_interface_init(                       \
	struct ape5r_v4l2_device *dev);
extern int  videobuf_queue_ape5r_interface_config(                     \
	struct ape5r_v4l2_device *dev);

extern int  videobuf_queue_ape5r_interface_postimage(                  \
	struct ape5r_v4l2_videobuffer *buf, struct ape5r_v4l2_device *dev);
extern int  videobuf_queue_ape5r_interface_createoverlay(              \
	struct ape5r_v4l2_device *dev, int type, void (*callback)(void *));
extern int  videobuf_queue_ape5r_interface_destroyoverlay(             \
	struct ape5r_v4l2_device *dev);
#endif

struct ape5r_v4l2_fh;

/*===============================================================*/
/* define for capability                                         */
/*===============================================================*/
#define CAP_CLIP1     0x01
#define CAP_RESIZE1   0x02
#define CAP_FMTCONV1  0x04


/*===============================================================*/
/* define for status                                             */
/* lower-16bit:  indicate now status transition                  */
/* higher-16bit: indicate complete status transition             */
/*===============================================================*/
#define STAT_STREAMINGON     0x00000001
#define STAT_STREAMINGOFF    0x00000002

#define STAT_HDMI_STOP       0x00000010

#define STAT_CONFIG_SFMTOUT  0x00010000  /* pixel  */
#define STAT_CONFIG_SFMTOVL  0x00020000  /* window */
#define STAT_CONFIG_SFMTPRV  0x00040000
#define STAT_CONFIG_CROPOVL  0x00080000  /* crop   */


/*===============================================================*/
/* define for event                                              */
/*===============================================================*/
#define EVENT_UPDATEQUEUE    0x01   /* QBUF  is  processed.    */
#define EVENT_REQUESTNEXT    0x02   /* QBUF  is  processed.    */
#define EVENT_START          0x03   /* STREAMON  is processed. */
#define EVENT_STOP           0x04   /* STREAMOFF is processed. */
#define EVENT_STOPHDMI       0x05   /* HDMI stop request.      */

#define EVENT_NOP            0xFF   /* used for timeout        */

/*===============================================================*/
/* define for kernel thread                                      */
/*===============================================================*/
#if defined(_V4L2_RT_THREAD) && _V4L2_RT_THREAD /* RT thread */
#define V4L2_THREAD_PRIORITY    1   /* thread priority */
#else /* Normal thread */
#define V4L2_THREAD_NICE    -20 /* thread priority */
#endif



/*===============================================================*/
/* custom video buffer                                           */
/*===============================================================*/
struct ape5r_v4l2_videobuffer {
    struct videobuf_buffer  vb;

    /* custom parameters */
    struct v4l2_pix_format  pix;
    struct v4l2_window      win;
    struct v4l2_crop        crop;
    int                     rotate;
    int                     mirror;

    struct list_head        work_queue;
    struct videobuf_queue   *vbq;
};

#if RTAPI_MEMORY_INTERFACE
/*===============================================================*/
/* custom video buffer                                           */
/*===============================================================*/
struct ape5r_v4l2_devappmemory {
	void            *handle;
	void            *apmem_handle;
	unsigned int    mRefCount;
};
struct ape5r_v4l2_devcontrol {
	void            *handle_overlay;
	unsigned short  overlay_id;
	unsigned short  output_mode;
	unsigned int    state;

	void            (*callback)(void *);
	screen_ovl_set_param prev_param;
	unsigned short       prev_chromakey;
	unsigned short       prev_globalalpha;
};
#endif

/*===============================================================*/
/* per-device data structure                                     */
/*===============================================================*/
struct ape5r_th_object {
    struct task_struct      *th;
    char                    *th_name;   /* thread name */
    wait_queue_head_t       th_wait;
    wait_queue_head_t       th_ack;
    int                     flag;
    int                     rp;
    int                     wp;
    unsigned long           events[8];
};

struct ape5r_v4l2_device {
    /* link list to next device */
    struct list_head        devlist;

    struct device           dev;
    struct video_device     *vfd;

    /* semafore for serialize operation */
    struct semaphore        sem_lock;
    /* spinlock for videobuf queues */
    spinlock_t                  vbq_lock;
    /* videobuf queue operations    */
    struct videobuf_queue_ops   vbq_ops;
    /* counter for videobuf_buffer  */
    unsigned long               sequence;

    /* We allow streaming from at most one filehandle at a time.
     * non-NULL means streaming is in progress.
     */
    struct ape5r_v4l2_fh   *streaming;

    /* function per output target. */
    int (*check_param)(struct ape5r_v4l2_device *);
    /* device states.                 */
    unsigned long           status;

    /* revise time data */
#if 0
    /* timer link list */
    struct timer_list       wait_timer;
#endif
    /* kernel thread data */
    struct ape5r_th_object  th_lcd;
    /* link list to next workqueue */
    struct list_head        vbq_in;
    struct list_head        vbq_work;
    int                     vbq_worknum;

    /* pix defines the size and pixel format of the image captured by the
     * sensor.  This also defines the size of the framebuffers.  The
     * same pool of framebuffers is used for video capture and video
     * overlay.  These parameters are set/queried by the
     * VIDIOC_S_FMT/VIDIOC_G_FMT ioctls with a CAPTURE buffer type.
     */
    struct v4l2_pix_format  pix;
    struct v4l2_window      win;
    struct v4l2_crop        crop;

    /* saving work buffer data */
    struct v4l2_workbuffer  workbuf;

#if RTAPI_MEMORY_INTERFACE
    struct ape5r_v4l2_devappmemory mMem;
    struct ape5r_v4l2_devcontrol   mDev;
#endif
    /* saving output device identifier */
    unsigned int            output;
    unsigned int            rotate;
    unsigned int            mirror;
    unsigned int            bg_color;

    char                suspend;
    char                active;
};


/*===============================================================*/
/* device name definetion                                        */
/*===============================================================*/
#define DEV_NAME      "ape5r_overlay"
#define DEV_VERSION   1
