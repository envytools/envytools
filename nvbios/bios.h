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
	ENVY_BIOS_GPIO_VTG_RST            = 0x12, /* Input signal from daughter card for Frame Lock interface headers. Seen, input [NV40, NV42, G70, G80] */
	ENVY_BIOS_GPIO_SUS_STAT           = 0x13, /* Input requesting the suspend state be entered */
	ENVY_BIOS_GPIO_SPREAD_0           = 0x14, /* Bit 0 of output to control Spread Spectrum if the chip isn’t I2C controlled */
	ENVY_BIOS_GPIO_SPREAD_1           = 0x15, /* Bit 1 of output to control Spread Spectrum if the chip isn’t I2C controlled */
	ENVY_BIOS_GPIO_VDS_FRAMEID_0      = 0x16, /* Bit 0 of the frame ID when using Virtual Display Switching */
	ENVY_BIOS_GPIO_VDS_FRAMEID_1      = 0x17, /* Bit 1 of the frame ID when using Virtual Display Switching */
	ENVY_BIOS_GPIO_MEM_VOLTAGE		= 0x18, /* at least GDDR5: 0 1.35V, 1 1.55V */

	ENVY_BIOS_GPIO_CUSTOMER           = 0x19, /* OEM reserved. Seen, output [G92, G96] */

	ENVY_BIOS_GPIO_VID_3			= 0x1a,
	ENVY_BIOS_GPIO_VID_DEFAULT        = 0x1b, /* Allow switching from default voltage (1) to selected voltage (0) */

	ENVY_BIOS_GPIO_TUNER              = 0x1c,
	ENVY_BIOS_GPIO_CURRENT_SHARE      = 0x1d,
	ENVY_BIOS_GPIO_CURRENT_SHARE_ENABLE = 0x1e,

	ENVY_BIOS_GPIO_PANEL_SELF_TEST    = 0x1f, /* LCD0 corresponds to the LCD0 defined in LCD ID field in Connector Table. */
	ENVY_BIOS_GPIO_PANEL_LAMP_STATUS  = 0x20, /* LCD0 corresponds to the LCD0 defined in LCD ID field in Connector Table. */
	ENVY_BIOS_GPIO_PANEL_BACKLIGHT_LEVEL	= 0x21,

	ENVY_BIOS_GPIO_REQUIRED_POWER_SENSE = 0x22, /* Similar to 0x10, but without the thermal half. Seen, input [G70, G71, G73, G84, G86, G92 */
	ENVY_BIOS_GPIO_THERM_SHUTDOWN		= 0x23,	/* XXX: is input sometimes, and even has GF119+ SPEC_IN? */

	ENVY_BIOS_GPIO_HDTV_SELECT        = 0x24, /* Allows selection of lines driven between SDTV - Off state, and HDTV - On State */
	ENVY_BIOS_GPIO_HDTV_ALT_DETECT    = 0x25, /* Allows detection of the connectors that are not selected by HDTV Select. Seen, input [G72, G71, G84, G86]
	                                             That is, if HDTV Select is currently selecting SDTV, then this GPIO would allow us to detect the presence
	                                             of the HDTV connection */
	/* RESERVED 0x26 */
	ENVY_BIOS_GPIO_OPTIONAL_POWER_SENSE = 0x27, /* Similar to 0x10 and 0x22, but without the thermal half and not necessary for normal non-overclocked operation */

	/* 0x28 seen, output [G84, G86] */
	ENVY_BIOS_GPIO_FRAME_LOCK_DAUGHTER_INTRPT	= 0x29,

	ENVY_BIOS_GPIO_SW_PERF_LEVEL_SLOWDOWN = 0x2a, /* When asserted, the SW will lower it’s performance level to the lowest state. */
	ENVY_BIOS_GPIO_HW_SLOWDOWN_ENABLE	= 0x2b, /* On assertion HW will slowdown clocks (NVCLK, HOTCLK) using either:
	                                             _EXT_POWER, _EXT_ALERT or _EXT_OVERT settings (depends on GPIO configured: 12, 9 & 8 respectively)
	                                             Then SW will take over, limit GPU p-state to battery level and disable slowdown.
	                                             On deassertion SW will reenable slowdown and remove p-state limit. System will continue running full clocks. */
	ENVY_BIOS_GPIO_DISABLE_POWER_SENSE = 0x2c, /* If asserted, this GPIO will remove the power sense circuit from affecting HW Slowdown. Seen, output [G73] */

	ENVY_BIOS_GPIO_MEM_VREF			= 0x2e,
	ENVY_BIOS_GPIO_TVDAC_1			= 0x2d,
	/* 0x2e seen, output neg [lotsa G84+ cards], related to mem reclocking... also used as a SPEC NVIO input on G80 */
	/* RESERVED 0x2f */
	ENVY_BIOS_GPIO_GENERIC_INITIALIZED = 0x30, /* This GPIO is used, but does not have a specific function assigned to it or has a function defined elsewhere.
	                                              System software should initialize this GPIO using the _INIT values for the chip.
	                                              This function should be specified when a GPIO needs to be set statically during initialization.
	                                              This is different than function 0x19, which implies that the GPIO is not used by NVIDIA software.
	                                              0x30 seen, output or neg input [G200, GF100], *twice*... and sometimes in lots of copies */
	ENVY_BIOS_GPIO_HD_OVER_SDTV_BOOT_PREF = 0x31, /* Allows user to select whether to boot to SDTV or component output by default. */
	ENVY_BIOS_GPIO_DIGITAL_ENC_INTRPT_ENABLE = 0x32, /* For SI1930UC, a GPIO will be set ON to trigger interrupt to SI1930UC to enable I2C communication.
	                                                    When I2C transactions to the SI1930UC are complete, the drivers will set this GPIO to OFF. */
	ENVY_BIOS_GPIO_I2C_OR_DDC         = 0x33, /* Selects I2C communications between either DDC or I2C */
	ENVY_BIOS_GPIO_THERM_ALERT		= 0x34,
	ENVY_BIOS_GPIO_THERM_CRITICAL     = 0x35, /* Comparator-driven input from external thermal device. Indicates that a temperature is above a critical limit. */
	/* RESERVED 0x36 */
	/* RESERVED 0x37 */
	/* RESERVED 0x38 */
	/* RESERVED 0x39 */
	/* RESERVED 0x3a */
	/* RESERVED 0x3b */
	ENVY_BIOS_GPIO_SCART_SELECT       = 0x3c, /* Allows selection of lines driven between SDTV (S-Video, Composite) and SDTV (SCART). */
	ENVY_BIOS_GPIO_FAN_TACH			= 0x3d,
	/* RESERVED 0x3e */
	ENVY_BIOS_GPIO_EXT_SYNC_0         = 0x3f, /* Used with external framelock with GSYNC products. It also could be used for raster lock. */
	ENVY_BIOS_GPIO_SLI_SENSE_0		= 0x40, /* XXX: uses unk40_0, unk41_4, unk41_line */
	ENVY_BIOS_GPIO_SLI_SENSE_1		= 0x41, /* XXX: uses unk40_0, unk41_4, unk41_line */
	ENVY_BIOS_GPIO_SWAP_RDY_SYNC_A		= 0x42,
	ENVY_BIOS_GPIO_SWAP_RDY_OUT		= 0x43,
	ENVY_BIOS_GPIO_SLI_SENSE_1_ALT		= 0x44, /* used on G80 instead of 0x41 for some reason */

	ENVY_BIOS_GPIO_SCART_0            = 0x45, /* Bit 0 of the SCART Aspect Ratio Field */
	ENVY_BIOS_GPIO_SCART_1            = 0x46, /* Bit 1 of the SCART Aspect Ratio Field */
	                                          /* GPIOs 0x45 and 0x46 define a 2 bit SCART Aspect Ratio Field. Possible values:
	                                               0 = 4:3(12V)
	                                               1 = 16:9(6V)
	                                               2 = Undefined
	                                               3 = SCART inactive (0 V) */

	ENVY_BIOS_GPIO_HD_DONGLE_STRAP_0  = 0x47, /* Bit 0 of the HD Dongle Strap Field */
	ENVY_BIOS_GPIO_HD_DONGLE_STRAP_1  = 0x48, /* Bit 1 of the HD Dongle Strap Field */

	/* 0x49 seen, output [G98, GT215, GT218, GF114] or input [GF119], unk41_line used... related to PWM? */
	ENVY_BIOS_GPIO_THERM_ALERT_OUT		= 0x49,
	ENVY_BIOS_GPIO_DP_EXT_0			= 0x4a,	/* XXX: figure out what this is... some input */
	ENVY_BIOS_GPIO_DP_EXT_1			= 0x4b,
	ENVY_BIOS_GPIO_ATX_POWER_BAD_ALT	= 0x4c,

	ENVY_BIOS_GPIO_TVDAC_0_LOAD_DETECT = 0x4d,  /* When the DAC 0 is not currently switched to a device that needs detection,
	                                               this GPIO pin can be used to detect the alternate display’s load on the green channel.
	                                               Seen, neg input [G84, G86] */

	ENVY_BIOS_GPIO_ANALOGIX_ENC_EXT_RESET = 0x4e, /* For Analogix encoder, a GPIO is used to control the RESET# line. */

	ENVY_BIOS_GPIO_I2C_SCL_KEEPER_CIRCUIT_ENABLE = 0x4f, /* OFF: Normal operation (do nothing); ON: Enable HW to detect slave-issued stretches on the SCL line and hold SCL low. */

	ENVY_BIOS_GPIO_DVI_DAC_SWITCH     = 0x50, /* OFF: DAC 0 (TV) routed to the DVI Connector; ON: DAC 1 (CRT) routed to the DVI Connector. */
	ENVY_BIOS_GPIO_HPD_2			= 0x51,
	ENVY_BIOS_GPIO_HPD_3			= 0x52,
	ENVY_BIOS_GPIO_DP_EXT_2   = 0x53,	/* 0x53 seen, neg input [G96] */
	ENVY_BIOS_GPIO_DP_EXT_3   = 0x54,

	ENVY_BIOS_GPIO_MAXIM_EXT_RESET    = 0x55, /* Enabled is Active Low so init value should be Active High [No inversions] */

	ENVY_BIOS_GPIO_SLI_LED_ACTIVE_DISPLAY = 0x56,	/* LED to indicate the GPU with active display in SLI mode. Seen, output [G92, G200] */

	ENVY_BIOS_GPIO_SPDIF_INPUT        = 0x57,
	ENVY_BIOS_GPIO_TOSLINK_INPUT      = 0x58,
	ENVY_BIOS_GPIO_AUDIO_SELECT       = 0x59, /* When GPIO is set LOW, SPDIF is selected. When GPIO is set HI, TOSLINK is selected. */

	ENVY_BIOS_GPIO_DPAUX_I2C_0        = 0x5a, /* Logical ON state, DPAUX will be selected. Logical OFF state selects I2C. Seen, neg output [G96] */
	ENVY_BIOS_GPIO_DPAUX_I2C_1        = 0x5b, /* Logical ON state, DPAUX will be selected. Logical OFF state selects I2C. */
	ENVY_BIOS_GPIO_DPAUX_I2C_2        = 0x5c, /* Logical ON state, DPAUX will be selected. Logical OFF state selects I2C. Seen, neg output [G94, G96, G98] */
	ENVY_BIOS_GPIO_DPAUX_I2C_3        = 0x5d, /* Logical ON state, DPAUX will be selected. Logical OFF state selects I2C. */

	ENVY_BIOS_GPIO_HPD_4			= 0x5e,
	ENVY_BIOS_GPIO_HPD_5			= 0x5f,
	ENVY_BIOS_GPIO_HPD_6			= 0x60,
	/* UNKNOWN 0x61 */
	/* UNKNOWN 0x62 */
	ENVY_BIOS_GPIO_EXT_DEV_0_INTRPT   = 0x63, /* Used to surface an interrupt from a GPIO external device */
	/* UNKNOWN 0x64 */
	/* UNKNOWN 0x65 */
	/* UNKNOWN 0x66 */
	/* UNKNOWN 0x67 */
	/* UNKNOWN 0x68 */
	/* UNKNOWN 0x69 */
	ENVY_BIOS_GPIO_SWITCHED_OUTPUTS   = 0x6a, /* Switched outputs GPIO must be processed by INIT_GPIO_ALL devinit opcode and set to its init state. Seen, output [MCP79] */

	ENVY_BIOS_GPIO_CUSTOMER_RW        = 0x6b, /* OEM reserved. */

	ENVY_BIOS_GPIO_MXM_3_0_PIN_26		= 0x6c,
	ENVY_BIOS_GPIO_MXM_3_0_PIN_28		= 0x6d,
	ENVY_BIOS_GPIO_MXM_3_0_PIN_30		= 0x6e,

	/* 0x6f seen, input [GT216, GT218] SPEC NVIO */
	ENVY_BIOS_GPIO_HW_PWR_SLOWDOWN		= 0x6f,

	ENVY_BIOS_GPIO_SWAP_RDY_SYNC_B    = 0x70, /* Signal, which is related to Fliplocking, is used to sense the state of the FET drain,
	                                             which is pulled high and is connected to Swap Ready pin on the Distributed Rendering Connector */

	ENVY_BIOS_GPIO_TRIGGER_PMU        = 0x71, /* Triggered either by system notify bit set in SBIOS postbox command register or an error entering into deep-idle. */

	ENVY_BIOS_GPIO_SWAP_RDY_OUT_B     = 0x72, /* Reserved for Swap Ready Out B */

	ENVY_BIOS_GPIO_VID_4			= 0x73,
	ENVY_BIOS_GPIO_VID_5			= 0x74,
	ENVY_BIOS_GPIO_VID_6			= 0x75,
	ENVY_BIOS_GPIO_VID_7			= 0x76,

	ENVY_BIOS_GPIO_LVDS_FAST_SWITCH_MUX = 0x77,

	/* 0x78 seen, output [GF100, GF106, GF104, GF110, GF114, GF116, GK104 */
	ENVY_BIOS_GPIO_FAN_FAILSAFE_PWM		= 0x78,
	/* 0x79 seen, neg input [GF100, GK104], uses unk41_line */
	ENVY_BIOS_GPIO_ATX_POWER_LOW		= 0x79,
	/* 0x7a seen, open-collector output [GK104] */
	ENVY_BIOS_GPIO_ATX_FORCE_LOW_PWR	= 0x7a,
	ENVY_BIOS_GPIO_FAN_OVERTEMP       = 0x7b, /* Pin will be driven from PWM source that has capability to MAX duty cycle based on the thermal ALERT signal,
	                                             as opposed to the already present "Fan" function which only outputs PWM.
	                                             This PWM source is independent from the pwm source for "Fan" function. */
	ENVY_BIOS_GPIO_POST_GPU_LED       = 0x7c, /* POSTed GPU LED to indicate the GPU that was POSTed by the SBIOS. */

	ENVY_BIOS_GPIO_RESERVED_7D		= 0x7d,
	ENVY_BIOS_GPIO_RESERVED_7E		= 0x7e,
	ENVY_BIOS_GPIO_RESERVED_7F		= 0x7f,

	ENVY_BIOS_GPIO_SMPBI_EVENT        = 0x80, /* Notifies the EC (or client of the SMBus Post Box Interface) of a pending GPU event requiring its attention. */
	/* 0x81 seen, output [GK106, GM107] */
	ENVY_BIOS_GPIO_VID_PWM			= 0x81,
	/* RESERVED 0x82 */
	/* 0x83 seen, input [GK104], SPEC - connected to PWM??? */
	ENVY_BIOS_GPIO_SLI_LED_PWM		= 0x83,
	ENVY_BIOS_GPIO_LOGO_LED_PWM		= 0x84,

	ENVY_BIOS_GPIO_PSR_FRAME_LOCK_A		= 0x85,
	ENVY_BIOS_GPIO_FB_CLAMP_EC_JT_MEM_SRFSH	= 0x86,
	ENVY_BIOS_GPIO_FB_CLAMP_TOGGLE_REQ	= 0x87,
	/* RESERVED 0x88 */
	/* RESERVED 0x89 */
	ENVY_BIOS_GPIO_LCD1_BACKLIGHT_ON	= 0x8a, /* LCD1 corresponds to the LCD1 defined in LCD ID field in Connector Table. */
	ENVY_BIOS_GPIO_LCD1_POWER 		    = 0x8b,
	ENVY_BIOS_GPIO_LCD1_POWER_STATUS  = 0x8c,
	ENVY_BIOS_GPIO_LCD1_SELF_TEST     = 0x8d,
	ENVY_BIOS_GPIO_LCD1_LAMP_STATUS   = 0x8e,
	ENVY_BIOS_GPIO_LCD1_BACKLIGHT_LEVEL = 0x8f,

	ENVY_BIOS_GPIO_LCD2_BACKLIGHT_ON	= 0x90, /* LCD2 corresponds to the LCD2 defined in LCD ID field in Connector Table. */
	ENVY_BIOS_GPIO_LCD2_POWER 		    = 0x91,
	ENVY_BIOS_GPIO_LCD2_POWER_STATUS  = 0x92,
	ENVY_BIOS_GPIO_LCD2_SELF_TEST     = 0x93,
	ENVY_BIOS_GPIO_LCD2_LAMP_STATUS   = 0x94,
	ENVY_BIOS_GPIO_LCD2_BACKLIGHT_LEVEL = 0x95,

	ENVY_BIOS_GPIO_LCD3_BACKLIGHT_ON	= 0x96, /* LCD3 corresponds to the LCD3 defined in LCD ID field in Connector Table. */
	ENVY_BIOS_GPIO_LCD3_POWER 		    = 0x97,
	ENVY_BIOS_GPIO_LCD3_POWER_STATUS  = 0x98,
	ENVY_BIOS_GPIO_LCD3_SELF_TEST     = 0x99,
	ENVY_BIOS_GPIO_LCD3_LAMP_STATUS   = 0x9a,
	ENVY_BIOS_GPIO_LCD3_BACKLIGHT_LEVEL = 0x9b,

	ENVY_BIOS_GPIO_LCD4_BACKLIGHT_ON	= 0x9c, /* LCD4 corresponds to the LCD4 defined in LCD ID field in Connector Table. */
	ENVY_BIOS_GPIO_LCD4_POWER 		    = 0x9d,
	ENVY_BIOS_GPIO_LCD4_POWER_STATUS  = 0x9e,
	ENVY_BIOS_GPIO_LCD4_SELF_TEST     = 0x9f,
	ENVY_BIOS_GPIO_LCD4_LAMP_STATUS   = 0xa0,
	ENVY_BIOS_GPIO_LCD4_BACKLIGHT_LEVEL = 0xa1,

	ENVY_BIOS_GPIO_LCD5_BACKLIGHT_ON	= 0xa2, /* LCD5 corresponds to the LCD5 defined in LCD ID field in Connector Table. */
	ENVY_BIOS_GPIO_LCD5_POWER 		    = 0xa3,
	ENVY_BIOS_GPIO_LCD5_POWER_STATUS  = 0xa4,
	ENVY_BIOS_GPIO_LCD5_SELF_TEST     = 0xa5,
	ENVY_BIOS_GPIO_LCD5_LAMP_STATUS   = 0xa6,
	ENVY_BIOS_GPIO_LCD5_BACKLIGHT_LEVEL = 0xa7,

	ENVY_BIOS_GPIO_LCD6_BACKLIGHT_ON	= 0xa8, /* LCD6 corresponds to the LCD6 defined in LCD ID field in Connector Table. */
	ENVY_BIOS_GPIO_LCD6_POWER 		    = 0xa9,
	ENVY_BIOS_GPIO_LCD6_POWER_STATUS  = 0xaa,
	ENVY_BIOS_GPIO_LCD6_SELF_TEST     = 0xab,
	ENVY_BIOS_GPIO_LCD6_LAMP_STATUS   = 0xac,
	ENVY_BIOS_GPIO_LCD6_BACKLIGHT_LEVEL = 0xad,

	ENVY_BIOS_GPIO_LCD7_BACKLIGHT_ON	= 0xae, /* LCD7 corresponds to the LCD7 defined in LCD ID field in Connector Table. */
	ENVY_BIOS_GPIO_LCD7_POWER 		    = 0xaf,
	ENVY_BIOS_GPIO_LCD7_POWER_STATUS  = 0xb0,
	ENVY_BIOS_GPIO_LCD7_SELF_TEST     = 0xb1,
	ENVY_BIOS_GPIO_LCD7_LAMP_STATUS   = 0xb2,
	ENVY_BIOS_GPIO_LCD7_BACKLIGHT_LEVEL = 0xb3,
	/* RESERVED 0xb4 */

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
	ENVY_BIOS_CONN_WFD			= 0x70,
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

struct envy_bios_mem_unk0d_entry {
	uint16_t offset;
};

struct envy_bios_mem_unk0d {
	uint16_t offset;
	uint8_t valid;
	uint8_t version;
	uint8_t hlen;
	uint8_t entriesnum;
	uint8_t rlen;

	struct envy_bios_mem_unk0d_entry *entries;
};

struct envy_bios_mem_unk11_entry {
	uint16_t offset;
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
	struct envy_bios_mem_unk0d unk0d;
	uint16_t trestrict;
};

struct envy_bios_power_volt {
	uint32_t offset;
};

enum envy_bios_power_therm_devices_class_id {
	ENVY_BIOS_THERM_DEVICE_GPU = 0x01,
};

struct envy_bios_power_therm_devices_entry {
	uint32_t offset;
	uint8_t class_id;
	uint8_t i2c_device;
	uint8_t flags;
};

struct envy_bios_power_therm_devices {
	uint32_t offset;
	uint8_t valid;
	uint8_t version;
	uint8_t hlen;
	uint8_t entriesnum;
	uint8_t rlen;

	struct envy_bios_power_therm_devices_entry *entries;
};

struct envy_bios_power_fan_calib_entry {
	uint32_t offset;
	uint8_t enable;
	uint8_t mode;

	uint16_t unk02;
	uint16_t unk04;
	uint16_t unk06;
	uint16_t pwm_freq;
	uint16_t duty_max;
	int16_t duty_offset;
	int16_t unk0e;
	uint16_t unk10;
	uint16_t unk12;
};

struct envy_bios_power_fan_calib {
	uint32_t offset;
	uint8_t valid;
	uint8_t version;
	uint8_t hlen;
	uint8_t entriesnum;
	uint8_t rlen;

	struct envy_bios_power_fan_calib_entry *entries;
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

struct envy_bios_power_sense_resistor {
	uint8_t mohm;
	uint8_t unk;
};

struct envy_bios_power_sense_entry {
	uint32_t offset;
	uint8_t mode;
	uint8_t extdev_id;

	union {
		struct {
			struct envy_bios_power_sense_resistor res;
			uint16_t config;
		} ina219;
		struct {
			struct envy_bios_power_sense_resistor res[3];
			uint16_t config;
		} ina3221;
		uint8_t raw[0x10];
	} d;
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
	uint16_t t0;
	uint16_t t1;
	uint16_t t2;
	uint16_t interval_us;
	 int16_t down_off;
	 int16_t up_off;
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

struct envy_bios_power_unk68_entry {
	uint32_t offset;
};

struct envy_bios_power_unk68 {
	uint32_t offset;
	uint8_t valid;
	uint8_t version;
	uint8_t hlen;
	uint8_t entriesnum;
	uint8_t rlen;

	struct envy_bios_power_unk68_entry *entries;
};

struct envy_bios_power_unk6c_entry {
	uint32_t offset;
};

struct envy_bios_power_unk6c {
	uint32_t offset;
	uint8_t valid;
	uint8_t version;
	uint8_t hlen;
	uint8_t entriesnum;
	uint8_t rlen;

	struct envy_bios_power_unk6c_entry *entries;
};

struct envy_bios_power_unk70_entry {
	uint32_t offset;
};

struct envy_bios_power_unk70 {
	uint32_t offset;
	uint8_t valid;
	uint8_t version;
	uint8_t hlen;
	uint8_t entriesnum;
	uint8_t rlen;

	struct envy_bios_power_unk70_entry *entries;
};

struct envy_bios_power_unk74_entry {
	uint32_t offset;
};

struct envy_bios_power_unk74 {
	uint32_t offset;
	uint8_t valid;
	uint8_t version;
	uint8_t hlen;
	uint8_t entriesnum;
	uint8_t rlen;

	struct envy_bios_power_unk74_entry *entries;
};

struct envy_bios_power_unk78_entry {
	uint32_t offset;
};

struct envy_bios_power_unk78 {
	uint32_t offset;
	uint8_t valid;
	uint8_t version;
	uint8_t hlen;
	uint8_t entriesnum;
	uint8_t rlen;

	struct envy_bios_power_unk78_entry *entries;
};

struct envy_bios_power_unk7c_entry {
	uint32_t offset;
};

struct envy_bios_power_unk7c {
	uint32_t offset;
	uint8_t valid;
	uint8_t version;
	uint8_t hlen;
	uint8_t entriesnum;
	uint8_t rlen;

	struct envy_bios_power_unk7c_entry *entries;
};

struct envy_bios_power_unk80_entry {
	uint32_t offset;
};

struct envy_bios_power_unk80 {
	uint32_t offset;
	uint8_t valid;
	uint8_t version;
	uint8_t hlen;
	uint8_t entriesnum;
	uint8_t rlen;

	struct envy_bios_power_unk80_entry *entries;
};

struct envy_bios_power_unk84_entry {
	uint32_t offset;
};

struct envy_bios_power_unk84 {
	uint32_t offset;
	uint8_t valid;
	uint8_t version;
	uint8_t hlen;
	uint8_t entriesnum;
	uint8_t rlen;

	struct envy_bios_power_unk84_entry *entries;
};

struct envy_bios_power_unk88_entry {
	uint32_t offset;
};

struct envy_bios_power_unk88 {
	uint32_t offset;
	uint8_t valid;
	uint8_t version;
	uint8_t hlen;
	uint8_t entriesnum;
	uint8_t rlen;

	struct envy_bios_power_unk88_entry *entries;
};

struct envy_bios_power_unk8c_entry {
	uint32_t offset;
};

struct envy_bios_power_unk8c {
	uint32_t offset;
	uint8_t valid;
	uint8_t version;
	uint8_t hlen;
	uint8_t entriesnum;
	uint8_t rlen;

	struct envy_bios_power_unk8c_entry *entries;
};

struct envy_bios_power_unk90_entry {
	uint32_t offset;
};

struct envy_bios_power_unk90 {
	uint32_t offset;
	uint8_t valid;
	uint8_t version;
	uint8_t hlen;
	uint8_t entriesnum;
	uint8_t rlen;

	struct envy_bios_power_unk90_entry *entries;
};

struct envy_bios_power_unk94_entry {
	uint32_t offset;
};

struct envy_bios_power_unk94 {
	uint32_t offset;
	uint8_t valid;
	uint8_t version;
	uint8_t hlen;
	uint8_t entriesnum;
	uint8_t rlen;

	struct envy_bios_power_unk94_entry *entries;
};

struct envy_bios_power_unk98_entry {
	uint32_t offset;
};

struct envy_bios_power_unk98 {
	uint32_t offset;
	uint8_t valid;
	uint8_t version;
	uint8_t hlen;
	uint8_t entriesnum;
	uint8_t rlen;

	struct envy_bios_power_unk98_entry *entries;
};

struct envy_bios_power {
	struct envy_bios_bit_entry *bit;

	struct envy_bios_power_perf perf;
	struct envy_bios_power_timing_map timing_map;
	struct envy_bios_power_timing timing;
	struct envy_bios_power_therm therm;
	struct envy_bios_power_volt volt;
	struct envy_bios_power_therm_devices therm_devices;
	struct envy_bios_power_fan_calib fan_calib;
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
	struct envy_bios_power_unk68 unk68;
	struct envy_bios_power_unk6c unk6c;
	struct envy_bios_power_unk70 unk70;
	struct envy_bios_power_unk74 unk74;
	struct envy_bios_power_unk78 unk78;
	struct envy_bios_power_unk7c unk7c;
	struct envy_bios_power_unk80 unk80;
	struct envy_bios_power_unk84 unk84;
	struct envy_bios_power_unk88 unk88;
	struct envy_bios_power_unk8c unk8c;
	struct envy_bios_power_unk90 unk90;
	struct envy_bios_power_unk94 unk94;
	struct envy_bios_power_unk98 unk98;
};

struct envy_bios_D_unk0_entry {
	uint16_t offset;
};

struct envy_bios_D_unk0 {
	uint16_t offset;
	uint8_t valid;
	uint8_t version;
	uint8_t hlen;
	uint8_t entriesnum;
	uint8_t rlen;

	struct envy_bios_D_unk0_entry *entries;
};

struct envy_bios_D_unk2_entry {
	uint16_t offset;
};

struct envy_bios_D_unk2 {
	uint16_t offset;
	uint8_t valid;
	uint8_t version;
	uint8_t hlen;
	uint8_t entriesnum;
	uint8_t rlen;

	struct envy_bios_D_unk2_entry *entries;
};

struct envy_bios_D {
	struct envy_bios_bit_entry *bit;

	struct envy_bios_D_unk0 unk0;
	struct envy_bios_D_unk2 unk2;
};

struct envy_bios_L_unk0_entry {
	uint16_t offset;
};

struct envy_bios_L_unk0 {
	uint16_t offset;
	uint8_t valid;
	uint8_t version;
	uint8_t hlen;
	uint8_t entriesnum;
	uint8_t rlen;

	struct envy_bios_L_unk0_entry *entries;
};

struct envy_bios_L {
	struct envy_bios_bit_entry *bit;

	struct envy_bios_L_unk0 unk0;
};

struct envy_bios_T_unk0_entry {
	uint16_t offset;
};

struct envy_bios_T_unk0 {
	uint16_t offset;
	uint8_t valid;
	uint8_t version;
	uint8_t hlen;
	uint8_t entriesnum;
	uint8_t rlen;

	struct envy_bios_T_unk0_entry *entries;
};

struct envy_bios_T {
	struct envy_bios_bit_entry *bit;

	struct envy_bios_T_unk0 unk0;
};

enum envy_bios_d_dp_info_link_rate {
	ENVY_BIOS_DP_INFO_LINK_RATE_162 = 0x06,
	ENVY_BIOS_DP_INFO_LINK_RATE_270 = 0x0a,
	ENVY_BIOS_DP_INFO_LINK_RATE_540 = 0x14,
	ENVY_BIOS_DP_INFO_LINK_RATE_810 = 0x1e,
};

struct envy_bios_d_dp_info_before_link_speed {
	uint16_t offset;
	uint8_t valid;
	uint8_t link_rate;
	uint16_t link_rate_ptr;
};

struct envy_bios_d_dp_info_entry {
	uint16_t offset;
	uint8_t valid;
	uint32_t key;
	uint8_t flags;
	uint16_t before_link_training;
	uint16_t after_link_training;
	uint16_t before_link_speed_0;
	uint16_t enable_spread;
	uint16_t disable_spread;
	uint16_t disable_lt;
	uint8_t level_entry_table_index;
	uint8_t hbr2_min_vdt_index;

	int before_link_speed_nums;

	struct envy_bios_d_dp_info_before_link_speed *before_link_speed_entries;
};

struct envy_bios_d_dp_info_level_entry {
	uint16_t offset;
	uint8_t valid;
	uint8_t post_cursor_2;
	uint8_t drive_current;
	uint8_t pre_emphasis;
	uint8_t tx_pu;
};

struct envy_bios_d_dp_info_level_entry_table {
	uint16_t offset;

	struct envy_bios_d_dp_info_level_entry *level_entries;
};

struct envy_bios_d_dp_info {
	uint16_t offset;
	uint8_t valid;
	uint8_t version;
	uint8_t hlen;
	uint8_t entriesnum;
	uint8_t rlen;
	uint8_t target_size;
	uint8_t levelentrytables_count;
	uint8_t levelentry_size;
	uint8_t levelentry_count;
	uint8_t flags;
	uint16_t regular_vswing;
	uint16_t low_vswing;

	struct envy_bios_d_dp_info_entry *entries;
	struct envy_bios_d_dp_info_level_entry_table *level_entry_tables;
};

struct envy_bios_d {
	struct envy_bios_bit_entry *bit;

	struct envy_bios_d_dp_info dp_info;
};

struct envy_bios_p_falcon_ucode_desc {
	uint32_t offset;
	uint8_t valid;
	uint32_t stored_size;
	uint32_t uncompressed_size;
	uint32_t virtual_entry;
	uint32_t interface_offset;
	uint32_t imem_phys_base;
	uint32_t imem_load_size;
	uint32_t imem_virt_base;
	uint32_t imem_sec_base;
	uint32_t imem_sec_size;
	uint32_t dmem_offset;
	uint32_t dmem_phys_base;
	uint32_t dmem_load_size;
};

enum envy_bios_p_falcon_ucode_application_id {
	ENVY_BIOS_FALCON_UCODE_APPLICATION_ID_PRE_OS  = 0x01,
	ENVY_BIOS_FALCON_UCODE_APPLICATION_ID_DEVINIT = 0x04,
};

enum envy_bios_p_falcon_ucode_target_id {
	ENVY_BIOS_FALCON_UCODE_TARGET_ID_PMU = 0x01,
};

struct envy_bios_p_falcon_ucode_entry {
	uint16_t offset;
	uint8_t application_id;
	uint8_t target_id;
	uint32_t desc_ptr;

	struct envy_bios_p_falcon_ucode_desc desc;
};

struct envy_bios_p_falcon_ucode {
	uint16_t offset;
	uint8_t valid;
	uint8_t version;
	uint8_t hlen;
	uint8_t entriesnum;
	uint8_t rlen;
	uint8_t desc_version;
	uint8_t desc_size;

	struct envy_bios_p_falcon_ucode_entry *entries;
};

struct envy_bios_p {
	struct envy_bios_bit_entry *bit;

	struct envy_bios_p_falcon_ucode falcon_ucode;
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
	struct envy_bios_D D;
	struct envy_bios_L L;
	struct envy_bios_T T;
	struct envy_bios_d d;
	struct envy_bios_p p;

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
#define ENVY_BIOS_PRINT_D		0x00002000
#define ENVY_BIOS_PRINT_L		0x00004000
#define ENVY_BIOS_PRINT_HWSQ		0x00008000
#define ENVY_BIOS_PRINT_DCB		0x00010000
#define ENVY_BIOS_PRINT_GPIO		0x00020000
#define ENVY_BIOS_PRINT_I2C		0x00040000
#define ENVY_BIOS_PRINT_EXTDEV		0x00080000
#define ENVY_BIOS_PRINT_CONN		0x00100000
#define ENVY_BIOS_PRINT_MUX		0x00200000
#define ENVY_BIOS_PRINT_DUNK		0x00400000
#define ENVY_BIOS_PRINT_DCB_ALL		0x007f0000
#define ENVY_BIOS_PRINT_T		0x00800000
#define ENVY_BIOS_PRINT_d		0x01000000
#define ENVY_BIOS_PRINT_p		0x02000000
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
void envy_bios_print_power_therm_devices(struct envy_bios *bios, FILE *out, unsigned mask);
void envy_bios_print_power_fan_calib(struct envy_bios *bios, FILE *out, unsigned mask);
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
void envy_bios_print_power_unk68(struct envy_bios *bios, FILE *out, unsigned mask);
void envy_bios_print_power_unk6c(struct envy_bios *bios, FILE *out, unsigned mask);
void envy_bios_print_power_unk70(struct envy_bios *bios, FILE *out, unsigned mask);
void envy_bios_print_power_unk74(struct envy_bios *bios, FILE *out, unsigned mask);
void envy_bios_print_power_unk78(struct envy_bios *bios, FILE *out, unsigned mask);
void envy_bios_print_power_unk7c(struct envy_bios *bios, FILE *out, unsigned mask);
void envy_bios_print_power_unk80(struct envy_bios *bios, FILE *out, unsigned mask);
void envy_bios_print_power_unk84(struct envy_bios *bios, FILE *out, unsigned mask);
void envy_bios_print_power_unk88(struct envy_bios *bios, FILE *out, unsigned mask);
void envy_bios_print_power_unk8c(struct envy_bios *bios, FILE *out, unsigned mask);
void envy_bios_print_power_unk90(struct envy_bios *bios, FILE *out, unsigned mask);
void envy_bios_print_power_unk94(struct envy_bios *bios, FILE *out, unsigned mask);
void envy_bios_print_power_unk98(struct envy_bios *bios, FILE *out, unsigned mask);

int envy_bios_parse_bit_p (struct envy_bios *bios, struct envy_bios_bit_entry *bit);
void envy_bios_print_bit_p (struct envy_bios *bios, FILE *out, unsigned mask);
void envy_bios_print_p_falcon_ucode(struct envy_bios *bios, FILE *out, unsigned mask);

int envy_bios_parse_bit_M (struct envy_bios *bios, struct envy_bios_bit_entry *bit);
void envy_bios_print_bit_M (struct envy_bios *bios, FILE *out, unsigned mask);
void envy_bios_print_mem_train(struct envy_bios *bios, FILE *out, unsigned mask);
void envy_bios_print_mem_train_ptrn(struct envy_bios *bios, FILE *out, unsigned mask);
void envy_bios_print_mem_type(struct envy_bios *bios, FILE *out, unsigned mask);
void envy_bios_print_mem_unk0d(struct envy_bios *bios, FILE *out, unsigned mask);

int envy_bios_parse_bit_D (struct envy_bios *, struct envy_bios_bit_entry *);
void envy_bios_print_bit_D (struct envy_bios *, FILE *out, unsigned mask);
void envy_bios_print_D_unk2(struct envy_bios *, FILE *out, unsigned mask);

int envy_bios_parse_bit_L(struct envy_bios *, struct envy_bios_bit_entry *);
void envy_bios_print_bit_L(struct envy_bios *, FILE *out, unsigned mask);
void envy_bios_print_L_unk0(struct envy_bios *, FILE *out, unsigned mask);

int envy_bios_parse_bit_T(struct envy_bios *, struct envy_bios_bit_entry *);
void envy_bios_print_bit_T(struct envy_bios *, FILE *out, unsigned mask);
void envy_bios_print_T_unk0(struct envy_bios *, FILE *out, unsigned mask);

int envy_bios_parse_bit_d(struct envy_bios *, struct envy_bios_bit_entry *);
void envy_bios_print_bit_d(struct envy_bios *, FILE *out, unsigned mask);
void envy_bios_print_d_dp_info(struct envy_bios *, FILE *out, unsigned mask);

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
