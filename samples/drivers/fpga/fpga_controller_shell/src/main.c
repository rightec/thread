/*
 * Copyright (c) 2021 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <shell/shell.h>
#include <device.h>
#include <sys/printk.h>
#include <drivers/fpga.h>
#include "redled.h"
#include "greenled.h"
#include <eoss3_dev.h>

void main(void)
{
	IO_MUX->PAD_21_CTRL = (PAD_E_4MA | PAD_P_PULLDOWN | PAD_OEN_NORMAL |
			       PAD_SMT_DISABLE | PAD_REN_DISABLE | PAD_SR_SLOW |
			       PAD_CTRL_SEL_AO_REG); /* Enable red led */
	IO_MUX->PAD_22_CTRL = (PAD_E_4MA | PAD_P_PULLDOWN | PAD_OEN_NORMAL |
			       PAD_SMT_DISABLE | PAD_REN_DISABLE | PAD_SR_SLOW |
			       PAD_CTRL_SEL_AO_REG); /* Enable green led */

	printk("Address of the bitstream (red): %p\n", &axFPGABitStream_red);
	printk("Address of the bitstream (green): %p\n", &axFPGABitStream_green);
	printk("Size of the bitstream (red): %d\n", sizeof(axFPGABitStream_red));
	printk("Size of the bitstream (green): %d\n", sizeof(axFPGABitStream_green));
}
