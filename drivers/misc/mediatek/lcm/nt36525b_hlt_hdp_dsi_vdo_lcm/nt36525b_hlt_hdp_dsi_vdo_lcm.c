/***********************************************************
** Copyright (C), 2008-2016, OPPO Mobile Comm Corp., Ltd.
** ODM_WT_EDIT
** File: - nt36525b_hlt_hdp_dsi_vdo_lcm.c
** Description: source file for lcm nt36525b+hlt in kernel stage
**
** Version: 1.0
** Date : 2019/9/25
**
** ------------------------------- Revision History: -------------------------------
**  	<author>		<data> 	   <version >	       <desc>
**  lianghao       2019/9/25     1.0     source file for lcm nt36525b+hlt in kernel stage
**
****************************************************************/

#define LOG_TAG "LCM"

#ifndef BUILD_LK
#include <linux/string.h>
#include <linux/kernel.h>
#endif

#include "lcm_drv.h"

#ifdef BUILD_LK
#include <platform/upmu_common.h>
#include <platform/mt_gpio.h>
#include <platform/mt_i2c.h>
#include <platform/mt_pmic.h>
#include <string.h>
#elif defined(BUILD_UBOOT)
#include <asm/arch/mt_gpio.h>
#else
#include "disp_dts_gpio.h"
#endif

#ifdef ODM_WT_EDIT
#include <linux/update_tpfw_notifier.h>
#endif
#include "disp_cust.h"
#include <soc/oppo/device_info.h>
static struct LCM_UTIL_FUNCS lcm_util;

#define SET_RESET_PIN(v)	(lcm_util.set_reset_pin((v)))
#define SET_LCM_VDD18_PIN(v)	(lcm_util.set_gpio_lcm_vddio_ctl((v)))
#define SET_LCM_VSP_PIN(v)	(lcm_util.set_gpio_lcd_enp_bias((v)))
#define SET_LCM_VSN_PIN(v)	(lcm_util.set_gpio_lcd_enn_bias((v)))
#define MDELAY(n)		(lcm_util.mdelay(n))
#define UDELAY(n)		(lcm_util.udelay(n))

#define dsi_set_cmdq_V22(cmdq, cmd, count, ppara, force_update) \
	lcm_util.dsi_set_cmdq_V22(cmdq, cmd, count, ppara, force_update)
#define dsi_set_cmdq_V2(cmd, count, ppara, force_update) \
	lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update) \
		lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd) lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums) \
		lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg(cmd) \
	  lcm_util.dsi_dcs_read_lcm_reg(cmd)
#define read_reg_v2(cmd, buffer, buffer_size) \
		lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)

#ifndef BUILD_LK
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/i2c.h>
#include <linux/irq.h>
/* #include <linux/jiffies.h> */
/* #include <linux/delay.h> */
#include <linux/uaccess.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#endif

#define LCM_DSI_CMD_MODE									0
#define FRAME_WIDTH										(720)
#define FRAME_HEIGHT									(1600)

#define LCM_PHYSICAL_WIDTH									(68000)
#define LCM_PHYSICAL_HEIGHT									(151000)

#define REGFLAG_DELAY		0xFFFC
#define REGFLAG_UDELAY	0xFFFB
#define REGFLAG_END_OF_TABLE	0xFFFD
#define REGFLAG_RESET_LOW	0xFFFE
#define REGFLAG_RESET_HIGH	0xFFFF

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifdef ODM_WT_EDIT
extern void lcd_resume_load_ili_fw(void);
extern void core_config_sleep_ctrl(bool out);
#endif /* ODM_WT_EDIT */

struct LCM_setting_table {
	unsigned int cmd;
	unsigned char count;
	unsigned char para_list[64];
};

//#ifdef ODM_WT_EDIT
static struct LCM_setting_table lcm_suspend_setting[] = {
	{0x28, 0, {} },
	{REGFLAG_DELAY, 20, {} },
	{0x10, 0, {} },
	{REGFLAG_DELAY, 80, {} }
};
//#endif

static int blmap_table[] = {
					36, 4,
					16, 11,
					17, 12,
					19, 13,
					19, 15,
					20, 14,
					22, 14,
					22, 14,
					24, 10,
					24, 8 ,
					26, 4 ,
					27, 0 ,
					29, 9 ,
					29, 9 ,
					30, 14,
					33, 25,
					34, 30,
					36, 44,
					37, 49,
					40, 65,
					40, 69,
					43, 88,
					46, 109,
					47, 112,
					50, 135,
					53, 161,
					53, 163,
					60, 220,
					60, 223,
					64, 257,
					63, 255,
					71, 334,
					71, 331,
					75, 375,
					80, 422,
					84, 473,
					89, 529,
					88, 518,
					99, 653,
					98, 640,
					103, 707,
					117, 878,
					115, 862,
					122, 947,
					128, 1039,
					135, 1138,
					132, 1102,
					149, 1355,
					157, 1478,
					166, 1611,
					163, 1563,
					183, 1900,
					180, 1844,
					203, 2232,
					199, 2169,
					209, 2344,
					236, 2821,
					232, 2742,
					243, 2958,
					255, 3188,
					268, 3433,
					282, 3705,
					317, 4400,
					176, 1555};

static struct LCM_setting_table init_setting_cmd[] = {
	{ 0xFF, 0x03, {0x98, 0x81, 0x03} },
};

static struct LCM_setting_table init_setting_vdo[] = {

//***********************************gamma code**************************************//
	{0xFF,0x01,{0x20}},
	{0xFB,0x01,{0x01}},
	//R(+)
	{0xB0,0x10,{0x00,0x8F,0x00,0x97,0x00,0xA3,0x00,0xB2,0x00,0xBE,0x00,0xC9,0x00,0xD5,0x00,0xE0}},
	{0xB1,0x10,{0x00,0xEA,0x01,0x0E,0x01,0x2D,0x01,0x60,0x01,0x8A,0x01,0xCF,0x02,0x0C,0x02,0x0E}},
	{0xB2,0x10,{0x02,0x4A,0x02,0x8E,0x02,0xBB,0x02,0xF3,0x03,0x18,0x03,0x45,0x03,0x53,0x03,0x63}},
	{0xB3,0x0C,{0x03,0x75,0x03,0x8C,0x03,0xA7,0x03,0xC5,0x03,0xDC,0x03,0xDF}},
	//G(+)
	{0xB4,0x10,{0x00,0x00,0x00,0x13,0x00,0x2E,0x00,0x44,0x00,0x59,0x00,0x6C,0x00,0x7D,0x00,0x8D}},
	{0xB5,0x10,{0x00,0x9C,0x00,0xD0,0x00,0xFA,0x01,0x3B,0x01,0x6E,0x01,0xBC,0x01,0xFD,0x01,0xFF}},
	{0xB6,0x10,{0x02,0x3D,0x02,0x84,0x02,0xB1,0x02,0xEA,0x03,0x10,0x03,0x3E,0x03,0x4D,0x03,0x5D}},
	{0xB7,0x0C,{0x03,0x6F,0x03,0x85,0x03,0xA2,0x03,0xC2,0x03,0xDB,0x03,0xDF}},
	//B(+)
	{0xB8,0x10,{0x00,0x00,0x00,0x14,0x00,0x32,0x00,0x49,0x00,0x5B,0x00,0x6F,0x00,0x81,0x00,0x92}},
	{0xB9,0x10,{0x00,0xA3,0x00,0xD7,0x01,0x00,0x01,0x41,0x01,0x72,0x01,0xBF,0x02,0x00,0x02,0x02}},
	{0xBA,0x10,{0x02,0x40,0x02,0x86,0x02,0xB4,0x02,0xEE,0x03,0x17,0x03,0x50,0x03,0x53,0x03,0x65}},
	{0xBB,0x0C,{0x03,0x73,0x03,0x8F,0x03,0xAA,0x03,0xC8,0x03,0xDC,0x03,0xDF}},
	//CMD2_Page1
	{0xFF,0x01,{0x21}},
	{0xFB,0x01,{0x01}},
	//R(-)
	{0xB0,0x10,{0x00,0x87,0x00,0x8F,0x00,0x9B,0x00,0xAA,0x00,0xB6,0x00,0xC1,0x00,0xCD,0x00,0xD8}},
	{0xB1,0x10,{0x00,0xE2,0x01,0x06,0x01,0x25,0x01,0x58,0x01,0x82,0x01,0xC7,0x02,0x04,0x02,0x06}},
	{0xB2,0x10,{0x02,0x42,0x02,0x86,0x02,0xB3,0x02,0xEB,0x03,0x10,0x03,0x3D,0x03,0x4B,0x03,0x5B}},
	{0xB3,0x0C,{0x03,0x6D,0x03,0x84,0x03,0x9F,0x03,0xBD,0x03,0xD4,0x03,0xD7}},
	//G(-)
	{0xB4,0x10,{0x00,0x00,0x00,0x0B,0x00,0x26,0x00,0x3C,0x00,0x51,0x00,0x64,0x00,0x75,0x00,0x85}},
	{0xB5,0x10,{0x00,0x94,0x00,0xC8,0x00,0xF2,0x01,0x33,0x01,0x66,0x01,0xB4,0x01,0xF5,0x01,0xF7}},
	{0xB6,0x10,{0x02,0x35,0x02,0x7C,0x02,0xA9,0x02,0xE2,0x03,0x08,0x03,0x36,0x03,0x45,0x03,0x55}},
	{0xB7,0x0C,{0x03,0x67,0x03,0x7D,0x03,0x9A,0x03,0xBA,0x03,0xD3,0x03,0xD7}},
	//B(-)
	{0xB8,0x10,{0x00,0x00,0x00,0x0C,0x00,0x2A,0x00,0x41,0x00,0x53,0x00,0x67,0x00,0x79,0x00,0x8A}},
	{0xB9,0x10,{0x00,0x9B,0x00,0xCF,0x00,0xF8,0x01,0x39,0x01,0x6A,0x01,0xB7,0x01,0xF8,0x01,0xFA}},
	{0xBA,0x10,{0x02,0x38,0x02,0x7E,0x02,0xAC,0x02,0xE6,0x03,0x0F,0x03,0x48,0x03,0x4B,0x03,0x5D}},
	{0xBB,0x0C,{0x03,0x6B,0x03,0x87,0x03,0xA2,0x03,0xC0,0x03,0xD4,0x03,0xD7}},

//***********************************gamma code end**************************************//
	//CABC Setting
	{0xFF,0x01,{0x23}},
	{REGFLAG_DELAY, 1, {} },
	{0xFB,0x01,{0x01}},
	//APL_WT
	{0x00,0x01,{0x68}},
	{0x07,0x01,{0x00}},
	{0x08,0x01,{0x01}},
	{0x10,0x01,{0x80}},
	//APL_THD
	{0x11,0x01,{0x00}},
	{0x12,0x01,{0xB4}},
	//APL_COMP
	{0x15,0x01,{0xE9}},
	{0x16,0x01,{0x0B}},
	//GAMMACMP
	{0x29,0x01,{0x20}},
	{0x2A,0x01,{0x20}},
	{0x2B,0x01,{0x20}},
	//CABC_PWM_UI
	{0x30,0x01,{0xF8}},
	{0x31,0x01,{0xF6}},
	{0x32,0x01,{0xF4}},
	{0x33,0x01,{0xF4}},
	{0x34,0x01,{0xF3}},
	{0x35,0x01,{0xF2}},
	{0x36,0x01,{0xF2}},
	{0x37,0x01,{0xF2}},
	{0x38,0x01,{0xF1}},
	{0x39,0x01,{0xF0}},
	{0x3A,0x01,{0xEE}},
	{0x3B,0x01,{0xEC}},
	{0x3D,0x01,{0xEA}},
	{0x3F,0x01,{0xE8}},
	{0x40,0x01,{0xE7}},
	{0x41,0x01,{0xE5}},
	//CABC_PWM_STILL
	{0x45,0x01,{0xE5}},
	{0x46,0x01,{0xE2}},
	{0x47,0x01,{0xDC}},
	{0x48,0x01,{0xD6}},
	{0x49,0x01,{0xD0}},
	{0x4A,0x01,{0xCC}},
	{0x4B,0x01,{0xC8}},
	{0x4C,0x01,{0xC4}},
	{0x4D,0x01,{0xBF}},
	{0x4E,0x01,{0xBF}},
	{0x4F,0x01,{0xBF}},
	{0x50,0x01,{0xBF}},
	{0x51,0x01,{0xBF}},
	{0x52,0x01,{0xBF}},
	{0x53,0x01,{0xBF}},
	{0x54,0x01,{0xBF}},
	//CABC_PWM_MOV
	{0x58,0x01,{0xD9}},
	{0x59,0x01,{0xD6}},
	{0x5A,0x01,{0xD2}},
	{0x5B,0x01,{0xCC}},
	{0x5C,0x01,{0xC4}},
	{0x5D,0x01,{0xBC}},
	{0x5E,0x01,{0xB2}},
	{0x5F,0x01,{0xA8}},
	{0x60,0x01,{0xA4}},
	{0x61,0x01,{0x9C}},
	{0x62,0x01,{0x94}},
	{0x63,0x01,{0x8C}},
	{0x64,0x01,{0x84}},
	{0x65,0x01,{0x7C}},
	{0x66,0x01,{0x76}},
	{0x67,0x01,{0x72}},
	{0x04,0x01,{0x05}},
	{0x05,0x01,{0x32}},
	{0x06,0x01,{0x02}},
	{0xFF,0x01,{0x10}},
	{0xFB,0x01,{0x01}},
	{0xBA,0x01,{0x02}},
	{0x68,0x02,{0x04,0x01}},
	{0x53,0x01,{0x24}},
	{0x35,0x01,{0x00}},
	{0x29,0x01,{0x00}},
	{REGFLAG_DELAY, 10, {} },
	{0x11,0x01,{0x00}},
	{REGFLAG_DELAY, 100, {} },
};



static void push_table(void *cmdq, struct LCM_setting_table *table,
	unsigned int count, unsigned char force_update)
{
	unsigned int i;
	unsigned cmd;

	for (i = 0; i < count; i++) {
		cmd = table[i].cmd;

		switch (cmd) {

		case REGFLAG_DELAY:
			if (table[i].count <= 10)
				MDELAY(table[i].count);
			else
				MDELAY(table[i].count);
			break;

		case REGFLAG_UDELAY:
			UDELAY(table[i].count);
			break;

		case REGFLAG_END_OF_TABLE:
			break;

		default:
			dsi_set_cmdq_V22(cmdq, cmd, table[i].count, table[i].para_list, force_update);
		}
	}
}

static void lcm_set_util_funcs(const struct LCM_UTIL_FUNCS *util)
{
	memcpy(&lcm_util, util, sizeof(struct LCM_UTIL_FUNCS));
}

static void lcm_get_params(struct LCM_PARAMS *params)
{
	memset(params, 0, sizeof(struct LCM_PARAMS));
	params->type = LCM_TYPE_DSI;
	params->width = FRAME_WIDTH;
	params->height = FRAME_HEIGHT;
	params->physical_width = LCM_PHYSICAL_WIDTH/1000;
	params->physical_height = LCM_PHYSICAL_HEIGHT/1000;
	params->physical_width_um = LCM_PHYSICAL_WIDTH;
	params->physical_height_um = LCM_PHYSICAL_HEIGHT;
#if (LCM_DSI_CMD_MODE)
	params->dsi.mode = CMD_MODE;
	params->dsi.switch_mode = SYNC_PULSE_VDO_MODE;
	lcm_dsi_mode = CMD_MODE;
#else
	params->dsi.mode = SYNC_PULSE_VDO_MODE;
	params->dsi.switch_mode = CMD_MODE;
	lcm_dsi_mode = SYNC_PULSE_VDO_MODE;
#endif
	pr_debug("lcm_get_params lcm_dsi_mode %d\n", lcm_dsi_mode);
	params->dsi.switch_mode_enable = 0;
	/* DSI */
	/* Command mode setting */
	params->dsi.LANE_NUM = LCM_THREE_LANE;
	/* The following defined the fomat for data coming from LCD engine. */
	params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
	params->dsi.data_format.trans_seq = LCM_DSI_TRANS_SEQ_MSB_FIRST;
	params->dsi.data_format.padding = LCM_DSI_PADDING_ON_LSB;
	params->dsi.data_format.format = LCM_DSI_FORMAT_RGB888;

	/* Highly depends on LCD driver capability. */
	params->dsi.packet_size = 256;
	/* video mode timing */
	params->dsi.PS = LCM_PACKED_PS_24BIT_RGB888;
	params->dsi.vertical_sync_active = 2;
	params->dsi.vertical_backporch = 254;
	params->dsi.vertical_frontporch = 10;
	//params->dsi.vertical_frontporch_for_low_power = 540;
	params->dsi.vertical_active_line = FRAME_HEIGHT;
	params->dsi.horizontal_sync_active = 4;
	params->dsi.horizontal_backporch = 44;
	params->dsi.horizontal_frontporch = 8;
	params->dsi.horizontal_active_pixel = FRAME_WIDTH;
	params->dsi.ssc_disable = 1;
	//params->dsi.HS_TRAIL = 6;
	//params->dsi.HS_PRPR = 5;
	params->dsi.CLK_HS_PRPR = 7;
	// jump pll_clk
		params->dsi.horizontal_sync_active_ext = 4;
		params->dsi.horizontal_backporch_ext = 30;

#ifndef CONFIG_FPGA_EARLY_PORTING
#if (LCM_DSI_CMD_MODE)
	params->dsi.PLL_CLOCK = 360;	/* this value must be in MTK suggested table */
#else
	params->dsi.data_rate= 733;	/* this value must be in MTK suggested table */
#endif
	params->dsi.PLL_CK_CMD = 360;
	params->dsi.PLL_CK_VDO = 360;
#else
	params->dsi.pll_div1 = 0;
	params->dsi.pll_div2 = 0;
	params->dsi.fbk_div = 0x1;
#endif
	//params->dsi.clk_lp_per_line_enable = 0;
	params->dsi.esd_check_enable = 1;
	params->dsi.customization_esd_check_enable = 1;
	params->dsi.lcm_esd_check_table[0].cmd = 0x0A;
	params->dsi.lcm_esd_check_table[0].count = 1;
	params->dsi.lcm_esd_check_table[0].para_list[0] = 0x9C;

#ifdef CONFIG_MTK_ROUND_CORNER_SUPPORT
	params->round_corner_en = 1;
	params->full_content = 1;
	params->corner_pattern_width = 720;
	params->corner_pattern_height = 75;
	params->corner_pattern_height_bot = 75;
#endif
	params->blmap = blmap_table;
	params->blmap_size = sizeof(blmap_table)/sizeof(blmap_table[0]);
	params->brightness_max = 2012;
	params->brightness_min = 1;
	register_device_proc("lcd", "nt36525b", "hlt_novtek");
}

static void lcm_init_power(void)
{
	/*pr_debug("lcm_init_power\n");*/
	pr_debug("lcm_init_power\n");
	MDELAY(1);
	SET_LCM_VSP_PIN(1);
	MDELAY(3);
	SET_LCM_VSN_PIN(1);
}
extern void lcd_queue_load_tp_fw(void);
extern int tp_gesture_enable_flag(void);
static void lcm_suspend_power(void)
{
	if (0 == tp_gesture_enable_flag()) {
		pr_debug("lcm_suspend_power\n");
		SET_LCM_VSN_PIN(0);
		MDELAY(2);
		SET_LCM_VSP_PIN(0);
	}
}

static void lcm_resume_power(void)
{
	pr_debug("lcm_resume_power\n");
	SET_LCM_VSP_PIN(1);
	MDELAY(3);
	SET_LCM_VSN_PIN(1);
	//base voltage = 4.0 each step = 100mV; 4.0+20 * 0.1 = 6.0v;
//	if ( display_bias_setting(0x14) )
//		pr_err("fatal error: lcd gate ic setting failed \n");
	MDELAY(10);
	SET_RESET_PIN(0);
	MDELAY(10);
	SET_RESET_PIN(1);
	MDELAY(10);
}

#if 0
static struct LCM_setting_table lcm_cabc_enter_setting[] = {
	{0x53,1,{0x2c}},
	{0x55,1,{0x01}},
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};

static struct LCM_setting_table lcm_cabc_exit_setting[] = {
	{0x53,1,{0x2c}},
	{0x55,1,{0x00}},
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};
#endif
static struct LCM_setting_table set_cabc[] = {
	{ 0x53, 0x01, {0x2C} },
	{REGFLAG_DELAY, 20, {} },
	{ 0x55, 0x01, {0x00} },
	{REGFLAG_DELAY, 100, {} },
	//{ 0x53, 0x01, {0x24} },
	{ REGFLAG_END_OF_TABLE, 0x00, {} }
};


static int cabc_status;
 static void lcm_set_cabc_cmdq(void *handle, unsigned int level){
	pr_debug("[lcm] cabc set level %d\n", level);
	if (level==0){
		set_cabc[2].para_list[0]=0x00;
		push_table(handle, set_cabc, sizeof(set_cabc) / sizeof(struct LCM_setting_table), 1);
		pr_err("[lcm] cabc set disable, set_cabc[0x%2x]=%x \n",set_cabc[2].cmd,set_cabc[2].para_list[0]);
	}else if (level == 1){
		set_cabc[2].para_list[0]=0x01;
		push_table(handle, set_cabc, sizeof(set_cabc) / sizeof(struct LCM_setting_table), 1);
		pr_err("[lcm] cabc set enable, set_cabc[0x%2x]=%x \n",set_cabc[2].cmd,set_cabc[2].para_list[0]);
	}else if(level==2){
		set_cabc[2].para_list[0]=0x02;
		push_table(handle,set_cabc,sizeof(set_cabc)/sizeof(struct LCM_setting_table),1);
		pr_err("[lcm] cabc set enable, set_cabc[0x%2x]=%x \n",set_cabc[2].cmd,set_cabc[2].para_list[0]);
	}else if(level==3){
		set_cabc[2].para_list[0]=0x03;
		push_table(handle,set_cabc,sizeof(set_cabc)/sizeof(struct LCM_setting_table),1);
		pr_err("[lcm] cabc set enable, set_cabc[0x%2x]=%x \n",set_cabc[2].cmd,set_cabc[2].para_list[0]);
	}else{
			pr_err("[lcm]  level %d is not support\n", level);
	}
	cabc_status = level;
}

static void lcm_init(void)
{
	pr_debug("lcm_init\n");
	SET_RESET_PIN(0);
	MDELAY(10);
	SET_RESET_PIN(1);

	MDELAY(11);
	lcd_queue_load_tp_fw();
	MDELAY(1);

	if (lcm_dsi_mode == CMD_MODE) {
		push_table(NULL, init_setting_cmd, sizeof(init_setting_cmd) / sizeof(struct LCM_setting_table), 1);
		pr_debug("nt36525b_hlt_lcm_mode = cmd mode :%d----\n", lcm_dsi_mode);
	} else {
		push_table(NULL, init_setting_vdo, sizeof(init_setting_vdo) / sizeof(struct LCM_setting_table), 1);
		pr_debug("nt36525b_hlt_lcm_mode = vdo mode :%d\n", lcm_dsi_mode);
	}
	lcm_set_cabc_cmdq(NULL, cabc_status);
}

static void lcm_suspend(void)
{
	pr_debug("lcm_suspend\n");
	push_table(NULL, lcm_suspend_setting, sizeof(lcm_suspend_setting) / sizeof(struct LCM_setting_table), 1);
}

static void lcm_resume(void)
{
	pr_debug("lcm_resume\n");
	lcm_init();
}

 static void lcm_get_cabc_status(int *status){
	pr_err("[lcm] cabc get to %d\n", cabc_status);
	*status = cabc_status;
}
/*
static void push_table_cust(void *cmdq, struct LCM_setting_table_V3*table,
	unsigned int count, bool hs)
{
	set_lcm(table, count, hs);
}

static struct LCM_setting_table_V3 bl_level[] = {
	{0x39,0x51, 2, {0x04, 0x00} }
};
*/
static struct LCM_setting_table bl_level[] = {
	{0x51, 2, {0x0F,0xFF} },
	{REGFLAG_END_OF_TABLE, 0x00, {} }
};
static void lcm_setbacklight_cmdq(void *handle, unsigned int level)
{
	pr_debug(" lcm nt36525b_hlt backlight_level = %d\n", level);
	bl_level[0].para_list[0] = 0x0007&(level >> 8);
	bl_level[0].para_list[1] = 0x00FF&(level);
	MDELAY(5);
	pr_err("[ HW check backlight nt36525_hlt]level=%d  para_list[0]=%x,para_list[1]=%x\n",level,bl_level[0].para_list[0],bl_level[0].para_list[1]);
	push_table(handle, bl_level, sizeof(bl_level) / sizeof(struct LCM_setting_table), 1);
	//push_table_cust(handle, bl_level, sizeof(bl_level) / sizeof(struct LCM_setting_table_V3), 1);
	//dump_stack();
}

static unsigned int lcm_esd_recover(void)
{
#ifndef BUILD_LK
	lcm_resume_power();
	SET_RESET_PIN(1);
	MDELAY(10);
	SET_RESET_PIN(0);
	MDELAY(10);
	SET_RESET_PIN(1);
	MDELAY(10);
#ifdef ODM_WT_EDIT
	MDELAY(10);
	update_tpfw_notifier_call_chain(2,NULL);
	MDELAY(1);
#endif
	if (lcm_dsi_mode == CMD_MODE) {
		push_table(NULL, init_setting_cmd, sizeof(init_setting_cmd) / sizeof(struct LCM_setting_table), 1);
		pr_debug("nt36525b_hlt_lcm_mode = cmd mode esd recovery :%d----\n", lcm_dsi_mode);
	} else {
		push_table(NULL, init_setting_vdo, sizeof(init_setting_vdo) / sizeof(struct LCM_setting_table), 1);
		pr_debug("nt36525b_hlt_lcm_mode = vdo mode esd recovery :%d----\n", lcm_dsi_mode);
	}
	lcm_set_cabc_cmdq(NULL, cabc_status);
	pr_debug("lcm_esd_recovery\n");
	push_table(NULL, bl_level, sizeof(bl_level) / sizeof(struct LCM_setting_table), 1);
	//push_table_cust(NULL, bl_level, sizeof(bl_level) / sizeof(struct LCM_setting_table_V3), 1);
	return FALSE;
#else
	return FALSE;
#endif
}


struct LCM_DRIVER nt36525b_hlt_hdp_dsi_vdo_lcm_drv = {
	.name = "nt36525b_hlt_hdp_dsi_vdo_lcm_drv",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params = lcm_get_params,
	.init = lcm_init,
	.suspend = lcm_suspend,
	.resume = lcm_resume,
	.init_power = lcm_init_power,
	.resume_power = lcm_resume_power,
	.suspend_power = lcm_suspend_power,
	.esd_recover = lcm_esd_recover,
	.set_backlight_cmdq = lcm_setbacklight_cmdq,
	.set_cabc_cmdq = lcm_set_cabc_cmdq,
	.set_cabc_mode_cmdq = lcm_set_cabc_cmdq,
	.get_cabc_status=lcm_get_cabc_status,

};

