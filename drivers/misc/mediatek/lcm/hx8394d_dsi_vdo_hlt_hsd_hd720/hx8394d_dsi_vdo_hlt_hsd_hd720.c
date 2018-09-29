#ifndef BUILD_LK
#include <linux/string.h>
#include <mt_gpio.h>
#else
#include <platform/mt_gpio.h>
#endif

#include "lcm_drv.h"
// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------
#include <mach/cust_adc.h>
#define MIN_VOLTAGE (800)
#define MAX_VOLTAGE (1200)
#define LCM_ID (0x8394)

#define FRAME_WIDTH  										(720)
#define FRAME_HEIGHT 										(1280)

#define REGFLAG_DELAY             							0xFE
#define REGFLAG_END_OF_TABLE      							0xFF   // END OF REGISTERS MARKER

// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static LCM_UTIL_FUNCS lcm_util = {0};
#ifndef BUILD_LK
extern atomic_t ESDCheck_byCPU;
#endif

#define SET_RESET_PIN(v)    (lcm_util.set_reset_pin((v)))

#define UDELAY(n) (lcm_util.udelay(n))
#define MDELAY(n) (lcm_util.mdelay(n))


// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------
#define dsi_set_cmdq_V3(para_tbl,size,force_update)        	lcm_util.dsi_set_cmdq_V3(para_tbl,size,force_update)
#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)	lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)		lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)									lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)				lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg											lcm_util.dsi_read_reg()
#define read_reg_v2(cmd, buffer, buffer_size)			lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)

extern int IMM_GetOneChannelValue(int dwChannel, int data[4], int* rawdata);
// ---------------------------------------------------------------------------
//  LCM Driver Implementations
// ---------------------------------------------------------------------------

static void lcm_set_util_funcs(const LCM_UTIL_FUNCS *util)
{
    memcpy(&lcm_util, util, sizeof(LCM_UTIL_FUNCS));
}


static void lcm_get_params(LCM_PARAMS *params)
{
    memset(params, 0, sizeof(LCM_PARAMS));

    params->type   = LCM_TYPE_DSI;
    params->width  = FRAME_WIDTH;
    params->height = FRAME_HEIGHT;

    // enable tearing-free
    params->dbi.te_mode 				= LCM_DBI_TE_MODE_VSYNC_ONLY;
    params->dbi.te_edge_polarity		= LCM_POLARITY_RISING;
    params->dsi.mode   = SYNC_PULSE_VDO_MODE;

    // DSI
    /* Command mode setting */
    params->dsi.LANE_NUM				= LCM_FOUR_LANE;
    //The following defined the fomat for data coming from LCD engine.
    params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
    params->dsi.data_format.trans_seq   = LCM_DSI_TRANS_SEQ_MSB_FIRST;
    params->dsi.data_format.padding     = LCM_DSI_PADDING_ON_LSB;
    params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;

    // Highly depends on LCD driver capability.
    // Not support in MT6573
    params->dsi.packet_size=256;
    params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;

    params->dsi.vertical_sync_active				= 4;
    params->dsi.vertical_backporch					= 14;
    params->dsi.vertical_frontporch					= 20;
    params->dsi.vertical_active_line				= FRAME_HEIGHT;

    params->dsi.horizontal_sync_active				= 20;
    params->dsi.horizontal_backporch				= 60;
    params->dsi.horizontal_frontporch				= 60;
    params->dsi.horizontal_active_pixel				= FRAME_WIDTH;

	params->dsi.PLL_CLOCK = 218; //this value must be in MTK suggested table
	params->dsi.ssc_disable                         = 1;

    params->dsi.noncont_clock=1;
    params->dsi.noncont_clock_period=2;

	params->dsi.esd_check_enable = 1;
	params->dsi.customization_esd_check_enable = 1;
	params->dsi.lcm_esd_check_table[0].cmd          = 0x0a;
	params->dsi.lcm_esd_check_table[0].count        = 1;
	params->dsi.lcm_esd_check_table[0].para_list[0] = 0x1c;
	
	params->dsi.lcm_esd_check_table[1].cmd          = 0xd9;
	params->dsi.lcm_esd_check_table[1].count        = 1;
	params->dsi.lcm_esd_check_table[1].para_list[0] = 0x80;

	params->dsi.lcm_esd_check_table[2].cmd          = 0x09;
	params->dsi.lcm_esd_check_table[2].count        = 3;
	params->dsi.lcm_esd_check_table[2].para_list[0] = 0x80;
	params->dsi.lcm_esd_check_table[2].para_list[1] = 0x73;
	params->dsi.lcm_esd_check_table[2].para_list[2] = 0x06;

}


static void lcm_init(void)
{ 
	unsigned int data_array[32];
			
		data_array[0] = 0x00043902;
		data_array[1] = 0x9483FFB9;
		dsi_set_cmdq(data_array, 2, 1);
		MDELAY(10);
		 
		data_array[0] = 0x00033902; 						
		data_array[1] = 0x008372BA;
		//data_array[2] = 0x0909b265;
		//data_array[3] = 0x00001040;
		dsi_set_cmdq(data_array, 2, 1);
		MDELAY(3);
		   
		data_array[0] = 0x00103902; 						
		data_array[1] = 0x15156aB1;//7c
		data_array[2] = 0xF1110413;
		data_array[3] = 0x2355ec80;//0x23543A81
		data_array[4] = 0x58D2C080;
		dsi_set_cmdq(data_array, 5, 1);
		MDELAY(5);
		 
		data_array[0] = 0x000C3902; 						
		data_array[1] = 0x106400B2;
		data_array[2] = 0x081C1207;
		data_array[3] = 0x004D1C08;
		dsi_set_cmdq(data_array, 4, 1);
		
		data_array[0] = 0x000D3902; 						
		data_array[1] = 0x03FF00B4;
		data_array[2] = 0x035a035a;//5a
		data_array[3] = 0x016a015a;//
		data_array[4] = 0x0000006a;
		dsi_set_cmdq(data_array, 5, 1);
		
		 
		//0x00,0x00,0x00,0x00,0x0A,0x00,0x01,0x00,0xCC,0x00,0x00,0x00,0x88,0x88,0x88,0x88,0x88,0x88,0x88,0x88,0x88,0x88,0x01,0x67,0x45,0x23,0x01,0x23,0x88,0x88,0x88,0x88}},
		  
		data_array[0] = 0x07BC1500;//			  
		dsi_set_cmdq(data_array, 1, 1);
		 
		 
		data_array[0] = 0x00043902; 						
		data_array[1] = 0x010E41BF;
		dsi_set_cmdq(data_array, 2, 1);
		
		data_array[0] = 0x55D21500; 		 
		dsi_set_cmdq(data_array, 1, 1);
		 
		 
		data_array[0] = 0x001f3902; 						
		data_array[1] = 0x000f00D3;//0f
		data_array[2] = 0x00081a01; 						
		data_array[3] = 0x00071032;
		data_array[4] = 0x0f155407; 						
		data_array[5] = 0x12020405;
		data_array[6] = 0x33070510; 						
		data_array[7] = 0x370b0b33;
		data_array[8] = 0x00070710;
		//data_array[9] = 0x0A000000;	
		//data_array[10] = 0x00000100;					  
		dsi_set_cmdq(data_array, 9, 1);
		
		   
		data_array[0] = 0x002D3902; 						
		data_array[1] = 0x181919D5;
		data_array[2] = 0x1a1b1b18;
		data_array[3] = 0x0605041a;
		data_array[4] = 0x02010007;
		data_array[5] = 0x18212003;
		data_array[6] = 0x18232218;
		data_array[7] = 0x18181818;
		data_array[8] = 0x18181818;
		data_array[9] = 0x18181818;
		data_array[10] = 0x18181818;
		data_array[11] = 0x18181818;
		data_array[12] = 0x00000018;
		dsi_set_cmdq(data_array, 13, 1);
		
		 
		data_array[0] = 0x002D3902; 						
		data_array[1] = 0x191818D6;
		data_array[2] = 0x1A1B1B19;
		data_array[3] = 0x0102031A;
		data_array[4] = 0x05060700;
		data_array[5] = 0x18222304;
		data_array[6] = 0x18202118;
		data_array[7] = 0x18181818;
		data_array[8] = 0x18181818;
		data_array[9] = 0x18181818;
		data_array[10] = 0x18181818;
		data_array[11] = 0x18181818;
		data_array[12] = 0x00000018;
		dsi_set_cmdq(data_array, 13, 1);
			  
		data_array[0] = 0x002B3902;						  
		data_array[1] = 0x1B1812E0;
		data_array[2] = 0x1E3f332D;//1d
		data_array[3] = 0x0B09053B;
		data_array[4] = 0x12100D16;
		data_array[5] = 0x10061211;
		data_array[6] = 0x18121612;
		data_array[7] = 0x3f332d1B;
		data_array[8] = 0x09053B1E;
		data_array[9] = 0x100D160B;
		data_array[10] = 0x06121112;
		data_array[11] = 0x00161210;
		dsi_set_cmdq(data_array, 12, 1);

		data_array[0] = 0x09CC1500;  
		dsi_set_cmdq(data_array, 1, 1);
		
		data_array[0] = 0x00033902; 						
		data_array[1] = 0x001430C0;
		dsi_set_cmdq(data_array, 2, 1);
		
		   
		data_array[0] = 0x00053902; 						
		data_array[1] = 0x00C000C7;
		data_array[2] = 0x000000C0;
		dsi_set_cmdq(data_array, 3, 1);
		
		 
		data_array[0] = 0x00033902; 						
		data_array[1] = 0x006D6DB6;//7c-85
		dsi_set_cmdq(data_array, 2, 1);
		
		data_array[0] = 0x88DF1500; 		   
		dsi_set_cmdq(data_array, 1, 1);
		
		data_array[0] = 0x00351500;//			  
		dsi_set_cmdq(data_array, 1, 1);
		
		 
		data_array[0] = 0x00110500;   
		dsi_set_cmdq(data_array, 1, 1);
		MDELAY(120);
		
		data_array[0] = 0x00290500;   
		dsi_set_cmdq(data_array, 1, 1);
		MDELAY(10);
}


static void lcm_suspend(void)
{
	SET_RESET_PIN(1);
	MDELAY(20);
	SET_RESET_PIN(0);
	MDELAY(50);
	
	SET_RESET_PIN(1);
	MDELAY(20);
}


static void lcm_resume(void)
{
    lcm_init();
}

static unsigned int lcm_compare_id(void)
{
	unsigned int id=0;
	unsigned char buffer[4];
	unsigned int array[16];  

		SET_RESET_PIN(1);
		SET_RESET_PIN(0);
		MDELAY(10);
		SET_RESET_PIN(1);
		MDELAY(20);

		array[0] = 0x00043902;
		array[1] = 0x9483FFB9;
		dsi_set_cmdq(array, 2, 1);
		MDELAY(10);
		 
		array[0] = 0x00033902; 						
		array[1] = 0x008372BA;
		//data_array[2] = 0x0909b265;
		//data_array[3] = 0x00001040;
		dsi_set_cmdq(array, 2, 1);
		MDELAY(3);	
	
	array[0] = 0x00043700;// read id return two byte,version and id
	dsi_set_cmdq(array, 1, 1);
	
	read_reg_v2(0x04, buffer, 4);
	id = buffer[0]<<8|buffer[1]; //we only need ID
#ifdef BUILD_LK
		printf("hx8394 uboot %s,buffer=%x,%x,%x,%x\n", __func__,buffer[0],buffer[1],buffer[2],id);
		printf("%s id = 0x%08x \n", __func__, id);
#else
		printk("hx8394 kernel %s,buffer=%x,%x,%x,%x\n", __func__,buffer[0],buffer[1],buffer[2],id);
		printk("%s id = 0x%08x \n", __func__, id);
#endif
	   
	return (id == LCM_ID) ? 1 : 0;


}

static unsigned int rgk_lcm_compare_id(void)
{
    int data[4] = {0,0,0,0};
    int lcm_vol = 0;

    lcm_vol = data[0]*1000+data[1]*10;

#ifdef BUILD_LK
	printf("[adc_uboot]: lcm_vol= %d\n",lcm_vol);
#else
	printk("[adc_kernel]: lcm_vol= %d\n",lcm_vol);
#endif
    if (lcm_vol>=MIN_VOLTAGE &&lcm_vol <= MAX_VOLTAGE && lcm_compare_id())
    {
		return 1;
    }

    return 0;

}

static unsigned int lcm_ata_check(unsigned char *buffer)
{
#ifndef BUILD_LK
			unsigned int id=0;
			unsigned char buf[4];
			unsigned int array[16];  
			
		array[0] = 0x00043902;
		array[1] = 0x9483FFB9;
		dsi_set_cmdq(array, 2, 1);
		MDELAY(10);		 	
			
			array[0] = 0x00043700;// read id return two byte,version and id
			dsi_set_cmdq(array, 1, 1);
			atomic_set(&ESDCheck_byCPU,1);
			read_reg_v2(0x04, buf, 4);
			atomic_set(&ESDCheck_byCPU,0);
			id = buf[0]<<8|buf[1]; //we only need ID
#ifdef BUILD_LK
				printf("hx8394 uboot %s,buffer=%x,%x,%x,%x\n", __func__,buf[0],buf[1],buf[2],id);
				printf("%s id = 0x%08x \n", __func__, id);
#else
				printk("hx8394 kernel %s,buffer=%x,%x,%x,%x\n", __func__,buf[0],buf[1],buf[2],id);
				printk("%s id = 0x%08x \n", __func__, id);
#endif
			   
			return (id == LCM_ID) ? 1 : 0;

#else
	return 0;
#endif
}

LCM_DRIVER hx8394d_dsi_vdo_hlt_hsd_hd720_lcm_drv = 
{
    .name			= "hx8394d_dsi_vdo_hlt_hsd_hd720",
    .set_util_funcs = lcm_set_util_funcs,
    .get_params     = lcm_get_params,
    .init           = lcm_init,
    .suspend        = lcm_suspend,
    .resume         = lcm_resume,
	.compare_id		= rgk_lcm_compare_id,
	.ata_check		= lcm_ata_check,
};