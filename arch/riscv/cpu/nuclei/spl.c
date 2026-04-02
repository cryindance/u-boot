// SPDX-License-Identifier: GPL-2.0+

#include <dm.h>
#include <log.h>
#include <asm/csr.h>

int spl_soc_init(void)
{
	int ret;
	struct udevice *dev;

	/* DDR init */
	ret = uclass_get_device(UCLASS_RAM, 0, &dev);
	if (ret) {
		debug("DRAM init failed: %d\n", ret);
		return ret;
	}
    
    //printf("spl_soc_init\n");
	return 0;
}

void harts_early_init(void)
{
    //printf("harts_early_init\n");
}
