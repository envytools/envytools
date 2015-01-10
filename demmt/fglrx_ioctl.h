#ifndef FGLRX_IOCTL_H
#define FGLRX_IOCTL_H

#define FGLRX_IOCTL_MAGIC 0x64

struct fglrx_ioctl_drv_info
{
	uint32_t unk00;
	uint32_t unk04;
	uint32_t unk08;
	uint32_t unk0c;
	uint32_t drv_id_len;
	uint32_t _pad14;
	uint64_t drv_id;
	uint32_t drv_date_len;
	uint32_t _pad24;
	uint64_t drv_date;
	uint32_t drv_name_len;
	uint32_t _pad34;
	uint64_t drv_name;
};
#define FGLRX_IOCTL_DRV_INFO _IOWR(FGLRX_IOCTL_MAGIC, 0x00, struct fglrx_ioctl_drv_info)

struct fglrx_ioctl_get_bus_id
{
	uint32_t bus_id_len;
	uint32_t _pad04;
	uint64_t bus_id;
};
#define FGLRX_IOCTL_GET_BUS_ID _IOWR(FGLRX_IOCTL_MAGIC, 0x01, struct fglrx_ioctl_get_bus_id)

struct fglrx_ioctl_02
{
	uint32_t unk00;
};
#define FGLRX_IOCTL_02 _IOR( FGLRX_IOCTL_MAGIC, 0x02, struct fglrx_ioctl_02)

struct fglrx_ioctl_07
{
	uint32_t unk00;
	uint32_t unk04;
	uint32_t unk08;
	uint32_t unk0c;
};
#define FGLRX_IOCTL_07 _IOWR(FGLRX_IOCTL_MAGIC, 0x07, struct fglrx_ioctl_07)

struct fglrx_ioctl_20
{
	uint32_t unk00;
	uint32_t unk04;
};
#define FGLRX_IOCTL_20 _IOWR(FGLRX_IOCTL_MAGIC, 0x20, struct fglrx_ioctl_20)

struct fglrx_ioctl_2a
{
	uint32_t unk00;
	uint32_t unk04;
};
#define FGLRX_IOCTL_2A _IOW( FGLRX_IOCTL_MAGIC, 0x2a, struct fglrx_ioctl_2a)

struct fglrx_ioctl_2b
{
	uint32_t unk00;
	uint32_t unk04;
};
#define FGLRX_IOCTL_2B _IOW( FGLRX_IOCTL_MAGIC, 0x2b, struct fglrx_ioctl_2b)

struct fglrx_ioctl_4f
{
	uint32_t unk00;
	uint32_t unk04;
	uint32_t unk08;
	uint32_t unk0c;
	uint32_t unk10;
	uint32_t unk14;
	uint32_t unk18;
	uint32_t unk1c;
	uint64_t ptr1;
};
#define FGLRX_IOCTL_4F _IOW( FGLRX_IOCTL_MAGIC, 0x4f, struct fglrx_ioctl_4f)

struct fglrx_ioctl_50
{
	uint32_t kernel_ver_len;
	uint32_t _pad04;
	uint64_t kernel_ver;
	uint32_t unk10;
	uint32_t unk14;
	uint32_t unk18;
	uint32_t unk1c;
	uint32_t unk20;
	uint32_t unk24;
	uint32_t unk28;
	uint32_t unk2c;
	uint32_t unk30;
	uint32_t unk34;
	uint32_t unk38;
	uint32_t unk3c;
	uint32_t unk40;
	uint32_t unk44;
	uint32_t unk48;
	uint32_t unk4c;
	uint32_t unk50;
	uint32_t unk54;
};
#define FGLRX_IOCTL_50 _IOWR(FGLRX_IOCTL_MAGIC, 0x50, struct fglrx_ioctl_50)

struct fglrx_ioctl_54
{
	uint32_t unk00;
	uint32_t unk04;
	uint32_t unk08;
	uint32_t unk0c;
	uint32_t unk10;
};
#define FGLRX_IOCTL_54 _IOR( FGLRX_IOCTL_MAGIC, 0x54, struct fglrx_ioctl_54)

struct fglrx_ioctl_58
{
	uint32_t unk00;
};
#define FGLRX_IOCTL_58 _IOW( FGLRX_IOCTL_MAGIC, 0x58, struct fglrx_ioctl_58)

struct fglrx_ioctl_64
{
	uint64_t ptr1;
	uint32_t unk08;
	uint32_t unk0c;
};
#define FGLRX_IOCTL_64 _IOWR(FGLRX_IOCTL_MAGIC, 0x64, struct fglrx_ioctl_64)

struct fglrx_ioctl_68
{
	uint64_t ptr1;
	uint64_t ptr2;
	uint32_t unk10;
	uint32_t unk14;
	uint32_t unk18;
	uint32_t unk1c;
	uint32_t unk20;
	uint32_t unk24;
};
#define FGLRX_IOCTL_68 _IOWR(FGLRX_IOCTL_MAGIC, 0x68, struct fglrx_ioctl_68)

struct fglrx_ioctl_69
{
	uint32_t unk00;
	uint32_t unk04;
	uint32_t unk08;
	uint32_t unk0c;
};
#define FGLRX_IOCTL_69 _IOWR(FGLRX_IOCTL_MAGIC, 0x69, struct fglrx_ioctl_69)

struct fglrx_ioctl_config
{
	uint32_t unk00;
	uint32_t unk04;
	uint32_t unk08;
	uint32_t username_len;
	uint64_t username;//WTF?
	uint32_t namespace_len;
	uint32_t _pad1c;
	uint64_t namespace;
	uint32_t prop_name_len;
	uint32_t _pad2c;
	uint64_t prop_name;
	uint32_t unk38;
	uint32_t unk3c;
	uint32_t unk40;
	uint32_t unk44;
	uint32_t unk48;
	uint32_t unk4c;
	uint32_t prop_value_len;
	uint32_t _pad54;
	uint64_t prop_value;
};
#define FGLRX_IOCTL_CONFIG _IOWR(FGLRX_IOCTL_MAGIC, 0x6e, struct fglrx_ioctl_config)

struct fglrx_ioctl_84
{
	uint32_t unk00;
};
#define FGLRX_IOCTL_84 _IOR( FGLRX_IOCTL_MAGIC, 0x84, struct fglrx_ioctl_84)

struct fglrx_ioctl_a6
{
	uint32_t unk00;
	uint32_t unk04;
	uint32_t len1;
	uint32_t _pad0c;
	uint64_t ptr1;
	uint32_t len2;
	uint32_t _pad1c;
	uint64_t ptr2;
	uint32_t unk28;
	uint32_t unk2c;
};
#define FGLRX_IOCTL_A6 _IOWR(FGLRX_IOCTL_MAGIC, 0xa6, struct fglrx_ioctl_a6)

#endif
