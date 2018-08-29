/*
 * Copyright (C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 *
 */

#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/of.h>
#include <linux/io.h>
#include <linux/pci.h>
#include <linux/platform_device.h>
#include <linux/spinlock.h>
#include <linux/timer.h>
#include <linux/atomic.h>
#include <asm/irqflags.h>
#include <linux/atomic.h>
#include <linux/completion.h>
#include <linux/dma-mapping.h>
#include "mt_spm.h"

#ifndef CONFIG_MTK_CLKMGR
#define MTK_UFOZIP_USING_CCF
#endif

#ifndef CONFIG_MTK_CLKMGR
#include <linux/clk.h>
#else
#include <mach/mt_clkmgr.h>  /* For clock mgr APIS. enable_clock() & disable_clock(). */
#endif

/* Trigger method for screen on/off */
#include <linux/fb.h>

/* hwzram impl header file */
#include "../hwzram_impl.h"

/* UFOZIP private header file */
#include "ufozip_private.h"

static unsigned int ufozip_default_fifo_size = 256;

/* Write to 32-bit register */
static void ufozip_write_register(struct hwzram_impl *hwz, unsigned int offset, uint64_t val)
{
	writel(val, hwz->regs + offset);
}

#if 0 /* Unused */
/* Read from 32-bit register */
static uint32_t ufozip_read_register(struct hwzram_impl *hwz, unsigned int offset)
{
	uint32_t ret;

	ret = readl(hwz->regs + offset);

	return ret;
}
#endif

/* Configure UFOZIP */
static void set_apb_default_normal(struct hwzram_impl *hwz, struct UFOZIP_CONFIG *uzcfg)
{
	int val;
	int axi_max_rout, axi_max_wout;

	/*
	 * bit[0] : enable clock for encoder and AXI
	 * bit[16]: bypass enc latency
	 */
	ufozip_write_register(hwz, ENC_DCM, 0x10001);

	/*
	 * bit[0] : enable clock for decoder and AXI
	 * bit[4] : bypass dec latency
	 */
	ufozip_write_register(hwz, DEC_DCM, 0x11);

	/*
	 * bit[12..0] : blocksize
	 * bit[25..20]: cutvalue
	 * bit[26]    : componly mode
	 * bit[27]    : refbyte_en
	 * bit[31..28]: level
	 */
	ufozip_write_register(hwz, ENC_CUT_BLKSIZE,
			(uzcfg->blksize >> 4) |
			(uzcfg->cutValue << 20) |
			(uzcfg->componly_mode << 26) |
			(uzcfg->compress_level << 28) |
			(uzcfg->refbyte_flag << 27) |
			(uzcfg->hybrid_decode << 31));

	/*
	 * bit[7..0]  : lenpos_max
	 * bit[24..8] : limitsize_add16
	 */
	ufozip_write_register(hwz, ENC_LIMIT_LENPOS,
			(uzcfg->limitSize << 8) |
			(uzcfg->hwcfg.lenpos_max));

	/* ligzh, fix default value of usmin and intf_us_margin */
	if (uzcfg->hwcfg.usmin < 0x120)
		uzcfg->hwcfg.usmin = 0x120;
	else if (uzcfg->hwcfg.usmin > 0xecf)
		uzcfg->hwcfg.usmin = 0xecf;

	/* For debug, default: 0x120 */
	ufozip_write_register(hwz, ENC_DSUS,
			(uzcfg->hwcfg.dsmax << 16) |
			(uzcfg->hwcfg.usmin));

	/* intf_us_margin */
	if (uzcfg->hwcfg.enc_intf_usmargin < (uzcfg->hwcfg.usmin + 0x111))
		uzcfg->hwcfg.enc_intf_usmargin = uzcfg->hwcfg.usmin + 0x111;
	else if (uzcfg->hwcfg.enc_intf_usmargin > 0xfa0)
		uzcfg->hwcfg.enc_intf_usmargin = 0xfa0;

	/* us guard margin, default 0x1000 */
	ufozip_write_register(hwz, ENC_INTF_USMARGIN, uzcfg->hwcfg.enc_intf_usmargin);

	/* Read/Write threshold */
	ufozip_write_register(hwz, ENC_INTF_RD_THR, uzcfg->blksize + 128);
	val = (uzcfg->limitSize + 128) >= 0x20000 ? 0x1ffff : (uzcfg->limitSize + 128);
	ufozip_write_register(hwz, ENC_INTF_WR_THR, val);

	/* set "time out value" */
	if (uzcfg->hybrid_decode)
		ufozip_write_register(hwz, DEC_TMO_THR, 0xFFFFFFFF);

#if defined(MT5891)
	/* Unnecessary */
	ufozip_write_register(hwz, ENC_AXI_PARAM0, 0x01023023);
	ufozip_write_register(hwz, DEC_AXI_PARAM0, 0x01023023);
	ufozip_write_register(hwz, ENC_AXI_PARAM1, 0x1);
	ufozip_write_register(hwz, DEC_AXI_PARAM1, 0x1);
	ufozip_write_register(hwz, ENC_AXI_PARAM2, 0x100);
	ufozip_write_register(hwz, DEC_AXI_PARAM2, 0x100);
	ufozip_write_register(hwz, ENC_INTF_BUS_BL, 0x0404);
	ufozip_write_register(hwz, DEC_INTF_BUS_BL, 0x0404);
#else	    /* end 5891 axi parameters */
	/* AXI Misc parameters 1, for debug */
	ufozip_write_register(hwz, ENC_AXI_PARAM1, 0x10);
	ufozip_write_register(hwz, DEC_AXI_PARAM1, 0x10);
#endif

	/*
	 * Default 0x1222.
	 * In normal mode, 0x1222 for 4KB block size; 0x1000 for block size > 4KB.
	 * In debug mode, minimum 0x330.
	 */
	ufozip_write_register(hwz, DEC_DBG_OUTBUF_SIZE, uzcfg->dictsize);

	/*
	 * bit[0]: set to 1 disable blk_done interrupt
	 * bit[1]: set to 1 disable tmo_fire interrupt
	 * bit[2]: set to 1 disable core_abnormal( exceed limitsize) interrupt
	 * bit[3]: set to 1 disable sw rst mode 0 ready interrupt
	 * bit[4]: set to 1 disable axi error interrupt
	 * bit[5]: set to 1 disable sw rst mode 1 done interrupt
	 * bit[6]: set to 1 disable zram block interrupt
	 */
	ufozip_write_register(hwz, ENC_INT_MASK, 0x4);

	/* UFOZIP_HWCONFIG */
	switch (uzcfg->hwcfg_sel) {
	case HWCFG_INTF_BUS_BL:
		/* AXI read/write burst length */
		ufozip_write_register(hwz, ENC_INTF_BUS_BL, uzcfg->hwcfg.intf_bus_bl);
		ufozip_write_register(hwz, DEC_INTF_BUS_BL, uzcfg->hwcfg.intf_bus_bl);
		break;
	case HWCFG_TMO_THR:
		/* Timeout */
		ufozip_write_register(hwz, ENC_TMO_THR, uzcfg->hwcfg.tmo_thr);
		ufozip_write_register(hwz, DEC_TMO_THR, uzcfg->hwcfg.tmo_thr);
		break;
	case HWCFG_ENC_DSUS:
		/* For debug, default: 0x120 */
		ufozip_write_register(hwz, ENC_DSUS, uzcfg->hwcfg.enc_dsus);
		break;
	case HWCFG_ENC_LIMIT_LENPOS:
		/* ENC_LIMIT_LENPOS */
		ufozip_write_register(hwz, ENC_LIMIT_LENPOS, (uzcfg->limitSize << 8) |
							(uzcfg->hwcfg.lenpos_max));
		ufozip_write_register(hwz, ENC_INT_MASK, 0x4);
		break;
	case HWCFG_ENC_INTF_USMARGIN:
		/* us guard margin, default 0x1000 */
		ufozip_write_register(hwz, ENC_INTF_USMARGIN, uzcfg->hwcfg.enc_intf_usmargin);
		break;
	case HWCFG_AXI_MAX_OUT:
		axi_max_rout = (uzcfg->hwcfg.axi_max_out & 0xFF000000) | 0x00023023;
		axi_max_wout = uzcfg->hwcfg.axi_max_out & 0x000000FF;
		/* AXI Misc parameters 0. AXI side info and max outstanding. */
		ufozip_write_register(hwz, ENC_AXI_PARAM0, axi_max_rout);
		ufozip_write_register(hwz, DEC_AXI_PARAM0, axi_max_rout);
		/* AXI Misc parameters 1, for debug */
		ufozip_write_register(hwz, ENC_AXI_PARAM1, axi_max_wout);
		ufozip_write_register(hwz, DEC_AXI_PARAM1, axi_max_wout);
		break;
	case HWCFG_INTF_SYNC_VAL:
		/* For debug */
		ufozip_write_register(hwz, ENC_INTF_SYNC_VAL, uzcfg->hwcfg.intf_sync_val);
		ufozip_write_register(hwz, DEC_INTF_SYNC_VAL, uzcfg->hwcfg.intf_sync_val);
		break;
	case HWCFG_DEC_DBG_INBUF_SIZE:
		/* For debug */
		ufozip_write_register(hwz, DEC_DBG_INBUF_SIZE, (uzcfg->hwcfg.dec_dbg_inbuf_size & 0x1FFFF));
		break;
	case HWCFG_DEC_DBG_OUTBUF_SIZE:
		/* For debug */
		ufozip_write_register(hwz, DEC_DBG_OUTBUF_SIZE, (uzcfg->hwcfg.dec_dbg_outbuf_size & 0x1FFFF));
		break;
	case HWCFG_DEC_DBG_INNERBUF_SIZE:
		/* For debug */
		ufozip_write_register(hwz, DEC_DBG_INNERBUF_SIZE, (uzcfg->hwcfg.dec_dbg_innerbuf_size & 0x7F));
		break;
	case HWCFG_CORNER_CASE:
		{
			UInt32	lenpos_max;

			ufozip_write_register(hwz, ENC_DCM, 1);
			lenpos_max = 0x2;
			ufozip_write_register(hwz, ENC_LIMIT_LENPOS, (uzcfg->limitSize << 8) |
								(lenpos_max));
			ufozip_write_register(hwz, ENC_INTF_RD_THR, uzcfg->blksize + 128);
			val = (uzcfg->limitSize + 128) >= 0x20000 ? 0x1ffff : (uzcfg->limitSize + 128);
			ufozip_write_register(hwz, ENC_INTF_WR_THR, val);

#if defined(MT5891)
			ufozip_write_register(hwz, ENC_AXI_PARAM0, 0x08023023);
			ufozip_write_register(hwz, DEC_AXI_PARAM0, 0x08023023);
			ufozip_write_register(hwz, ENC_AXI_PARAM1, 0x1);
			ufozip_write_register(hwz, DEC_AXI_PARAM1, 0x1);
			ufozip_write_register(hwz, ENC_AXI_PARAM2, 0x100);
			ufozip_write_register(hwz, DEC_AXI_PARAM2, 0x100);
			ufozip_write_register(hwz, ENC_INTF_BUS_BL, 0x0404);
			ufozip_write_register(hwz, DEC_INTF_BUS_BL, 0x0404);
#else	    /* end 5891 axi parameters */
			ufozip_write_register(hwz, ENC_AXI_PARAM1, 0x10);
			ufozip_write_register(hwz, DEC_AXI_PARAM1, 0x10);
#endif
			ufozip_write_register(hwz, ENC_INT_MASK, 0x4); /* mask exceed limitsize irq for simu */

			ufozip_write_register(hwz, ENC_INTF_BUS_BL, 0x0202);
			ufozip_write_register(hwz, DEC_INTF_BUS_BL, 0x0202);

			axi_max_rout = 0x01023023; /* 0x01000000 | 0x00023023; //0x00023023:default value in [23:0] */
			axi_max_wout = 0x1; /* tc_data & 0x000000FF; */
			ufozip_write_register(hwz, ENC_AXI_PARAM0, axi_max_rout);
			ufozip_write_register(hwz, DEC_AXI_PARAM0, axi_max_rout);
			ufozip_write_register(hwz, ENC_AXI_PARAM1, axi_max_wout);
			ufozip_write_register(hwz, DEC_AXI_PARAM1, axi_max_wout);

			ufozip_write_register(hwz, DEC_DBG_INBUF_SIZE, 0x130);
			ufozip_write_register(hwz, DEC_DBG_OUTBUF_SIZE, 0x330);
			ufozip_write_register(hwz, DEC_DBG_INNERBUF_SIZE, 0x2);

			ufozip_write_register(hwz, DEC_INT_MASK, 0xFFFFFFDE);  /* open only dec done and rst_done */
			ufozip_write_register(hwz, ENC_INT_MASK, 0xFFFFFFDE);  /* open only dec done and rst_done */
		}
		break;
	}

	/* Is it hybrid decoding */
	if (uzcfg->hybrid_decode)
		uzcfg->dictsize	= DEC_DICTSIZE;
	else
		if (uzcfg->dictsize > (USBUF_SIZE-uzcfg->hwcfg.enc_intf_usmargin))
			uzcfg->dictsize	= USBUF_SIZE-uzcfg->hwcfg.enc_intf_usmargin - 48;

	ufozip_write_register(hwz, DEC_CONFIG, (uzcfg->blksize >> 4) |
					  (uzcfg->refbyte_flag << 20) |
					  (uzcfg->hybrid_decode << 21));
	ufozip_write_register(hwz, DEC_CONFIG2, uzcfg->dictsize);

	ufozip_write_register(hwz, DEC_DBG_INBUF_SIZE, uzcfg->hwcfg.dec_dbg_inbuf_size);

	/*
	 * bit[0]: set to 1 disable blk_done interrupt
	 * bit[1]: set to 1 disable tmo_fire interrupt
	 * bit[2]: set to 1 disable dec_err interrupt
	 * bit[3]: set to 1 disable sw rst mode 0 ready interrupt
	 * bit[4]: set to 1 disable axi error interrupt
	 * bit[5]: set to 1 disable sw rst mode 1 done interrupt
	 */
	ufozip_write_register(hwz, DEC_INT_MASK, 0xFFFFFFFA);

	/* Mode for ZRAM accelerator */
	if (uzcfg->batch_mode == 3)
		ufozip_write_register(hwz, ENC_INT_MASK, 0xFFFFFF9E); /* open only enc done and rst_done */
	else
		ufozip_write_register(hwz, ENC_INT_MASK, 0xFFFFFFDE); /* open only enc done and rst_done */

	/*
	 * bit[0]: set to 1 disable blk_done interrupt
	 * bit[1]: set to 1 disable tmo_fire interrupt
	 * bit[2]: set to 1 disable core_abnormal( exceed limitsize) interrupt
	 * bit[3]: set to 1 disable sw rst mode 0 ready interrupt
	 * bit[4]: set to 1 disable axi error interrupt
	 * bit[5]: set to 1 disable sw rst mode 1 done interrupt
	 * bit[6]: set to 1 disable zram block interrupt (equal to ZRAM_INTRP_MASK_CMP)
	 */
	ufozip_write_register(hwz, ENC_INT_MASK, 0xFFFFFFBB);

	ufozip_write_register(hwz, ENC_PROB_STEP , (uzcfg->dictsize << 15) |
					      (uzcfg->hashmask << 2)|
					      (uzcfg->probstep));
	ufozip_write_register(hwz, DEC_PROB_STEP , uzcfg->probstep);
}

/* FB event notifier */
static int ufozip_lowpower_fb_event(struct notifier_block *notifier, unsigned long event, void *data)
{
#ifndef CONFIG_MTK_CLKMGR /* only support @ CCF & DVFS chip */
	struct fb_event *fb_event = data;
	int blank;

	if (event != FB_EVENT_BLANK)
		return NOTIFY_DONE;

	blank = *(int *)fb_event->data;

	pr_info("ufozip fb event event = %ld, blank = %d !\n", event, blank);

	switch (blank) {
	case FB_BLANK_UNBLANK:
		pr_info("UFOZIP screen on !!\n");
		break;
	case FB_BLANK_NORMAL:
	case FB_BLANK_VSYNC_SUSPEND:
	case FB_BLANK_HSYNC_SUSPEND:
	case FB_BLANK_POWERDOWN:
		pr_info("UFOZIP screen off !!, clock need to switch to slow ?\n");
		break;
	default:
		return -EINVAL;
	}
#endif
	return NOTIFY_OK;
}


static struct notifier_block fb_ufozip_notifier_block = {
	.notifier_call = ufozip_lowpower_fb_event,
	.priority = 0,
};

static void UFOZIP_HwInit(struct hwzram_impl *hwz)
{
	u32 batch_blocknum = 16;
	int zip_blksize = 4096;
	u32 limit_ii;
	unsigned long flags;
	struct UFOZIP_CONFIG   uzip_config;

#ifdef	CONFIG_MTK_CLKMGR /*not support CCF, Rushmore only*/
	spm_set_sleep_ufozip_sram_config(UFO_SRAM_SPM_ON);
#endif

	pr_info("%s start ...\n", __func__);
	limit_ii = 4096;
	pr_info("UFOZIP test blksize=%x, limitsize=%x\n", zip_blksize, limit_ii);
	uzip_config.compress_level	= 1;
	uzip_config.refbyte_flag	= 1;
	uzip_config.hwcfg.lenpos_max	= 0x7e;
	uzip_config.hwcfg.dsmax		= 0xff0;
	uzip_config.hwcfg.usmin		= 0x0120;
	uzip_config.hwcfg.enc_intf_usmargin	= uzip_config.hwcfg.usmin + 0x111;
	uzip_config.hwcfg.dec_dbg_inbuf_size	= 0x400;
	uzip_config.componly_mode	= 0;
	uzip_config.cutValue		= 8;
	uzip_config.blksize		= zip_blksize;
	uzip_config.dictsize		= 1 << 10;
	uzip_config.hashmask		= 2 * 1024 - 1;
	uzip_config.limitSize		= limit_ii;
	uzip_config.batch_mode		= 3;
	uzip_config.batch_blocknum	= batch_blocknum;
	uzip_config.batch_srcbuflen	= 16 * 4096;
	uzip_config.hwcfg_sel		= HWCFG_NONE;
	uzip_config.hwcfg.intf_bus_bl	= 0x00000404;
	uzip_config.probstep		= 0;
	uzip_config.hybrid_decode	= 0;
	local_irq_save(flags);
	set_apb_default_normal(hwz, &uzip_config);
	local_irq_restore(flags);

	pr_info("%s finish ...\n", __func__);
}

static void ufozip_platform_init(struct hwzram_impl *hwz,
				 unsigned int vendor, unsigned int device)
{
	if (vendor == ZRAM_VENDOR_ID_MEDIATEK) {
		UFOZIP_HwInit(hwz);
		pr_info("%s: done\n", __func__);
	} else {
		pr_warn("%s: mismatched vendor %u, should be %u\n",
				__func__, vendor, ZRAM_VENDOR_ID_MEDIATEK);
	}
}

#ifdef	CONFIG_MTK_CLKMGR
static DEFINE_SPINLOCK(hclklock);
static int hclk_count;
#endif
/* bus clock */
static void ufozip_hclkctrl(enum platform_ops ops)
{
#ifdef	CONFIG_MTK_CLKMGR
	unsigned long flags;
	int err = 0;

	spin_lock_irqsave(&hclklock, flags);
	if (ops == COMP_ENABLE || ops == DECOMP_ENABLE) {
		if (hclk_count++ == 0) {
			err = enable_clock(MT_CG_UFOZIP_HCLK_SW_CG, "UFOZIP");
			if (err)
				pr_err("%s: ops(%d) err(%d)\n", __func__, ops, err);
		}
	} else {
		if (--hclk_count == 0) {
			err = disable_clock(MT_CG_UFOZIP_HCLK_SW_CG, "UFOZIP");
			if (err)
				pr_err("%s: ops(%d) err(%d)\n", __func__, ops, err);
		}
	}
	spin_unlock_irqrestore(&hclklock, flags);
#endif
}

/* compression core clock */
static void ufozip_enc_clkctrl(enum platform_ops ops)
{
#ifdef	CONFIG_MTK_CLKMGR
	static DEFINE_SPINLOCK(lock);
	static int enc_count;
	unsigned long flags;
	int err = 0;

	spin_lock_irqsave(&lock, flags);
	if (ops == COMP_ENABLE) {
		if (enc_count++ == 0) {
			err = enable_clock(MT_CG_UFOZIP_ENC_CLK_SW_CG, "UFOZIP");
			if (err)
				pr_err("%s: ops(%d) err(%d)\n", __func__, ops, err);
		}
	} else {
		if (--enc_count == 0) {
			err = disable_clock(MT_CG_UFOZIP_ENC_CLK_SW_CG, "UFOZIP");
			if (err)
				pr_err("%s: ops(%d) err(%d)\n", __func__, ops, err);
		}
	}
	spin_unlock_irqrestore(&lock, flags);
#endif
}

/* decompression core clock */
static void ufozip_dec_clkctrl(enum platform_ops ops)
{
#ifdef	CONFIG_MTK_CLKMGR
	static DEFINE_SPINLOCK(lock);
	static int dec_count;
	unsigned long flags;
	int err = 0;

	spin_lock_irqsave(&lock, flags);
	if (ops == DECOMP_ENABLE) {
		if (dec_count++ == 0) {
			err = enable_clock(MT_CG_UFOZIP_DEC_CLK_SW_CG, "UFOZIP");
			if (err)
				pr_err("%s: ops(%d) err(%d)\n", __func__, ops, err);
		}
	} else {
		if (--dec_count == 0) {
			err = disable_clock(MT_CG_UFOZIP_DEC_CLK_SW_CG, "UFOZIP");
			if (err)
				pr_err("%s: ops(%d) err(%d)\n", __func__, ops, err);
		}
	}
	spin_unlock_irqrestore(&lock, flags);
#endif
}

static void ufozip_clock_control(enum platform_ops ops)
{
	ufozip_hclkctrl(ops);

	if (ops > COMP_DISABLE)
		ufozip_dec_clkctrl(ops);
	else
		ufozip_enc_clkctrl(ops);
}

#ifdef CONFIG_OF
static int ufozip_of_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct hwzram_impl *hwz;
	struct resource *mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	int irq = platform_get_irq(pdev, 0);

	pr_devel("%s starting: got irq %d\n", __func__, irq);

	/*
	 * Set dma_mask -
	 * UFOZIP supports 34-bit address for accessing DRAM.
	 * Only higher 32bits(bit 33-2) can be configured and
	 * the lower 2bits(bit 1-0) are fixed zero.
	 * Thus, the addresses are 4-Byte's aligned.
	 */
	if (dma_set_mask(dev, DMA_BIT_MASK(36))) {  /* for Large memory or only for whitney ? */
		pr_warn("%s: no dma_mask\n", __func__);
		return -EINVAL;
	}

	/* Initialization of hwzram_impl */
	hwz = hwzram_impl_init(&pdev->dev, ufozip_default_fifo_size,
			       mem->start, irq, ufozip_platform_init, ufozip_clock_control);

	platform_set_drvdata(pdev, hwz);

	if (IS_ERR(hwz)) {
		pr_warn("%s: failed to initialize hwzram_impl\n", __func__);
		return -EINVAL;
	}

	if (fb_register_client(&fb_ufozip_notifier_block) != 0)
		return -EINVAL;

	return 0;
}

static int ufozip_of_remove(struct platform_device *pdev)
{
	struct hwzram_impl *hwz = platform_get_drvdata(pdev);

	hwzram_impl_destroy(hwz);

	return 0;
}

static const struct of_device_id ufozip_of_match[] = {
	{ .compatible = "mediatek,ufozip", },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, ufozip_of_match);

static int enable_ufozip_clock(void)
{
#ifdef	CONFIG_MTK_CLKMGR /*not support CCF, Rushmore only*/
	int err = 0;

	err = enable_clock(MT_CG_UFOZIP_ENC_CLK_SW_CG, "UFOZIP");
	err |= enable_clock(MT_CG_UFOZIP_DEC_CLK_SW_CG, "UFOZIP");
	err |= enable_clock(MT_CG_UFOZIP_HCLK_SW_CG, "UFOZIP");

	return err;
#else
	return 0;
#endif
}

static int disable_ufozip_clock(void)
{
#ifdef	CONFIG_MTK_CLKMGR /*not support CCF, Rushmore only*/
	int err = 0;

	err = disable_clock(MT_CG_UFOZIP_ENC_CLK_SW_CG, "UFOZIP");
	err |= disable_clock(MT_CG_UFOZIP_DEC_CLK_SW_CG, "UFOZIP");
	err |= disable_clock(MT_CG_UFOZIP_HCLK_SW_CG, "UFOZIP");

	return err;
#else
	return 0;
#endif
}

static int mt_ufozip_suspend(struct device *dev)
{
	struct hwzram_impl *hwz = dev_get_drvdata(dev);
#ifdef CONFIG_MTK_CLKMGR
	unsigned long flags;
#endif

	pr_info("%mt_ufozip_suspend\n");

#ifdef CONFIG_MTK_CLKMGR
	spin_lock_irqsave(&hclklock, flags);
	if (hclk_count) {
		spin_unlock_irqrestore(&hclklock, flags);
		pr_warn("%s: UFOZIP is in use.\n", __func__);
		return -1;
	}
	spin_unlock_irqrestore(&hclklock, flags);
#endif

	if (enable_ufozip_clock())
		pr_warn("%s: failed to enable clock\n", __func__);

	hwzram_impl_suspend(hwz);

	spm_set_sleep_ufozip_sram_config(UFO_SRAM_SPM_OFF);	/* SPM ON @ INIT*/

	if (disable_ufozip_clock())
		pr_warn("%s: failed to disable clock\n", __func__);

	return 0;
}

static int mt_ufozip_resume(struct device *dev)
{
	struct hwzram_impl *hwz = dev_get_drvdata(dev);

	pr_info("%mt_ufozip_resume\n");

	if (enable_ufozip_clock())
		pr_warn("%s: failed to enable clock\n", __func__);

	UFOZIP_HwInit(hwz); /* SPM ON @ INIT*/
	hwzram_impl_resume(hwz);

	/* disable clock until actual request is coming */
	if (disable_ufozip_clock())
		pr_warn("%s: failed to disable clock\n", __func__);

	return 0;
}



static const struct dev_pm_ops mt_ufozip_pm_ops = {
	.suspend = mt_ufozip_suspend,
	.resume = mt_ufozip_resume,
	.freeze = mt_ufozip_suspend,
	.thaw = mt_ufozip_resume,
	.restore = mt_ufozip_resume,
};


static struct platform_driver ufozip_of_driver = {
	.probe		= ufozip_of_probe,
	.remove		= ufozip_of_remove,
	.driver		= {
		.name	= "ufozip",
		.pm = &mt_ufozip_pm_ops,
		.of_match_table = of_match_ptr(ufozip_of_match),
	},
};

module_platform_driver(ufozip_of_driver);
#endif

module_param(ufozip_default_fifo_size, uint, 0644);
MODULE_AUTHOR("Mediatek");
MODULE_DESCRIPTION("Hardware Memory compression accelerator UFOZIP");
