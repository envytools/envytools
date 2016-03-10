/*
 * Copyright (C) 2010-2011 Marcin Kościelnicki <koriakin@0x04.net>
 * Copyright (C) 2011 Martin Peres <martin.peres@ensi-bourges.fr>
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

#include <pciaccess.h>
#include <stdio.h>
#include <stdlib.h>
#include "nva.h"
#include "util.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <dirent.h>
#include <string.h>
#include <fcntl.h>

#define PLATFORM_DEVICES_DIR "/sys/bus/platform/devices"

struct nva_card **nva_cards = 0;
int nva_cardsnum = 0;
int nva_cardsmax = 0;
int nva_vgaarberr = 0;

struct pci_id_match nv_gpu_match[] = {
	{0x104a, 0x0009, PCI_MATCH_ANY, PCI_MATCH_ANY, 0, 0},
	{0x12d2, PCI_MATCH_ANY, PCI_MATCH_ANY, PCI_MATCH_ANY, 0x30000, 0xffff0000},
	{0x10de, PCI_MATCH_ANY, PCI_MATCH_ANY, PCI_MATCH_ANY, 0x30000, 0xffff0000},
	{0x10de, PCI_MATCH_ANY, PCI_MATCH_ANY, PCI_MATCH_ANY, 0x48000, 0xffffff00},
};

struct pci_id_match nv_apu_match[] = {
	{0x10de, 0x01b0, PCI_MATCH_ANY, PCI_MATCH_ANY, 0, 0},
	{0x10de, 0x006b, PCI_MATCH_ANY, PCI_MATCH_ANY, 0, 0},
};

struct pci_id_match nv_smu_match[] = {
	{0x10de, PCI_MATCH_ANY, PCI_MATCH_ANY, PCI_MATCH_ANY, 0xb4000, 0xffffff00},
};

struct pci_id_match nv_eth_match[] = {
	{0x10de, PCI_MATCH_ANY, PCI_MATCH_ANY, PCI_MATCH_ANY, 0x20000, 0xffffff00},
};

struct nva_card *nva_init_gpu(struct pci_device *dev) {
	struct nva_card *card = calloc(sizeof *card, 1);
	if (!card)
		return 0;
	card->type = NVA_DEVICE_GPU;
	card->bus_type = NVA_BUS_PCI;
	card->bus.pci = dev;
	int ret = pci_device_map_range(dev, dev->regions[0].base_addr, dev->regions[0].size, PCI_DEV_MAP_FLAG_WRITABLE, &card->bar0);
	if (ret) {
		fprintf (stderr, "WARN: Can't probe %04x:%02x:%02x.%x\n", dev->domain, dev->bus, dev->dev, dev->func);
		free(card);
		return 0;
	}
	card->bar0len = dev->regions[0].size;
	if (dev->regions[1].size) {
		card->hasbar1 = 1;
		card->bar1len = dev->regions[1].size;
		ret = pci_device_map_range(dev, dev->regions[1].base_addr, dev->regions[1].size, PCI_DEV_MAP_FLAG_WRITABLE, &card->bar1);
		if (ret) {
			card->bar1 = 0;
		}
	}
	if (dev->regions[2].size && !dev->regions[2].is_IO) {
		card->hasbar2 = 1;
		card->bar2len = dev->regions[2].size;
		ret = pci_device_map_range(dev, dev->regions[2].base_addr, dev->regions[2].size, PCI_DEV_MAP_FLAG_WRITABLE, &card->bar2);
		if (ret) {
			card->bar2 = 0;
		}
	} else if (dev->regions[3].size) {
		card->hasbar2 = 1;
		card->bar2len = dev->regions[3].size;
		ret = pci_device_map_range(dev, dev->regions[3].base_addr, dev->regions[3].size, PCI_DEV_MAP_FLAG_WRITABLE, &card->bar2);
		if (ret) {
			card->bar2 = 0;
		}
	}
	/* ignore errors */
	pci_device_map_legacy(dev, 0, 0x100000, PCI_DEV_MAP_FLAG_WRITABLE, &card->rawmem);
	card->rawio = pci_legacy_open_io(dev, 0, 0x10000);
	int iobar = -1;
	if (dev->regions[2].size && dev->regions[2].is_IO)
		iobar = 2;
	if (dev->regions[5].size && dev->regions[5].is_IO)
		iobar = 5;
	if (iobar != -1) {
		card->iobar = pci_device_open_io(dev, dev->regions[iobar].base_addr, dev->regions[iobar].size);
		card->iobarlen = dev->regions[iobar].size;
	}
	uint32_t pmc_id = nva_grd32(card->bar0, 0);
	parse_pmc_id(pmc_id, &card->chipset);
	return card;
}

struct nva_card *nva_init_smu(struct pci_device *dev) {
	struct nva_card *card = calloc(sizeof *card, 1);
	if (!card)
		return 0;
	card->type = NVA_DEVICE_SMU;
	card->bus_type = NVA_BUS_PCI;
	card->bus.pci = dev;
	int ret = pci_device_map_range(dev, dev->regions[0].base_addr, dev->regions[0].size, PCI_DEV_MAP_FLAG_WRITABLE, &card->bar0);
	if (ret) {
		fprintf (stderr, "WARN: Can't probe %04x:%02x:%02x.%x\n", dev->domain, dev->bus, dev->dev, dev->func);
		free(card);
		return 0;
	}
	card->bar0len = dev->regions[0].size;
	return card;
}

struct nva_card *nva_init_apu(struct pci_device *dev) {
	struct nva_card *card = calloc(sizeof *card, 1);
	if (!card)
		return 0;
	card->type = NVA_DEVICE_APU;
	card->bus_type = NVA_BUS_PCI;
	card->bus.pci = dev;
	int ret = pci_device_map_range(dev, dev->regions[0].base_addr, dev->regions[0].size, PCI_DEV_MAP_FLAG_WRITABLE, &card->bar0);
	if (ret) {
		fprintf (stderr, "WARN: Can't probe %04x:%02x:%02x.%x\n", dev->domain, dev->bus, dev->dev, dev->func);
		free(card);
		return 0;
	}
	card->bar0len = dev->regions[0].size;
	return card;
}

struct nva_card *nva_init_eth(struct pci_device *dev) {
	struct nva_card *card = calloc(sizeof *card, 1);
	if (!card)
		return 0;
	card->type = NVA_DEVICE_ETH;
	card->bus_type = NVA_BUS_PCI;
	card->bus.pci = dev;
	int ret = pci_device_map_range(dev, dev->regions[0].base_addr, dev->regions[0].size, PCI_DEV_MAP_FLAG_WRITABLE, &card->bar0);
	if (ret) {
		fprintf (stderr, "WARN: Can't probe %04x:%02x:%02x.%x\n", dev->domain, dev->bus, dev->dev, dev->func);
		free(card);
		return 0;
	}
	card->bar0len = dev->regions[0].size;
	return card;
}

static int check_modalias(const char *node_name) {
	char path[PATH_MAX];
	char buffer[255] = { 0 };
	FILE *file;

	/* Check if the GPU is indeed made by NVIDIA by checking the compatibility
	 * field of the device tree, exposed to the userspace via modalias. As
	 * we are lazy, let's not parse the modalias string but just check if
	 * it mentions nvidia at all. If it does, return 0.
	 */

	snprintf(path, PATH_MAX, "%s/%s/modalias", PLATFORM_DEVICES_DIR, node_name);
	if (!(file = fopen(path, "r"))) {
		return 2;
	}
	fread(buffer, 1, sizeof(buffer), file);
	fclose(file);
	return strstr(buffer, "nvidia") == NULL;
}

struct nva_card *nva_init_platform() {
	uint32_t tmp, bar0 = 0xffffffff;
	struct nva_card *card;
	struct dirent *dent;
	DIR *dir;
	int fd;

	/* Look for a gpu on the "platform" bus by looking at the platform bus,
	 * looking for an entry such as '/sys/bus/platform/devices/57000000.gpu'.
	 * We only expect up to one entry. This will not pick up the GPUs older
	 * than the tk1 because there was no "gpu" node before that, only gr2d
	 * and gr3d.
	 */
	if (!(dir = opendir(PLATFORM_DEVICES_DIR)))
		return NULL;
	while ((dent = readdir(dir))) {
		/* FIXME: this may not work on downstream kernels */
		const char *last_four = dent->d_name + strlen(dent->d_name) - 4;
		if (strcmp(last_four, ".gpu") == 0) {
			if (sscanf(dent->d_name, "%x.gpu", &tmp) == 1) {
				/* Check if the modalias contains nvidia */
				if (check_modalias(dent->d_name))
					continue;

				/* We have found the gpu, exit the loop */
				bar0 = tmp;
				break;
			}
		}
	}

	if (bar0 == 0xffffffff)
		return NULL;

	/* we have found a gpu */
	card = calloc(sizeof *card, 1);
	if (!card)
                return 0;
	card->type = NVA_DEVICE_GPU;
	card->bus_type = NVA_BUS_PLATFORM;
	card->bus.platform_address = bar0;

	if((fd = open("/dev/mem", O_RDWR | O_SYNC)) == -1) {
		fprintf (stderr, "WARN: Can't open /dev/mem. Are you root?\n");
		free(card);
		return 0;
	}

	card->bar0 = mmap(0, 0x1000000, PROT_READ | PROT_WRITE, MAP_SHARED, fd, bar0);
	if (!card->bar0) {
		fprintf (stderr, "WARN: Can't map the physical address 0x%x\n", bar0);
		free(card);
		return 0;
	}
	card->bar0len = 0x1000000;

	uint32_t pmc_id = nva_grd32(card->bar0, 0);
	parse_pmc_id(pmc_id, &card->chipset);

	/* Enable bar1 for the devices we know because I cannot figure out how
	 * to read the relevant part of the DT via sysfs...
	 */
	if (card->chipset.chipset == 0xea) {
		card->hasbar1 = 1;
		card->bar1len = 0x1000000;
		card->bar1 = mmap(0, 0x1000000, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0x58000000);
	}

	return card;
}

struct {
	struct pci_id_match *match;
	size_t len;
	struct nva_card *(*func) (struct pci_device *dev);
} nva_types[] = {
	{ nv_gpu_match, ARRAY_SIZE(nv_gpu_match), nva_init_gpu },
	{ nv_smu_match, ARRAY_SIZE(nv_smu_match), nva_init_smu },
	{ nv_apu_match, ARRAY_SIZE(nv_apu_match), nva_init_apu },
	{ nv_eth_match, ARRAY_SIZE(nv_eth_match), nva_init_eth },
};

int nva_init() {
	int ret;
	ret = pci_system_init();
	if (ret)
		return -1;
	nva_vgaarberr = pci_device_vgaarb_init();
	int i, j;

	for (i = 0; i < ARRAY_SIZE(nva_types); i++) {
		for (j = 0; j < nva_types[i].len; j++) {
			struct pci_device_iterator* it = pci_id_match_iterator_create(&nva_types[i].match[j]);
			if (!it) {
				pci_system_cleanup();
				return -1;
			}

			struct pci_device *dev;
			while ((dev = pci_device_next(it))) {
				ret = pci_device_probe(dev);
				if (ret) {
					fprintf (stderr, "WARN: Can't probe %04x:%02x:%02x.%x\n", dev->domain, dev->bus, dev->dev, dev->func);
					continue;
				}
				pci_device_enable(dev);
				struct nva_card *card = nva_types[i].func(dev);
				if (card)
					ADDARRAY(nva_cards, card);
			}
			pci_iterator_destroy(it);
		}
	}

	/* Finally, try reading the platform */
	struct nva_card *card = nva_init_platform();
	if (card)
		ADDARRAY(nva_cards, card);

	return (nva_cardsnum == 0);
}
