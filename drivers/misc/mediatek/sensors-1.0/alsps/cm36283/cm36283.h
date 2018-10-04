/*
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
/*
 * Definitions for CM36283 als/ps sensor chip.
 */
#ifndef __CM36283_H__
#define __CM36283_H__

#include <linux/ioctl.h>

/*cm36283 als/ps sensor register related macro*/
#define CM36283_REG_CS_CONF		0X00
#define CM36283_REG_ALS_THDH		0X01
#define CM36283_REG_ALS_THDL		0X02
#define CM36283_REG_PS_CONF1_2		0X03
#define CM36283_REG_PS_CONF3_MS		0X04
#define CM36283_REG_PS_THD			0X05
#define CM36283_REG_PS_CANC			0X06
#define CM36283_REG_PS_DATA			0X07
#define CM36283_REG_ALS_DATA		0X09
#define CM36283_REG_INT_FLAG		0X0B
#define CM36283_REG_ID_MODE			0X0C

/*CM36283 related driver tag macro*/
#define CM36283_SUCCESS						 0
#define CM36283_ERR_I2C						-1
#define CM36283_ERR_STATUS					-3
#define CM36283_ERR_SETUP_FAILURE			-4
#define CM36283_ERR_GETGSENSORDATA			-5
#define CM36283_ERR_IDENTIFICATION			-6

/*----------------------------------------------------------------------------*/
enum CM36283_NOTIFY_TYPE {
	CM36283_NOTIFY_PROXIMITY_CHANGE = 1,
	CM36283_NOTIFY_ALS_RAW_DATA,
	CM36283_NOTIFY_PS_RAW_DATA,
	CM36283_NOTIFY_PROXIMITY_NOT_CHANGE
};
/*----------------------------------------------------------------------------*/
enum CM36283_CUST_ACTION {
	CM36283_CUST_ACTION_SET_CUST = 1,
	CM36283_CUST_ACTION_CLR_CALI,
	CM36283_CUST_ACTION_SET_CALI,
	CM36283_CUST_ACTION_SET_PS_THRESHODL,
	CM36283_CUST_ACTION_SET_EINT_INFO,
	CM36283_CUST_ACTION_GET_ALS_RAW_DATA,
	CM36283_CUST_ACTION_GET_PS_RAW_DATA,
};
/*----------------------------------------------------------------------------*/
struct CM36283_CUST {
	uint16_t	action;
};
/*----------------------------------------------------------------------------*/
struct CM36283_SET_CUST {
	uint16_t action;
	uint16_t part;
	int32_t	data[0];
};
/*----------------------------------------------------------------------------*/
struct CM36283_SET_CALI {
	uint16_t	action;
	int32_t	 cali;
};
/*----------------------------------------------------------------------------*/
struct CM36283_SET_PS_THRESHOLD {
	uint16_t	action;
	int32_t	 threshold[2];
};
/*----------------------------------------------------------------------------*/
struct CM36283_SET_EINT_INFO {
	uint16_t	action;
	uint32_t	gpio_pin;
	uint32_t	gpio_mode;
	uint32_t	eint_num;
	uint32_t	eint_is_deb_en;
	uint32_t	eint_type;
};
/*----------------------------------------------------------------------------*/
struct CM36283_GET_ALS_RAW_DATA {
	uint16_t	action;
	uint16_t	als;
};
/*----------------------------------------------------------------------------*/
struct CM36283_GET_PS_RAW_DATA {
	uint16_t	action;
	uint16_t	ps;
};
/*----------------------------------------------------------------------------*/
union CM36283_CUST_DATA {
	uint32_t					data[10];
	struct CM36283_CUST				cust;
	struct CM36283_SET_CUST			setCust;
	struct CM36283_CUST			clearCali;
	struct CM36283_SET_CALI			setCali;
	struct CM36283_SET_PS_THRESHOLD	setPSThreshold;
	struct CM36283_SET_EINT_INFO	   setEintInfo;
	struct CM36283_GET_ALS_RAW_DATA	getALSRawData;
	struct CM36283_GET_PS_RAW_DATA	 getPSRawData;
};
/*----------------------------------------------------------------------------*/

extern struct platform_device *get_alsps_platformdev(void);
#ifdef CONFIG_CUSTOM_KERNEL_ALSPS_MODULE
extern bool alsps_success_Flag;
#endif

#endif