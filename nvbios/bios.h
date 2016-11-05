/*
 * Copyright (C) 2011 Marcin Kościelnicki <koriakin@0x04.net>
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef BIOS_H
#define BIOS_H

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#define ENVY_BIOS_ERR(fmt, arg...) fprintf(stderr, fmt, ##arg)
#define ENVY_BIOS_WARN(fmt, arg...) fprintf(stderr, fmt, ##arg)

struct envy_bios_part {
	unsigned int start;
	unsigned int length;
	unsigned int pcir_offset;
	uint16_t pcir_vendor;
	uint16_t pcir_device;
	uint16_t pcir_vpd;
	uint16_t pcir_len;
	uint8_t pcir_rev;
	uint8_t pcir_class[3];
	uint16_t pcir_code_rev;
	uint8_t pcir_code_type;
	uint8_t pcir_indi;
	unsigned int init_length;
	int chksum_pass;
};

enum envy_bios_type {
	ENVY_BIOS_TYPE_UNKNOWN = 0,
	ENVY_BIOS_TYPE_NV01,
	ENVY_BIOS_TYPE_NV03,
	ENVY_BIOS_TYPE_NV04,
};

struct envy_bios_mmioinit_entry {
	uint32_t addr;
	uint32_t len;
	uint32_t data[32];
};

struct envy_bios_hwea_entry {
	uint32_t base;
	uint32_t len;
	uint8_t type;
	uint32_t *data;
};

struct envy_bios_bit_entry {
	uint16_t offset;
	uint8_t type;
	uint8_t version;
	uint16_t t_offset;
	uint16_t t_len;
	uint8_t is_unk;
};

struct envy_bios_bit {
	uint16_t offset;
	uint8_t valid;
	uint8_t version;
	uint8_t hlen;
	uint8_t entriesnum;
	uint8_t rlen;
	struct envy_bios_bit_entry *entries;
};

struct envy_bios_info {
	struct envy_bios_bit_entry *bit;
	uint8_t valid;
	uint8_t version[5];
	uint16_t feature;
	uint8_t unk07[6];
	char date[9];
	/* for BR02 cards - the PCI device id of the device behind the bridge */
	uint16_t real_pcidev;
	uint8_t unk19[8];
	uint16_t unk21; /* XXX: another table pointer */
	uint8_t unk23;
	uint8_t unk24;
	uint8_t unk25;
	uint16_t unk26;
	uint8_t unk28[8];
	char unk30[13];
	uint8_t unk3c[8];
};

struct envy_bios_dacload_entry {
	uint16_t offset;
	uint32_t val;
};

struct envy_bios_dacload {
	struct envy_bios_bit_entry *bit;
	uint8_t unk02;
	uint16_t offset;
	uint8_t valid;
	uint8_t version;
	uint8_t hlen;
	uint8_t entriesnum;
	uint8_t rlen;
	struct envy_bios_dacload_entry *entries;
};

struct envy_bios_iunk21_entry {
	uint16_t offset;
	uint32_t val;
};

struct envy_bios_iunk21 {
	uint16_t offset;
	uint8_t valid;
	uint8_t version;
	uint8_t hlen;
	uint8_t entriesnum;
	uint8_t rlen;
	struct envy_bios_iunk21_entry *entries;
};

struct envy_bios_i2cscript {
	struct envy_bios_bit_entry *bit;
	uint8_t valid;
	uint16_t unk00;
	uint16_t script;
};

enum envy_bios_dcb_type {
	ENVY_BIOS_DCB_ANALOG = 0,
	ENVY_BIOS_DCB_TV = 1,
	ENVY_BIOS_DCB_TMDS = 2,
	ENVY_BIOS_DCB_LVDS = 3,
	/* 4 seen - parasite VGA encoder? */
	ENVY_BIOS_DCB_RESERVED4 = 4,
	ENVY_BIOS_DCB_SDI = 5,
	ENVY_BIOS_DCB_DP = 6,
	ENVY_BIOS_DCB_RESERVED8 = 8,
	ENVY_BIOS_DCB_EOL = 14,
	ENVY_BIOS_DCB_UNUSED = 15,
};

enum envy_bios_dcb_lvds_info {
	ENVY_BIOS_DCB_LVDS_EDID_I2C = 0,
	ENVY_BIOS_DCB_LVDS_STRAPS = 1,
	ENVY_BIOS_DCB_LVDS_EDID_ACPI = 2,
};

struct envy_bios_dcb_entry {
	uint16_t offset;
	uint8_t type;
	uint8_t i2c;
	uint8_t heads;
	uint8_t conn;
	uint8_t unk00_4;
	uint8_t unk01_4;
	uint8_t conntag;
	uint8_t loc;
	uint8_t unk02_6; /* seen used */
	uint8_t or;
	uint8_t unk03_4;
	/* generic */
	uint8_t unk04;
	uint8_t unk05;
	uint8_t unk06;
	uint8_t unk07;
	/* analog */
	uint32_t maxfreq;
	/* LVDS */
	uint8_t lvds_info;
	uint8_t lvds_pscript; /* is that a mask on pre-2.2 or what? */ /* also present on eDP? */
	/* also seen used: unk06 bit 0 */
	/* TMDS */
	uint8_t tmds_hdmi;
	/* DP */
	uint8_t dp_bw;
	uint8_t dp_lanes;
	/* LVDs/TMDS/DP */
	uint8_t links;
	uint8_t ext_addr;
};

struct envy_bios_dcb {
	uint16_t offset;
	uint8_t valid;
	uint8_t version;
	uint8_t hlen;
	uint8_t entriesnum;
	uint8_t rlen;
	uint8_t unk16;
	struct envy_bios_dcb_entry *entries;
	uint8_t rdcb_version;
	uint8_t rdcb_valid;
	uint16_t rdcb_len;
	uint8_t tvdac0_present;
	uint8_t tvdac0_neg;
	uint8_t tvdac0_line;
	uint8_t rdcb_unk04_0;
	uint8_t rdcb_unk05_2;
	uint8_t rdcb_unk06[7];
	uint8_t rdcb_unk0d;
	uint8_t rdcb_unk10[13];
};

enum envy_bios_i2c_type {
	ENVY_BIOS_I2C_VGACR = 0,
	ENVY_BIOS_I2C_PCRTC = 4,
	ENVY_BIOS_I2C_PNVIO = 5,
	ENVY_BIOS_I2C_AUXCH = 6,
	ENVY_BIOS_I2C_UNUSED = 0xff,
};

struct envy_bios_i2c_entry {
	uint16_t offset;
	uint8_t type;
	uint8_t vgacr_wr;
	uint8_t vgacr_rd;
	uint8_t loc;
	uint8_t unk00_4;
	uint8_t addr;
	uint8_t shared;
	uint8_t sharedid;
	uint8_t unk01_5;
	uint8_t unk01;
	uint8_t unk02;
};

struct envy_bios_i2c {
	uint16_t offset;
	uint8_t valid;
	uint8_t version;
	uint8_t hlen;
	uint8_t entriesnum;
	uint8_t rlen;
	uint8_t def[2];
	struct envy_bios_i2c_entry *entries;
};

enum envy_bios_gpio_tag {
	ENVY_BIOS_GPIO_PANEL_BACKLIGHT_ON	= 0x00,
	ENVY_BIOS_GPIO_PANEL_POWER		= 0x01,
	ENVY_BIOS_GPIO_PANEL_POWER_STATUS	= 0x02,
	ENVY_BIOS_GPIO_VSYNC			= 0x03,

	ENVY_BIOS_GPIO_VID_0			= 0x04,
	ENVY_BIOS_GPIO_VID_1			= 0x05,
	ENVY_BIOS_GPIO_VID_2			= 0x06,
	ENVY_BIOS_GPIO_HPD_0			= 0x07,
	ENVY_BIOS_GPIO_HPD_1			= 0x08,
	ENVY_BIOS_GPIO_FAN_PWM			= 0x09,
	/* RESERVED 0x0a */
	/* RESERVED 0x0b */
	ENVY_BIOS_GPIO_TVDAC_0			= 0x0c,
	ENVY_BIOS_GPIO_TVDAC_0_ALTERNATE_LOAD_DETECT = 0x0d,
	ENVY_BIOS_GPIO_STEREO_DAC_SELECT	= 0x0e,
	ENVY_BIOS_GPIO_STEREO_TOGGLE		= 0x0f,
	ENVY_BIOS_GPIO_ATX_POWER_BAD		= 0x10,
	ENVY_BIOS_GPIO_THERM_EVENT_DETECT	= 0x11, /* eg. ADT7473 THERM* input [pin 9] */
	/* 0x12 seen, input [NV40, NV42, G70, G80] */

	ENVY_BIOS_GPIO_MEM_VOLTAGE		= 0x18, /* at least GDDR5: 0 1.35V, 1 1.55V */
	/* 0x19 seen, output [G92, G96] */
	ENVY_BIOS_GPIO_VID_3			= 0x1a,

	ENVY_BIOS_GPIO_PANEL_BACKLIGHT_LEVEL	= 0x21,
	/* 0x22 seen, input [G70, G71, G73, G84, G86, G92 */
	ENVY_BIOS_GPIO_THERM_SHUTDOWN		= 0x23,	/* XXX: is input sometimes, and even has GF119+ SPEC_IN? */

	/* 0x25 seen, input [G72, G71, G84, G86] */

	/* 0x28 seen, output [G84, G86] */
	ENVY_BIOS_GPIO_FRAME_LOCK_DAUGHTER_INTRPT	= 0x29,

	ENVY_BIOS_GPIO_HW_SLOWDOWN_ENABLE	= 0x2b,

	/* 0x2c seen, output [G73] */
	ENVY_BIOS_GPIO_MEM_VREF			= 0x2e,
	ENVY_BIOS_GPIO_TVDAC_1			= 0x2d,
	/* 0x2e seen, output neg [lotsa G84+ cards], related to mem reclocking... also used as a SPEC NVIO input on G80 */

	/* 0x30 seen, output or neg input [G200, GF100], *twice*... and sometimes in lots of copies */

	ENVY_BIOS_GPIO_THERM_ALERT		= 0x34,

	ENVY_BIOS_GPIO_FAN_TACH			= 0x3d,

	ENVY_BIOS_GPIO_SLI_SENSE_0		= 0x40, /* XXX: uses unk40_0, unk41_4, unk41_line */
	ENVY_BIOS_GPIO_SLI_SENSE_1		= 0x41, /* XXX: uses unk40_0, unk41_4, unk41_line */
	ENVY_BIOS_GPIO_SWAP_RDY_SYNC_A		= 0x42,
	/* 0x43 seen, output [G80, G200], SPEC NVIO [or not]... not seen on GF100+ */
	ENVY_BIOS_GPIO_SLI_SENSE_1_ALT		= 0x44, /* used on G80 instead of 0x41 for some reason */


	/* 0x49 seen, output [G98, GT215, GT218, GF114] or input [GF119], unk41_line used... related to PWM? */
	ENVY_BIOS_GPIO_THERM_ALERT_OUT		= 0x49,
	ENVY_BIOS_GPIO_DP_EXT_0			= 0x4a,	/* XXX: figure out what this is... some input */
	ENVY_BIOS_GPIO_DP_EXT_1			= 0x4b,
	ENVY_BIOS_GPIO_ATX_POWER_BAD_ALT	= 0x4c,
	/* 0x4d seen, neg input [G84, G86] */

	ENVY_BIOS_GPIO_HPD_2			= 0x51,
	ENVY_BIOS_GPIO_HPD_3			= 0x52,
	/* 0x53 seen, neg input [G96] */

	/* 0x56 seen, output [G92, G200] */

	/* 0x5a seen, neg output [G96], related to 0x5c? */

	/* 0x5c seen, neg output [G94, G96, G98] */

	ENVY_BIOS_GPIO_HPD_4			= 0x5e,
	ENVY_BIOS_GPIO_HPD_5			= 0x5f,
	ENVY_BIOS_GPIO_HPD_6			= 0x60,

	/* 0x6a seen, output [MCP79] */

	ENVY_BIOS_GPIO_MXM_3_0_PIN_26		= 0x6c,
	ENVY_BIOS_GPIO_MXM_3_0_PIN_28		= 0x6d,
	ENVY_BIOS_GPIO_MXM_3_0_PIN_30		= 0x6e,

	/* 0x6f seen, input [GT216, GT218] SPEC NVIO */
	ENVY_BIOS_GPIO_HW_PWR_SLOWDOWN		= 0x6f,

	ENVY_BIOS_GPIO_VID_4			= 0x73,
	ENVY_BIOS_GPIO_VID_5			= 0x74,
	ENVY_BIOS_GPIO_VID_6			= 0x75,
	ENVY_BIOS_GPIO_VID_7			= 0x76,

	/* 0x78 seen, output [GF100, GF106, GF104, GF110, GF114, GF116, GK104 */
	ENVY_BIOS_GPIO_FAN_FAILSAFE_PWM		= 0x78,
	/* 0x79 seen, neg input [GF100, GK104], uses unk41_line */
	ENVY_BIOS_GPIO_ATX_POWER_LOW		= 0x79,
	/* 0x7a seen, open-collector output [GK104] */
	ENVY_BIOS_GPIO_ATX_FORCE_LOW_PWR	= 0x7a,

	/* 0x81 seen, output [GK106, GM107] */
	ENVY_BIOS_GPIO_VID_PWM			= 0x81,

	/* 0x83 seen, input [GK104], SPEC - connected to PWM??? */
	ENVY_BIOS_GPIO_SLI_LED_PWM		= 0x83,
	ENVY_BIOS_GPIO_LOGO_LED_PWM		= 0x84,

	ENVY_BIOS_GPIO_PSR_FRAME_LOCK_A		= 0x85,
	ENVY_BIOS_GPIO_FB_CLAMP_EC_JT_MEM_SRFSH	= 0x86,
	ENVY_BIOS_GPIO_FB_CLAMP_TOGGLE_REQ	= 0x87,

	ENVY_BIOS_GPIO_UNUSED			= 0xff,
};

struct envy_bios_gpio_entry {
	uint16_t offset;
	uint8_t tag;
	uint8_t line;
	uint8_t log[2];
	uint8_t param;
	uint8_t unk40_0;
	uint8_t unk40_2;
	uint8_t def;
	uint8_t mode;
	uint8_t spec_out;
	uint8_t spec_in;
	uint8_t gsync;
	uint8_t reserved;
	uint8_t lockpin;
	uint8_t io;
};

enum envy_bios_xpio_type {
	ENVY_BIOS_XPIO_UNUSED	= 0x00,
	/* 0x01 seen, address 0x40 */
	ENVY_BIOS_XPIO_ADT7473	= 0x02,
	ENVY_BIOS_XPIO_PCA9536	= 0x05,
	/* 0x07 seen, address 0x92 */
};

struct envy_bios_xpio {
	uint16_t offset;
	uint8_t valid;
	uint8_t version;
	uint8_t hlen;
	uint8_t entriesnum;
	uint8_t rlen;
	uint8_t type;
	uint8_t addr;
	uint8_t bus;
	uint8_t unk02_0;
	uint8_t unk02_5;
	struct envy_bios_gpio_entry *entries;
};

struct envy_bios_xpiodir {
	uint16_t offset;
	uint8_t valid;
	uint8_t version;
	uint8_t hlen;
	uint8_t entriesnum;
	uint8_t rlen;
	struct envy_bios_xpio *entries;
};

struct envy_bios_gpio {
	uint16_t offset;
	uint8_t valid;
	uint8_t version;
	uint8_t hlen;
	uint8_t entriesnum;
	uint8_t rlen;
	struct envy_bios_gpio_entry *entries;
	struct envy_bios_xpiodir xpiodir;
};

struct envy_bios_inputdev_entry {
	uint16_t offset;
	uint8_t entry;
};

struct envy_bios_inputdev {
	uint16_t offset;
	uint8_t valid;
	uint8_t version;
	uint8_t hlen;
	uint8_t entriesnum;
	uint8_t rlen;
	struct envy_bios_inputdev_entry *entries;
};

struct envy_bios_cinema {
	uint16_t offset;
	uint8_t valid;
	uint8_t version;
	uint8_t hlen;
	uint8_t unk02[10];
};

struct envy_bios_spreadspectrum_entry {
	uint16_t offset;
	uint8_t unk00;
	uint8_t unk01;
};

struct envy_bios_spreadspectrum {
	uint16_t offset;
	uint8_t valid;
	uint8_t version;
	uint8_t hlen;
	uint8_t entriesnum;
	uint8_t rlen;
	uint8_t unk04;
	uint8_t unk05;
	uint8_t unk06;
	uint8_t unk07;
	uint8_t unk08;
	uint8_t unk09;
	struct envy_bios_spreadspectrum_entry *entries;
};

enum envy_bios_extdev_type {
	/* thermal chips */
	ENVY_BIOS_EXTDEV_ADM1032	= 0x01,
	ENVY_BIOS_EXTDEV_MAX6649	= 0x02,
	ENVY_BIOS_EXTDEV_LM99		= 0x03,
	/* 0x04 seen, at address 0x98 */
	ENVY_BIOS_EXTDEV_MAX1617	= 0x06,
	ENVY_BIOS_EXTDEV_LM64		= 0x07,
	ENVY_BIOS_EXTDEV_LM89		= 0x0b,
	ENVY_BIOS_EXTDEV_TMP411		= 0x0c,

	/* I2C ADC */
	ENVY_BIOS_EXTDEV_ADS1112	= 0x30,

	/* I2C Power Controllers */
	ENVY_BIOS_EXTDEV_PIC16F690	= 0xC0,
	ENVY_BIOS_EXTDEV_VT1103		= 0x40,
	ENVY_BIOS_EXTDEV_PX3540		= 0x41,
	ENVY_BIOS_EXTDEV_VT1165		= 0x42,
	ENVY_BIOS_EXTDEV_CHIL_I2C	= 0x43, /* CHL8203/8212/8213/8214 */

	/* SMBUS Power Controllers */
	ENVY_BIOS_EXTDEV_CHIL_SMBUS0	= 0x48, /* CHL8112A/B, CHL8225/8228 */
	ENVY_BIOS_EXTDEV_CHIL_SMBUS1	= 0x49, /* CHL8266, CHL8316 */

	/* Power sensors */
	ENVY_BIOS_EXTDEV_INA219		= 0x4c,
	ENVY_BIOS_EXTDEV_INA209		= 0x4d,
	ENVY_BIOS_EXTDEV_INA3221	= 0x4e,

	/* Clock generator */
	ENVY_BIOS_EXTDEV_CY2XP304	= 0x50,

	/* External GPIO controllers */
	ENVY_BIOS_EXTDEV_PCA9555	= 0x60, /* Japanese HDTV support */

	/* Fan controllers */
	ENVY_BIOS_EXTDEV_ADT7473	= 0x70,

	/* HDMI Compositor/converters */
	ENVY_BIOS_EXTDEV_SI1930UC	= 0x80,

	/* HDCP EEPROM */
	ENVY_BIOS_EXTDEV_HDCP_EEPROM	= 0x90,

	ENVY_BIOS_EXTEDV_INTERNAL_A0	= 0xa0,

	/* GPU I2C Controllers */
	ENVY_BIOS_EXTDEV_GT21X		= 0xB0,
	ENVY_BIOS_EXTDEV_GF11X		= 0xB1,

	/* Display encoders */
	ENVY_BIOS_EXTDEV_ANX9805	= 0xD0,

	/* 0xa0 seen, at address 0xa8 */
	ENVY_BIOS_EXTDEV_UNUSED		= 0xff,
};

struct envy_bios_extdev_entry {
	uint16_t offset;
	uint8_t type;
	uint8_t addr;
	uint8_t bus;
	uint8_t unk02_0;
	uint8_t unk02_5;
	uint8_t unk03;
};

struct envy_bios_extdev {
	uint16_t offset;
	uint8_t valid;
	uint8_t version;
	uint8_t hlen;
	uint8_t entriesnum;
	uint8_t rlen;
	uint8_t unk04;
	struct envy_bios_extdev_entry *entries;
};

enum envy_bios_conn_type {
	/* VGA */
	ENVY_BIOS_CONN_VGA			= 0x00,
	/* TV */
	ENVY_BIOS_CONN_COMPOSITE		= 0x10,
	ENVY_BIOS_CONN_S_VIDEO			= 0x11,
	ENVY_BIOS_CONN_S_VIDEO_COMPOSITE	= 0x12,	/* used on NV40 only - later cards have separate 0x10 and 0x11 entries instead */
	ENVY_BIOS_CONN_RGB			= 0x13, /* ... or YCbCr? */
	/* 0x17 seen, another type of TV */
	ENVY_BIOS_CONN_DVI_I_TV_S_VIDEO		= 0x20,
	/* DVI */
	ENVY_BIOS_CONN_DVI_I			= 0x30,
	ENVY_BIOS_CONN_DVI_D			= 0x31,
	ENVY_BIOS_CONN_DMS59_0			= 0x38,
	ENVY_BIOS_CONN_DMS59_1			= 0x39,
	/* LVDS and DP */
	ENVY_BIOS_CONN_LVDS			= 0x40,
	ENVY_BIOS_CONN_LVDS_SPWG		= 0x41,
	ENVY_BIOS_CONN_DP			= 0x46,
	ENVY_BIOS_CONN_EDP			= 0x47,
	/* ??? */
	/* 0x54 and 0x55 seen - some crazy TMDS connector in use on Dell laptops */
	/* stereo, HDMI... and DP? */
	ENVY_BIOS_CONN_STEREO			= 0x60,
	ENVY_BIOS_CONN_HDMI			= 0x61,
	/* 0x62 seen, never connected to anything sensible */
	ENVY_BIOS_CONN_HDMI_C			= 0x63,
	ENVY_BIOS_CONN_DMS59_DP0		= 0x64,
	ENVY_BIOS_CONN_DMS59_DP1		= 0x65,
	ENVY_BIOS_CONN_UNUSED			= 0xff,
};

struct envy_bios_conn_entry {
	uint16_t offset;
	uint8_t type;
	uint8_t tag;
	int8_t hpd;
	int8_t dp_ext;
	uint8_t unk02_2;
	uint8_t unk02_4; /* a bitmask [like hpd/dp_ext] of something to do with native DP [but not EDP] encoders */
	uint8_t unk02_7; /* probably unused */
	uint8_t unk03_3; /* probably unused */
};

struct envy_bios_conn {
	uint16_t offset;
	uint8_t valid;
	uint8_t version;
	uint8_t hlen;
	uint8_t entriesnum;
	uint8_t rlen;
	struct envy_bios_conn_entry *entries;
};

struct envy_bios_hdtvtt_entry {
	uint16_t offset;
};

struct envy_bios_hdtvtt {
	uint16_t offset;
	uint8_t valid;
	uint8_t version;
	uint8_t hlen;
	uint8_t entriesnum;
	uint8_t rlen;
	struct envy_bios_hdtvtt_entry *entries;
};

struct envy_bios_mux_entry {
	uint16_t offset;
	uint8_t idx;
	uint8_t sub_loc[4];
	uint8_t sub_line[4];
	uint8_t sub_val[4];
	uint8_t sub_unk7[4];
};

struct envy_bios_mux {
	uint16_t offset;
	uint8_t valid;
	uint8_t version;
	uint8_t hlen;
	uint8_t entriesnum;
	uint8_t rlen;
	struct envy_bios_mux_entry *entries;
};

struct envy_bios_mem_train_entry {
	uint16_t offset;
	uint8_t u00;
	uint8_t subentry[8];
};

struct envy_bios_mem_train {
	uint16_t offset;
	uint8_t valid;
	uint8_t version;
	uint8_t hlen;
	uint8_t entriesnum;
	uint8_t rlen;
	uint8_t subentries;
	uint8_t subentrylen;
	uint16_t mclk;
	struct envy_bios_mem_train_entry *entries;
};

struct envy_bios_mem_train_ptrn_entry {
	uint16_t offset;
	uint8_t bits;
	uint8_t modulo;
	uint8_t indirect;
	uint8_t indirect_entry;
};

struct envy_bios_mem_train_ptrn {
	uint16_t offset;
	uint8_t valid;
	uint8_t version;
	uint8_t hlen;
	uint8_t entriesnum;
	uint8_t rlen;
	uint8_t subentries;
	uint8_t subentrylen;
	struct envy_bios_mem_train_ptrn_entry *entries;
};

struct envy_bios_mem_type {
	uint16_t offset;
	uint8_t valid;
	uint8_t version;
	uint8_t hlen;
	uint8_t entriesnum;
	uint8_t rlen;
	const char **entries;
};

struct envy_bios_mem {
	struct envy_bios_bit_entry *bit;

	struct envy_bios_mem_train train;
	struct envy_bios_mem_train_ptrn train_ptrn;
	struct envy_bios_mem_type type;
	uint16_t trestrict;
};

struct envy_bios_power_volt {
	uint32_t offset;
};

struct envy_bios_power_unk14_entry {
	uint32_t offset;
};

struct envy_bios_power_unk14 {
	uint32_t offset;
	uint8_t valid;
	uint8_t version;
	uint8_t hlen;
	uint8_t entriesnum;
	uint8_t rlen;

	struct envy_bios_power_unk14_entry *entries;
};

struct envy_bios_power_unk18_entry {
	uint32_t offset;
};

struct envy_bios_power_unk18 {
	uint32_t offset;
	uint8_t valid;
	uint8_t version;
	uint8_t hlen;
	uint8_t entriesnum;
	uint8_t rlen;

	struct envy_bios_power_unk18_entry *entries;
};

struct envy_bios_power_unk1c_entry {
	uint32_t offset;
};

struct envy_bios_power_unk1c {
	uint32_t offset;
	uint8_t version;
};

struct envy_bios_power_volt_map {
	uint32_t offset;
};

struct envy_bios_power_perf {
	uint32_t offset;
};

struct envy_bios_power_therm {
	uint32_t offset;
};

struct envy_bios_power_timing {
	uint32_t offset;
};

struct envy_bios_power_timing_map {
	uint32_t offset;
};

struct envy_bios_power_boost_subentry {
	uint32_t offset;
	uint8_t domain;
	uint8_t percent;
	uint16_t min;
	uint16_t max;
};

struct envy_bios_power_boost_entry {
	uint32_t offset;
	uint8_t pstate;
	uint16_t min;
	uint16_t max;
	struct envy_bios_power_boost_subentry *entries;
};

struct envy_bios_power_boost {
	uint32_t offset;
	uint8_t valid;
	uint8_t version;
	uint8_t hlen;
	uint8_t entriesnum;
	uint8_t rlen;
	uint8_t snr;
	uint8_t ssz;
	struct envy_bios_power_boost_entry *entries;
};

struct envy_bios_power_cstep_entry1 {
	uint32_t offset;
	uint8_t pstate;
	uint8_t index;
};

struct envy_bios_power_cstep_entry2 {
	uint32_t offset;
	uint8_t valid;
	uint16_t freq;
	uint8_t unkn[2];
	uint8_t voltage;
};

struct envy_bios_power_cstep {
	uint32_t offset;
	uint8_t valid;
	uint8_t version;
	uint8_t hlen;
	uint8_t entriesnum;
	uint8_t rlen;
	uint8_t snr;
	uint8_t ssz;
	struct envy_bios_power_cstep_entry1 ent1[8];
	struct envy_bios_power_cstep_entry2 *ent2;
};

struct envy_bios_power_budget_entry {
	uint32_t offset;
	uint8_t valid;

	uint32_t min;
	uint32_t avg;
	uint32_t peak;
	uint32_t unkn12;
};

struct envy_bios_power_budget {
	uint32_t offset;
	uint8_t valid;
	uint8_t version;
	uint8_t hlen;
	uint8_t entriesnum;
	uint8_t rlen;

	uint8_t cap_entry;

	struct envy_bios_power_budget_entry *entries;
};

struct envy_bios_power_unk24_entry {
	uint32_t offset;
};

struct envy_bios_power_unk24 {
	uint32_t offset;
	uint8_t valid;
	uint8_t version;
	uint8_t hlen;
	uint8_t entriesnum;
	uint8_t rlen;

	struct envy_bios_power_unk24_entry *entries;
};

struct envy_bios_power_sense_entry {
	uint32_t offset;
	uint8_t mode;
	uint8_t extdev_id;
	uint8_t resistor_mohm;
	uint8_t rail;
	uint16_t config;
};

struct envy_bios_power_sense {
	uint32_t offset;
	uint8_t valid;
	uint8_t version;
	uint8_t hlen;
	uint8_t entriesnum;
	uint8_t rlen;

	struct envy_bios_power_sense_entry *entries;
};

struct envy_bios_power_base_clock_entry {
	uint32_t offset;

	uint8_t pstate;
	uint16_t reqPower;
	uint16_t reqSlowdownPower;
	uint16_t *clock;
};

struct envy_bios_power_base_clock {
	uint32_t offset;
	uint8_t valid;
	uint8_t version;

	uint8_t hlen;
	uint8_t entriesnum;
	uint8_t rlen;
	uint8_t selen;
	uint8_t secount;

	uint8_t d2_entry;
	uint8_t d3_entry;
	uint8_t d4_entry;
	uint8_t d5_entry;
	uint8_t over_current_entry;
	uint8_t vrhot_entry;
	uint8_t max_batt_entry;
	uint8_t max_sli_entry;
	uint8_t max_therm_sustain_entry;
	uint8_t boost_entry;
	uint8_t turbo_boost_entry;
	uint8_t rated_tdp_entry;
	uint8_t slowdown_pwr_entry;
	uint8_t mid_point_entry;
	uint8_t unk15_entry;
	uint8_t unk16_entry;

	struct envy_bios_power_base_clock_entry *entries;
};

struct envy_bios_power_unk3c_entry {
	uint32_t offset;
};

struct envy_bios_power_unk3c {
	uint32_t offset;
	uint8_t valid;
	uint8_t version;
	uint8_t hlen;
	uint8_t entriesnum;
	uint8_t rlen;

	struct envy_bios_power_unk3c_entry *entries;
};

struct envy_bios_power_unk40_entry {
	uint32_t offset;
};

struct envy_bios_power_unk40 {
	uint32_t offset;
	uint8_t valid;
	uint8_t version;
	uint8_t hlen;
	uint8_t entriesnum;
	uint8_t rlen;

	struct envy_bios_power_unk40_entry *entries;
};

struct envy_bios_power_unk44 {
	uint32_t offset;
	uint8_t valid;
	uint8_t version;
	uint8_t hlen;
};

struct envy_bios_power_unk48_entry {
	uint32_t offset;
};

struct envy_bios_power_unk48 {
	uint32_t offset;
	uint8_t valid;
	uint8_t version;
	uint8_t hlen;
	uint8_t entriesnum;
	uint8_t rlen;

	struct envy_bios_power_unk48_entry *entries;
};

struct envy_bios_power_unk4c_entry {
	uint32_t offset;
};

struct envy_bios_power_unk4c {
	uint32_t offset;
	uint8_t valid;
	uint8_t version;
	uint8_t hlen;
	uint8_t entriesnum;
	uint8_t rlen;

	struct envy_bios_power_unk4c_entry *entries;
};

struct envy_bios_power_unk50_entry {
	uint32_t offset;

	uint8_t  mode;
	uint16_t downclock_t;
	uint16_t t1;
	uint16_t t2;
	uint16_t interval_us;
};

struct envy_bios_power_unk50 {
	uint32_t offset;
	uint8_t valid;
	uint8_t version;
	uint8_t hlen;
	uint8_t entriesnum;
	uint8_t rlen;

	struct envy_bios_power_unk50_entry *entries;
};

struct envy_bios_power_unk54_entry {
	uint32_t offset;
};

struct envy_bios_power_unk54 {
	uint32_t offset;
	uint8_t valid;
	uint8_t version;
	uint8_t hlen;
	uint8_t entriesnum;
	uint8_t rlen;

	struct envy_bios_power_unk54_entry *entries;
};

struct envy_bios_power_fan {
	uint32_t offset;
	uint8_t valid;
	uint8_t version;
	uint8_t hlen;
	uint8_t entriesnum;
	uint8_t rlen;

	uint8_t type;
	uint8_t duty_min;
	uint8_t duty_max;
	uint32_t divisor;
	uint16_t unk0e;
	uint16_t unk10;
	uint16_t unboost_unboost_ms;
	uint8_t duty_boosted;
};

struct envy_bios_power_fan_mgmt_entry {
	uint32_t offset;

	uint8_t  duty0;
	uint16_t temp0;
	uint16_t speed0;

	uint8_t  duty1;
	uint16_t temp1;
	uint16_t speed1;

	uint8_t  duty2;
	uint16_t temp2;
	uint16_t speed2;
};

struct envy_bios_power_fan_mgmt {
	uint32_t offset;
	uint8_t valid;
	uint8_t version;
	uint8_t hlen;
	uint8_t entriesnum;
	uint8_t rlen;

	struct envy_bios_power_fan_mgmt_entry *entries;
};

struct envy_bios_power_unk60_entry {
	uint32_t offset;
};

struct envy_bios_power_unk60 {
	uint32_t offset;
	uint8_t valid;
	uint8_t version;
	uint8_t hlen;
	uint8_t entriesnum;
	uint8_t rlen;

	struct envy_bios_power_unk60_entry *entries;
};

struct envy_bios_power_unk64_entry {
	uint32_t offset;
};

struct envy_bios_power_unk64 {
	uint32_t offset;
	uint8_t valid;
	uint8_t version;
	uint8_t hlen;
	uint8_t entriesnum;
	uint8_t rlen;

	struct envy_bios_power_unk64_entry *entries;
};

struct envy_bios_power {
	struct envy_bios_bit_entry *bit;

	struct envy_bios_power_perf perf;
	struct envy_bios_power_timing_map timing_map;
	struct envy_bios_power_timing timing;
	struct envy_bios_power_therm therm;
	struct envy_bios_power_volt volt;
	struct envy_bios_power_unk14 unk14;
	struct envy_bios_power_unk18 unk18;
	struct envy_bios_power_unk1c unk1c;
	struct envy_bios_power_volt_map volt_map;

	struct envy_bios_power_unk24 unk24;
	struct envy_bios_power_sense sense;
	struct envy_bios_power_budget budget;
	struct envy_bios_power_boost boost;
	struct envy_bios_power_cstep cstep;
	struct envy_bios_power_base_clock base_clock;
	struct envy_bios_power_unk3c unk3c;
	struct envy_bios_power_unk40 unk40;
	struct envy_bios_power_unk44 unk44;
	struct envy_bios_power_unk48 unk48;
	struct envy_bios_power_unk4c unk4c;
	struct envy_bios_power_unk50 unk50;
	struct envy_bios_power_unk54 unk54;
	struct envy_bios_power_fan fan;
	struct envy_bios_power_fan_mgmt fan_mgmt;
	struct envy_bios_power_unk60 unk60;
	struct envy_bios_power_unk64 unk64;
};

struct envy_bios_block {
	unsigned int start;
	unsigned int len;
	const char *name;
	int idx;
};

struct envy_bios {
	uint8_t *data;
	unsigned int length;
	unsigned int origlength;

	struct envy_bios_part *parts;
	int partsnum;
	int broken_part;

	enum envy_bios_type type;
	int chipset;
	char *chipset_name;

	uint16_t subsystem_vendor;
	uint16_t subsystem_device;

	uint8_t hwinfo_ext_valid;
	uint32_t straps0_select;
	uint32_t straps0_value;
	uint32_t straps1_select;
	uint32_t straps1_value;
	uint32_t mmioinit_offset;
	uint32_t mmioinit_len;

	struct envy_bios_mmioinit_entry *mmioinits;
	int mmioinitsnum;
	int mmioinitsmax;

	unsigned int hwea_version;
	unsigned int hwea_offset;
	unsigned int hwea_len;
	struct envy_bios_hwea_entry *hwea_entries;
	int hwea_entriesnum;
	int hwea_entriesmax;

	unsigned int bmp_offset;
	unsigned int bmp_length;
	uint8_t bmp_ver_major;
	uint8_t bmp_ver_minor;
	uint16_t bmp_ver;
	uint16_t mode_x86;
	uint16_t init_x86;
	uint16_t init_script;

	struct envy_bios_bit bit;

	struct envy_bios_info info;
	struct envy_bios_dacload dacload;
	struct envy_bios_iunk21 iunk21;

	struct envy_bios_i2cscript i2cscript;

	unsigned int hwsq_offset;

	unsigned int odcb_offset;
	unsigned int dev_rec_offset;
	struct envy_bios_dcb dcb;
	struct envy_bios_i2c i2c;
	struct envy_bios_gpio gpio;
	struct envy_bios_inputdev inputdev;
	struct envy_bios_cinema cinema;
	struct envy_bios_spreadspectrum spreadspectrum;
	struct envy_bios_extdev extdev;
	struct envy_bios_conn conn;
	struct envy_bios_hdtvtt hdtvtt;
	struct envy_bios_mux mux;
	struct envy_bios_power power;
	struct envy_bios_mem mem;

	struct envy_bios_block *blocks;
	int blocksnum;
	int blocksmax;
};

static inline int bios_u8(struct envy_bios *bios, unsigned int offs, uint8_t *res) {
	if (offs >= bios->length) {
		*res = 0;
		ENVY_BIOS_ERR("requested OOB u8 at 0x%04x\n", offs);
		return -EFAULT;
	}
	*res = bios->data[offs];
	return 0;
}

static inline int bios_u16(struct envy_bios *bios, unsigned int offs, uint16_t *res) {
	if (offs+1 >= bios->length) {
		*res = 0;
		ENVY_BIOS_ERR("requested OOB u16 at 0x%04x\n", offs);
		return -EFAULT;
	}
	*res = bios->data[offs] | bios->data[offs+1] << 8;
	return 0;
}

static inline int bios_u32(struct envy_bios *bios, unsigned int offs, uint32_t *res) {
	if (offs+3 >= bios->length) {
		*res = 0;
		ENVY_BIOS_ERR("requested OOB u32 at 0x%04x\n", offs);
		return -EFAULT;
	}
	*res = bios->data[offs] | bios->data[offs+1] << 8 | bios->data[offs+2] << 16 | bios->data[offs+3] << 24;
	return 0;
}

static inline int bios_string(struct envy_bios *bios, unsigned int offs, char *res, int len) {
	if (offs+len-1 >= bios->length) {
		ENVY_BIOS_ERR("requested OOB string at 0x%04x len 0x%04x\n", offs, len);
		return -EFAULT;
	}
	int i;
	for (i = 0; i < len; i++)
		res[i] = bios->data[offs+i];
	res[len] = 0;
	return 0;
}

#define ENVY_BIOS_PRINT_PCIR		0x00000001
#define ENVY_BIOS_PRINT_VERSION		0x00000002
#define ENVY_BIOS_PRINT_HWINFO		0x00000004
#define ENVY_BIOS_PRINT_BMP_BIT		0x00000008
#define ENVY_BIOS_PRINT_INFO		0x00000010
#define ENVY_BIOS_PRINT_DACLOAD		0x00000020
#define ENVY_BIOS_PRINT_IUNK		0x00000040
#define ENVY_BIOS_PRINT_SCRIPTS		0x00000080
#define ENVY_BIOS_PRINT_PLL		0x00000100
#define ENVY_BIOS_PRINT_RAM		0x00000200
#define ENVY_BIOS_PRINT_PERF		0x00000400
#define ENVY_BIOS_PRINT_I2CSCRIPT	0x00000800
#define ENVY_BIOS_PRINT_MEM		0x00001000
#define ENVY_BIOS_PRINT_HWSQ		0x00008000
#define ENVY_BIOS_PRINT_DCB		0x00010000
#define ENVY_BIOS_PRINT_GPIO		0x00020000
#define ENVY_BIOS_PRINT_I2C		0x00040000
#define ENVY_BIOS_PRINT_EXTDEV		0x00080000
#define ENVY_BIOS_PRINT_CONN		0x00100000
#define ENVY_BIOS_PRINT_MUX		0x00200000
#define ENVY_BIOS_PRINT_DUNK		0x00400000
#define ENVY_BIOS_PRINT_DCB_ALL		0x007f0000
#define ENVY_BIOS_PRINT_ALL		0x1fffffff
#define ENVY_BIOS_PRINT_BLOCKS		0x20000000
#define ENVY_BIOS_PRINT_UNUSED		0x40000000
#define ENVY_BIOS_PRINT_VERBOSE		0x80000000

int envy_bios_parse (struct envy_bios *bios);
void envy_bios_dump_hex (struct envy_bios *bios, FILE *out, unsigned int start, unsigned int length, unsigned mask);
void envy_bios_print (struct envy_bios *bios, FILE *out, unsigned mask);
void envy_bios_mmioinit (struct envy_bios *bios, FILE *out, unsigned mask);

int envy_bios_parse_bit (struct envy_bios *bios);
void envy_bios_print_bit (struct envy_bios *bios, FILE *out, unsigned mask);

int envy_bios_parse_bit_i (struct envy_bios *bios, struct envy_bios_bit_entry *bit);
void envy_bios_print_info (struct envy_bios *bios, FILE *out, unsigned mask);
int envy_bios_parse_bit_A (struct envy_bios *bios, struct envy_bios_bit_entry *bit);
int envy_bios_parse_dacload (struct envy_bios *bios);
void envy_bios_print_dacload (struct envy_bios *bios, FILE *out, unsigned mask);
int envy_bios_parse_iunk21 (struct envy_bios *bios);
void envy_bios_print_iunk21 (struct envy_bios *bios, FILE *out, unsigned mask);

int envy_bios_parse_bit_2 (struct envy_bios *bios, struct envy_bios_bit_entry *bit);
void envy_bios_print_i2cscript (struct envy_bios *bios, FILE *out, unsigned mask);

int envy_bios_parse_bit_P (struct envy_bios *bios, struct envy_bios_bit_entry *bit);
void envy_bios_print_bit_P (struct envy_bios *bios, FILE *out, unsigned mask);
void envy_bios_print_power_unk14(struct envy_bios *bios, FILE *out, unsigned mask);
void envy_bios_print_power_unk18(struct envy_bios *bios, FILE *out, unsigned mask);
void envy_bios_print_power_unk1c(struct envy_bios *bios, FILE *out, unsigned mask);
void envy_bios_print_power_unk24(struct envy_bios *bios, FILE *out, unsigned mask);
void envy_bios_print_power_sense(struct envy_bios *bios, FILE *out, unsigned mask);
void envy_bios_print_power_budget(struct envy_bios *bios, FILE *out, unsigned mask);
void envy_bios_print_power_boost(struct envy_bios *bios, FILE *out, unsigned mask);
void envy_bios_print_power_cstep(struct envy_bios *bios, FILE *out, unsigned mask);
void envy_bios_print_power_base_clock(struct envy_bios *bios, FILE *out, unsigned mask);
void envy_bios_print_power_unk3c(struct envy_bios *bios, FILE *out, unsigned mask);
void envy_bios_print_power_unk40(struct envy_bios *bios, FILE *out, unsigned mask);
void envy_bios_print_power_unk44(struct envy_bios *bios, FILE *out, unsigned mask);
void envy_bios_print_power_unk48(struct envy_bios *bios, FILE *out, unsigned mask);
void envy_bios_print_power_unk4c(struct envy_bios *bios, FILE *out, unsigned mask);
void envy_bios_print_power_unk50(struct envy_bios *bios, FILE *out, unsigned mask);
void envy_bios_print_power_unk54(struct envy_bios *bios, FILE *out, unsigned mask);
void envy_bios_print_power_fan(struct envy_bios *bios, FILE *out, unsigned mask);
void envy_bios_print_power_fan_mgmt(struct envy_bios *bios, FILE *out, unsigned mask);
void envy_bios_print_power_unk60(struct envy_bios *bios, FILE *out, unsigned mask);
void envy_bios_print_power_unk64(struct envy_bios *bios, FILE *out, unsigned mask);

int envy_bios_parse_bit_M (struct envy_bios *bios, struct envy_bios_bit_entry *bit);
void envy_bios_print_bit_M (struct envy_bios *bios, FILE *out, unsigned mask);
void envy_bios_print_mem_train(struct envy_bios *bios, FILE *out, unsigned mask);
void envy_bios_print_mem_train_ptrn(struct envy_bios *bios, FILE *out, unsigned mask);
void envy_bios_print_mem_type(struct envy_bios *bios, FILE *out, unsigned mask);

int envy_bios_parse_dcb (struct envy_bios *bios);
void envy_bios_print_dcb (struct envy_bios *bios, FILE *out, unsigned mask);
void envy_bios_print_odcb (struct envy_bios *bios, FILE *out, unsigned mask);
int envy_bios_parse_i2c (struct envy_bios *bios);
void envy_bios_print_i2c (struct envy_bios *bios, FILE *out, unsigned mask);
int envy_bios_parse_gpio (struct envy_bios *bios);
void envy_bios_print_gpio (struct envy_bios *bios, FILE *out, unsigned mask);
int envy_bios_parse_inputdev (struct envy_bios *bios);
void envy_bios_print_inputdev (struct envy_bios *bios, FILE *out, unsigned mask);
int envy_bios_parse_cinema (struct envy_bios *bios);
void envy_bios_print_cinema (struct envy_bios *bios, FILE *out, unsigned mask);
int envy_bios_parse_spreadspectrum (struct envy_bios *bios);
void envy_bios_print_spreadspectrum (struct envy_bios *bios, FILE *out, unsigned mask);
int envy_bios_parse_extdev (struct envy_bios *bios);
void envy_bios_print_extdev (struct envy_bios *bios, FILE *out, unsigned mask);
int envy_bios_parse_conn (struct envy_bios *bios);
void envy_bios_print_conn (struct envy_bios *bios, FILE *out, unsigned mask);
int envy_bios_parse_hdtvtt (struct envy_bios *bios);
void envy_bios_print_hdtvtt (struct envy_bios *bios, FILE *out, unsigned mask);
int envy_bios_parse_mux (struct envy_bios *bios);
void envy_bios_print_mux (struct envy_bios *bios, FILE *out, unsigned mask);
int envy_bios_parse_mem (struct envy_bios *bios);
void envy_bios_print_mem (struct envy_bios *bios, FILE *out, unsigned mask);


struct enum_val {
	int val;
	const char *str;
};

const char *find_enum(struct enum_val *evals, int val);

void envy_bios_block(struct envy_bios *bios, unsigned start, unsigned len, const char *name, int idx);

#endif
