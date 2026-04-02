// SPDX-License-Identifier: GPL-2.0+

#include <init.h>
#include <spl.h>
#include <misc.h>
#include <log.h>
#include <hang.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <image.h>

extern char _end[];

// This is called in board_init_f of arch/riscv/lib/spl.c
int spl_board_init_f(void)
{
    // TODO you can init your DDR memory here
	#define CCM_SUEN	0x7CE
	#define CCM_SEN		0x2020202

	/* enable ccm ops for smode */
	csr_write(CCM_SUEN, CCM_SEN);

    #define MATTRI0_MASK    0x7F4
    #define MATTRI0_BASE    0x7F3
    #define HOST_MAILBOX_BASE_ADDR  (0x8800000UL)
    #define HOST_MAILBOX_SIZE       (0x4000ULL)

    uint64_t base_val;

    base_val = (HOST_MAILBOX_BASE_ADDR & (~(HOST_MAILBOX_SIZE-1))) | BIT(2) | BIT(0);

    
    csr_write(MATTRI0_MASK, ~(HOST_MAILBOX_SIZE-1));
    csr_write(MATTRI0_BASE, base_val);
    

	return 0;
}


void spl_board_init(void)
{

// TODO do your basic board initialization for uboot spl
    log_info("Do initialization for spl board, sizeof(struct global_data)=%lu!\n", sizeof(struct global_data));
    

}


u32 spl_boot_device(void)
{
	return BOOT_DEVICE_RAM;
}

void *board_fdt_blob_setup(int *err)
{
    void *fdt_blob = NULL;
    fdt_blob = (ulong *)&_end;
    *err = 0;
    return fdt_blob;
}

// board_init_f weak version is defined arch/riscv/lib/spl.c
// you should never select CONFIG_SPL_FRAMEWORK_BOARD_INIT_F in kconfig

#ifdef CONFIG_SPL_DISPLAY_PRINT
void spl_display_print(void)
{
	DECLARE_GLOBAL_DATA_PTR;
	const char *model;

	/* same code than show_board_info() but not compiled for SPL
	 * see CONFIG_DISPLAY_BOARDINFO & common/board_info.c
	 */
	model = fdt_getprop(gd->fdt_blob, 0, "model", NULL);
	if (model)
		log_info("Model: %s\n", model);
}
#endif

#ifdef CONFIG_SPL_LOAD_FIT
int board_fit_config_name_match(const char *name)
{
	/* boot using first FIT config */
	return 0;
}

static int image_uncipher(const void *fit, int image_noffset,
                             void **data, size_t *size)
{
       int cipher_noffset, ret;
       void *dst;
       size_t size_dst;

       cipher_noffset = fdt_subnode_offset(fit, image_noffset,
                                           FIT_CIPHER_NODENAME);
       if (cipher_noffset < 0)
               return 0;

       log_info("decrypt %s...", fit_get_name(fit, image_noffset, NULL));
       ret = fit_image_decrypt_data(fit, image_noffset, cipher_noffset,
                                    *data, *size, &dst, &size_dst);
       if (ret) {
               log_info("Failed,err:0x%x\n", ret);
               goto out;
	   }

       *data = dst;
       *size = size_dst;

       log_info("OK\n");
 out:
       return ret;
}

void board_fit_image_post_process(const void *fit, int node, void **p_image,
                                 size_t *p_size)
{
       image_uncipher(fit, node, p_image, p_size);
}

#endif
