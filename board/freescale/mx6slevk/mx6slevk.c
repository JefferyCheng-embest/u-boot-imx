/*
 * Copyright (C) 2013 Freescale Semiconductor, Inc.
 *
 * Author: Fabio Estevam <fabio.estevam@freescale.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#include <asm/arch/clock.h>
#include <asm/arch/iomux.h>
#include <asm/arch/imx-regs.h>
#include <asm/arch/mx6-pins.h>
#include <asm/arch/sys_proto.h>
#include <asm/gpio.h>
#include <asm/imx-common/iomux-v3.h>
#include <asm/io.h>
#include <asm/sizes.h>
#include <common.h>
#include <fsl_esdhc.h>
#include <mmc.h>
#include <netdev.h>
#if CONFIG_I2C_MXC
#include <i2c.h>
#include <asm/imx-common/mxc_i2c.h>
#endif

DECLARE_GLOBAL_DATA_PTR;

#define UART_PAD_CTRL  (PAD_CTL_PKE | PAD_CTL_PUE |		\
	PAD_CTL_PUS_100K_UP | PAD_CTL_SPEED_MED |		\
	PAD_CTL_DSE_40ohm   | PAD_CTL_SRE_FAST  | PAD_CTL_HYS)

#define USDHC_PAD_CTRL (PAD_CTL_PKE | PAD_CTL_PUE |		\
	PAD_CTL_PUS_22K_UP  | PAD_CTL_SPEED_LOW |		\
	PAD_CTL_DSE_80ohm   | PAD_CTL_SRE_FAST  | PAD_CTL_HYS)

#define ENET_PAD_CTRL  (PAD_CTL_PKE | PAD_CTL_PUE |             \
	PAD_CTL_PUS_100K_UP | PAD_CTL_SPEED_MED   |             \
	PAD_CTL_DSE_40ohm   | PAD_CTL_HYS)

#define I2C_PAD_CTRL    (PAD_CTL_PKE | PAD_CTL_PUE |            \
	PAD_CTL_PUS_100K_UP | PAD_CTL_SPEED_MED |               \
	PAD_CTL_DSE_40ohm | PAD_CTL_HYS |			\
	PAD_CTL_ODE | PAD_CTL_SRE_FAST)

#if CONFIG_I2C_MXC
#define PC MUX_PAD_CTRL(I2C_PAD_CTRL)
/* I2C1 for PMIC */
struct i2c_pads_info i2c_pad_info0 = {
	.scl = {
		.i2c_mode = MX6_PAD_I2C1_SDA__I2C1_SDA | PC,
		.gpio_mode = MX6_PAD_I2C1_SDA__GPIO_3_13 | PC,
		.gp = IMX_GPIO_NR(3, 13),
	},
	.sda = {
		.i2c_mode = MX6_PAD_I2C1_SCL__I2C1_SCL | PC,
		.gpio_mode = MX6_PAD_I2C1_SCL__GPIO_3_12 | PC,
		.gp = IMX_GPIO_NR(3, 12),
	},
};
#endif

int dram_init(void)
{
	gd->ram_size = get_ram_size((void *)PHYS_SDRAM, PHYS_SDRAM_SIZE);

	return 0;
}

static iomux_v3_cfg_t const uart1_pads[] = {
	MX6_PAD_UART1_TXD__UART1_TXD | MUX_PAD_CTRL(UART_PAD_CTRL),
	MX6_PAD_UART1_RXD__UART1_RXD | MUX_PAD_CTRL(UART_PAD_CTRL),
};

static iomux_v3_cfg_t const usdhc2_pads[] = {
	MX6_PAD_SD2_CLK__USDHC2_CLK | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD2_CMD__USDHC2_CMD | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD2_DAT0__USDHC2_DAT0 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD2_DAT1__USDHC2_DAT1 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD2_DAT2__USDHC2_DAT2 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD2_DAT3__USDHC2_DAT3 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
};

static iomux_v3_cfg_t const fec_pads[] = {
	MX6_PAD_FEC_MDC__FEC_MDC | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_FEC_MDIO__FEC_MDIO | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_FEC_CRS_DV__FEC_RX_DV | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_FEC_RXD0__FEC_RX_DATA0 | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_FEC_RXD1__FEC_RX_DATA1 | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_FEC_TX_EN__FEC_TX_EN | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_FEC_TXD0__FEC_TX_DATA0 | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_FEC_TXD1__FEC_TX_DATA1 | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_FEC_REF_CLK__FEC_REF_OUT | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_FEC_RX_ER__GPIO_4_19 | MUX_PAD_CTRL(NO_PAD_CTRL),
	MX6_PAD_FEC_TX_CLK__GPIO_4_21 | MUX_PAD_CTRL(NO_PAD_CTRL),
};

static void setup_iomux_uart(void)
{
	imx_iomux_v3_setup_multiple_pads(uart1_pads, ARRAY_SIZE(uart1_pads));
}

static void setup_iomux_fec(void)
{
	imx_iomux_v3_setup_multiple_pads(fec_pads, ARRAY_SIZE(fec_pads));

	/* Reset LAN8720 PHY */
	gpio_direction_output(IMX_GPIO_NR(4, 21) , 0);
	udelay(1000);
	gpio_set_value(IMX_GPIO_NR(4, 21), 1);
}

static struct fsl_esdhc_cfg usdhc_cfg[1] = {
	{USDHC2_BASE_ADDR},
};

int board_mmc_getcd(struct mmc *mmc)
{
	return 1;	/* Assume boot SD always present */
}

int board_mmc_init(bd_t *bis)
{
	imx_iomux_v3_setup_multiple_pads(usdhc2_pads, ARRAY_SIZE(usdhc2_pads));

	usdhc_cfg[0].sdhc_clk = mxc_get_clock(MXC_ESDHC2_CLK);
	return fsl_esdhc_initialize(bis, &usdhc_cfg[0]);
}

#ifdef CONFIG_FEC_MXC
int board_eth_init(bd_t *bis)
{
	int ret;

	setup_iomux_fec();

	ret = cpu_eth_init(bis);
	if (ret)
		printf("FEC MXC: %s:failed\n", __func__);

	return 0;
}

static int setup_fec(void)
{
	struct iomuxc_base_regs *const iomuxc_regs
		= (struct iomuxc_base_regs *) IOMUXC_BASE_ADDR;
	int ret;

	/* clear gpr1[14], gpr1[18:17] to select anatop clock */
	clrsetbits_le32(&iomuxc_regs->gpr[1], IOMUX_GPR1_FEC_MASK, 0);

	ret = enable_fec_clock();
	if (ret)
		return ret;

	return 0;
}
#endif

#ifdef CONFIG_I2C_MXC
static int setup_pmic_voltages(void)
{
	unsigned char value, rev_id = 0;

	i2c_init(CONFIG_SYS_I2C_SPEED, CONFIG_SYS_I2C_SLAVE);
	if (!i2c_probe(0x8)) {
		if (i2c_read(0x8, 0, 1, &value, 1)) {
			printf("Read device ID error!\n");
			return -1;
		}
		if (i2c_read(0x8, 3, 1, &rev_id, 1)) {
			printf("Read Rev ID error!\n");
			return -1;
		}
		printf("Found PFUZE100! deviceid=%x,revid=%x\n", value, rev_id);

		/* set SW1AB staby volatage 0.975V */
		if (i2c_read(0x8, 0x21, 1, &value, 1)) {
			printf("Read SW1ABSTBY error!\n");
			return -1;
		}
		value &= ~0x3f;
		value |= 0x1b;
		if (i2c_write(0x8, 0x21, 1, &value, 1)) {
			printf("Set SW1ABSTBY error!\n");
			return -1;
		}

		/* set SW1AB/VDDARM step ramp up time from 16us to 4us/25mV */
		if (i2c_read(0x8, 0x24, 1, &value, 1)) {
			printf("Read SW1ABCONFIG error!\n");
			return -1;
		}
		value &= ~0xc0;
		value |= 0x40;
		if (i2c_write(0x8, 0x24, 1, &value, 1)) {
			printf("Set SW1ABCONFIG error!\n");
			return -1;
		}

		/* set SW1C staby volatage 0.975V */
		if (i2c_read(0x8, 0x2f, 1, &value, 1)) {
			printf("Read SW1CSTBY error!\n");
			return -1;
		}
		value &= ~0x3f;
		value |= 0x1b;
		if (i2c_write(0x8, 0x2f, 1, &value, 1)) {
			printf("Set SW1CSTBY error!\n");
			return -1;
		}

		/* set SW1C/VDDSOC step ramp up time to from 16us to 4us/25mV */
		if (i2c_read(0x8, 0x32, 1, &value, 1)) {
			printf("Read SW1CCONFIG error!\n");
			return -1;
		}
		value &= ~0xc0;
		value |= 0x40;
		if (i2c_write(0x8, 0x32, 1, &value, 1)) {
			printf("Set SW1CCONFIG error!\n");
			return -1;
		}
	}

	return 0;
}

#ifdef CONFIG_LDO_BYPASS_CHECK
void ldo_mode_set(int ldo_bypass)
{
	unsigned char value;
	/* swith to ldo_bypass mode */
	if (ldo_bypass) {
		/* decrease VDDARM to 1.15V */
		if (i2c_read(0x8, 0x20, 1, &value, 1)) {
			printf("Read SW1AB error!\n");
			return;
		}
		value &= ~0x3f;
		value |= 0x22;
		if (i2c_write(0x8, 0x20, 1, &value, 1)) {
			printf("Set SW1AB error!\n");
			return;
		}
		/* increase VDDSOC to 1.15V */
		if (i2c_read(0x8, 0x2e, 1, &value, 1)) {
			printf("Read SW1C error!\n");
			return;
		}
		value &= ~0x3f;
		value |= 0x22;
		if (i2c_write(0x8, 0x2e, 1, &value, 1)) {
			printf("Set SW1C error!\n");
			return;
		}

		set_anatop_bypass();
		printf("switch to ldo_bypass mode!\n");
	}

}
#endif
#endif

int board_early_init_f(void)
{
	setup_iomux_uart();
	return 0;
}

int board_init(void)
{
	/* address of boot parameters */
	gd->bd->bi_boot_params = PHYS_SDRAM + 0x100;

#ifdef	CONFIG_FEC_MXC
	setup_fec();
#endif

	return 0;
}

int board_late_init(void)
{
	int ret = 0;

#ifdef CONFIG_I2C_MXC
	setup_i2c(0, CONFIG_SYS_I2C_SPEED,
			CONFIG_SYS_I2C_SLAVE, &i2c_pad_info0);
	ret = setup_pmic_voltages();
	if (ret)
		return -1;
#endif

	return 0;
}

u32 get_board_rev(void)
{
	return get_cpu_rev();
}

int checkboard(void)
{
	puts("Board: MX6SLEVK\n");

	return 0;
}