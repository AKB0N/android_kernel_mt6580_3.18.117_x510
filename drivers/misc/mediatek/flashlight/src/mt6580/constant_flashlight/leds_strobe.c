/*
 * Copyright (C) 2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/wait.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/poll.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/errno.h>
#include <linux/time.h>
#include "kd_flashlight.h"
#include <asm/io.h>
#include <asm/uaccess.h>
#include "kd_camera_typedef.h"
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/version.h>
#include <linux/mutex.h>
#include <linux/i2c.h>
#include <linux/leds.h>

#include <gpio_const.h>
#include <mt_gpio.h>
/******************************************************************************
 * GPIO configuration new nik-kst
******************************************************************************/
#define GPIO_TORCH_EN         (GPIO4 | 0x80000000)
#define GPIO_TORCH_EN_M_GPIO   GPIO_MODE_00
#define GPIO_TORCH_EN_M_EINT   GPIO_MODE_06
#define GPIO_TORCH_EN_M_PWM   GPIO_MODE_05

#define GPIO_FLASH_LED_EN         (GPIO14 | 0x80000000)
#define GPIO_FLASH_LED_EN_M_GPIO   GPIO_MODE_00
#define GPIO_FLASH_LED_EN_M_EINT   GPIO_MODE_06
/******************************************************************************
 * end new nik-kst
******************************************************************************/

/******************************************************************************
 * Debug configuration
******************************************************************************/
#define TAG_NAME "[leds_strobe.c]"
#define PK_DBG_NONE(fmt, arg...)    do {} while (0)
#define PK_DBG_FUNC(fmt, arg...)    pr_debug(TAG_NAME "%s: " fmt, __func__ , ##arg)
#define PK_ERROR(fmt, arg...)       pr_err(TAG_NAME "%s: " fmt, __FUNCTION__ ,##arg)

/*#define DEBUG_LEDS_STROBE*/
#ifdef DEBUG_LEDS_STROBE
	#define PK_DBG PK_DBG_FUNC
	#define PK_ERR PK_ERROR
#else
	#define PK_DBG(a, ...)
	#define PK_ERR(a,...)
#endif
/******************************************************************************
 * local variables
******************************************************************************/

static DEFINE_SPINLOCK(g_strobeSMPLock);	/* cotta-- SMP proection */


static u32 strobe_Res;
static u32 strobe_Timeus;
static BOOL g_strobe_On;


static int g_timeOutTimeMs;
static BOOL g_is_torch_mode;
static DEFINE_MUTEX(g_strobeSem);



static struct work_struct workTimeOut;

/*****************************************************************************
Functions
*****************************************************************************/
static void work_timeOutFunc(struct work_struct *data);

int FL_Enable(void)
{
	PK_DBG("FL_enable.g_is_torch_mode = %d\n",g_is_torch_mode);	
	if(g_is_torch_mode)
	{
		PK_DBG("wangguan go here.g_is_torch_mode=1\n");
		mt_set_gpio_out(GPIO_TORCH_EN, 1);
		mt_set_gpio_out(GPIO_FLASH_LED_EN , 0);
	}
	else
	{	
		mt_set_gpio_out(GPIO_TORCH_EN, 1);
		mt_set_gpio_out(GPIO_FLASH_LED_EN , 1);

	}
	
    return 0;
}

int FL_Disable(void)
{
	PK_DBG("FL_Disable");
	mt_set_gpio_out(GPIO_FLASH_LED_EN , 0);
	mt_set_gpio_out(GPIO_TORCH_EN, 0);	
    return 0;
}

int FL_dim_duty(kal_uint32 duty)
{
	PK_DBG("FL_dim_duty %d, g_is_torch_mode %d", duty, g_is_torch_mode);
	if(duty == 0)	{
		g_is_torch_mode = 1;		
	}
	else{
		g_is_torch_mode = 0;		
	}
	if((g_timeOutTimeMs == 0) && (duty > 0))
	{
		PK_ERR("FL_dim_duty %d , FLASH mode but timeout %d", duty, g_timeOutTimeMs);	
		g_is_torch_mode = 1;	
	}	
    return 0;
}




int FL_Init(void)
{
	PK_DBG("FL_init");

	mt_set_gpio_mode(GPIO_TORCH_EN, GPIO_TORCH_EN_M_GPIO);
	mt_set_gpio_dir(GPIO_TORCH_EN, GPIO_DIR_OUT);  
	mt_set_gpio_mode(GPIO_FLASH_LED_EN , GPIO_FLASH_LED_EN_M_GPIO );
	mt_set_gpio_dir(GPIO_FLASH_LED_EN , GPIO_DIR_OUT);

	FL_Disable();
	INIT_WORK(&workTimeOut, work_timeOutFunc);
	g_is_torch_mode = 1;
    return 0;
}



int FL_Uninit(void)
{
	FL_Disable();
	g_is_torch_mode = 0;
    return 0;
}

/*****************************************************************************
User interface
*****************************************************************************/

static void work_timeOutFunc(struct work_struct *data)
{
	FL_Disable();
	PK_DBG("ledTimeOut_callback\n");
}

enum hrtimer_restart ledTimeOutCallback(struct hrtimer *timer)
{
	schedule_work(&workTimeOut);
	return HRTIMER_NORESTART;
}

static struct hrtimer g_timeOutTimer;
void timerInit(void)
{
	static int init_flag;

	if (init_flag == 0) {
		init_flag = 1;
		INIT_WORK(&workTimeOut, work_timeOutFunc);
		g_timeOutTimeMs = 1000;
		hrtimer_init(&g_timeOutTimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
		g_timeOutTimer.function = ledTimeOutCallback;
	}
}



static int constant_flashlight_ioctl(unsigned int cmd, unsigned long arg)
{
	int i4RetValue = 0;
	int ior_shift;
	int iow_shift;
	int iowr_shift;

	ior_shift = cmd - (_IOR(FLASHLIGHT_MAGIC, 0, int));
	iow_shift = cmd - (_IOW(FLASHLIGHT_MAGIC, 0, int));
	iowr_shift = cmd - (_IOWR(FLASHLIGHT_MAGIC, 0, int));
/*	PK_DBG
	    ("LM3642 constant_flashlight_ioctl() line=%d ior_shift=%d, iow_shift=%d iowr_shift=%d arg=%d\n",
	     __LINE__, ior_shift, iow_shift, iowr_shift, (int)arg);
*/
	switch (cmd) {

	case FLASH_IOC_SET_TIME_OUT_TIME_MS:
		PK_DBG("FLASH_IOC_SET_TIME_OUT_TIME_MS: %d\n", (int)arg);
		g_timeOutTimeMs = arg;
		break;


	case FLASH_IOC_SET_DUTY:
		PK_DBG("FLASHLIGHT_DUTY: %d\n", (int)arg);
		FL_dim_duty(arg);
		break;


	case FLASH_IOC_SET_STEP:
		PK_DBG("FLASH_IOC_SET_STEP: %d\n", (int)arg);

		break;

	case FLASH_IOC_SET_ONOFF:
		PK_DBG("FLASHLIGHT_ONOFF: %d\n", (int)arg);
		if (arg == 1) {

			int s;
			int ms;

			if (g_timeOutTimeMs > 1000) {
				s = g_timeOutTimeMs / 1000;
				ms = g_timeOutTimeMs - s * 1000;
			} else {
				s = 0;
				ms = g_timeOutTimeMs;
			}

			if (g_timeOutTimeMs != 0) {
				ktime_t ktime;

				ktime = ktime_set(s, ms * 1000000);
				hrtimer_start(&g_timeOutTimer, ktime, HRTIMER_MODE_REL);
			}
			FL_Enable();
		} else {
			FL_Disable();
			hrtimer_cancel(&g_timeOutTimer);
		}
		break;
	default:
		PK_DBG(" No such command\n");
		i4RetValue = -EPERM;
		break;
	}
	return i4RetValue;
}




static int constant_flashlight_open(void *pArg)
{
	int i4RetValue = 0;

	PK_DBG("constant_flashlight_open line=%d\n", __LINE__);

	if (0 == strobe_Res) {
		FL_Init();
		timerInit();
	}
	PK_DBG("constant_flashlight_open line=%d\n", __LINE__);
	spin_lock_irq(&g_strobeSMPLock);


	if (strobe_Res) {
		PK_DBG(" busy!\n");
		i4RetValue = -EBUSY;
	} else {
		strobe_Res += 1;
	}


	spin_unlock_irq(&g_strobeSMPLock);
	PK_DBG("constant_flashlight_open line=%d\n", __LINE__);

	return i4RetValue;

}


static int constant_flashlight_release(void *pArg)
{
	PK_DBG(" constant_flashlight_release\n");

	if (strobe_Res) {
		spin_lock_irq(&g_strobeSMPLock);

		strobe_Res = 0;
		strobe_Timeus = 0;

		/* LED On Status */
		g_strobe_On = FALSE;

		spin_unlock_irq(&g_strobeSMPLock);

		FL_Uninit();
	}

	PK_DBG(" Done\n");

	return 0;

}


FLASHLIGHT_FUNCTION_STRUCT constantFlashlightFunc = {
	constant_flashlight_open,
	constant_flashlight_release,
	constant_flashlight_ioctl
};


MUINT32 constantFlashlightInit(PFLASHLIGHT_FUNCTION_STRUCT *pfFunc)
{
	if (pfFunc != NULL)
		*pfFunc = &constantFlashlightFunc;
	return 0;
}



/* LED flash control for high current capture mode*/
ssize_t strobe_VDIrq(void)
{

	return 0;
}
EXPORT_SYMBOL(strobe_VDIrq);
