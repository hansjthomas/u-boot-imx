/*
 * Copyright (C) 2016 Technologic Systems
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <asm/arch/clock.h>
#include <asm/arch/crm_regs.h>
#include <asm/arch/iomux.h>
#include <asm/arch/imx-regs.h>
#include <asm/arch/mx6-pins.h>
#include <asm/arch/sys_proto.h>
#include <asm/gpio.h>
#include <asm/imx-common/iomux-v3.h>
#include <asm/imx-common/boot_mode.h>
#include <asm/imx-common/mxc_i2c.h>
#include <asm/imx-common/video.h>
#include <asm/io.h>
#include <common.h>
#include <fpga.h>
#include <fsl_esdhc.h>
#include <i2c.h>
#include <lattice.h>
#include <linux/sizes.h>
#include <linux/fb.h>
#include <miiphy.h>
#include <mmc.h>
#include <mxsfb.h>
#include <netdev.h>
#include <usb.h>
#include <usb/ehci-fsl.h>
#include "tsfpga.h"
#include "fram.h"

DECLARE_GLOBAL_DATA_PTR;

#define PHY1_DUPLEX 		IMX_GPIO_NR(2, 0)
#define PHY2_DUPLEX 		IMX_GPIO_NR(2, 8)
#define PHY1_PHYADDR2 		IMX_GPIO_NR(2, 1)
#define PHY2_PHYADDR2		IMX_GPIO_NR(2, 9)
#define PHY1_CONFIG_2 		IMX_GPIO_NR(2, 2)
#define PHY2_CONFIG_2		IMX_GPIO_NR(2, 10)
#define PHY1_ISOLATE		IMX_GPIO_NR(2, 7)
#define PHY2_ISOLATE		IMX_GPIO_NR(2, 15)
#define EN_USB_5V		IMX_GPIO_NR(3, 0)
#define EN_XBEE_USB		IMX_GPIO_NR(1, 21)
#define DETECT_94120		IMX_GPIO_NR(4, 1)

#define UART_PAD_CTRL  (PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP | \
	PAD_CTL_SPEED_MED | PAD_CTL_DSE_40ohm | PAD_CTL_SRE_FAST | PAD_CTL_HYS)

#define USDHC_PAD_CTRL (PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_22K_UP | \
	PAD_CTL_SPEED_LOW | PAD_CTL_DSE_80ohm | PAD_CTL_SRE_FAST | PAD_CTL_HYS)

#define USDHC_PAD_CTRL_WP (PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_DOWN | \
	PAD_CTL_SPEED_LOW | PAD_CTL_DSE_80ohm | PAD_CTL_SRE_FAST | PAD_CTL_HYS)

#define ENET_PAD_CTRL (PAD_CTL_PUS_100K_UP | PAD_CTL_PUE | \
	PAD_CTL_SPEED_HIGH | PAD_CTL_DSE_48ohm | PAD_CTL_SRE_FAST)

#define MISC_PAD_PU_CTRL (PAD_CTL_PUS_100K_UP | PAD_CTL_PKE | PAD_CTL_PUE | \
	PAD_CTL_DSE_48ohm | PAD_CTL_SRE_FAST)

#define MISC_PAD_PD_CTRL (PAD_CTL_PUS_100K_DOWN | PAD_CTL_PKE | PAD_CTL_PUE | \
	PAD_CTL_DSE_48ohm | PAD_CTL_SRE_FAST)

#define MISC_PAD_CTRL (PAD_CTL_DSE_48ohm | PAD_CTL_SRE_FAST)

#define ENET_CLK_PAD_CTRL (PAD_CTL_SPEED_MED | PAD_CTL_DSE_120ohm | \
	PAD_CTL_SRE_FAST)

#define ENET_RX_PAD_CTRL (PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_SPEED_HIGH | \
	PAD_CTL_SRE_FAST)

#define I2C_PAD_CTRL (PAD_CTL_SPEED_MED | PAD_CTL_DSE_40ohm | PAD_CTL_HYS | \
	PAD_CTL_ODE)

#define LCD_PAD_CTRL (PAD_CTL_HYS | PAD_CTL_PUS_100K_UP | PAD_CTL_PUE | \
	PAD_CTL_PKE | PAD_CTL_SPEED_MED | PAD_CTL_DSE_34ohm)

#define OTG_ID_PAD_CTRL (PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_47K_UP  | \
	PAD_CTL_SPEED_LOW | PAD_CTL_DSE_80ohm   | PAD_CTL_SRE_FAST  | \
	PAD_CTL_HYS)

#define PC MUX_PAD_CTRL(I2C_PAD_CTRL)

/* I2C1 for Silabs */
struct i2c_pads_info i2c_pad_info1 = {
	.scl = {
		.i2c_mode = MX6_PAD_GPIO1_IO02__I2C1_SCL | PC,
		.gpio_mode = MX6_PAD_GPIO1_IO02__GPIO1_IO02 | PC,
		.gp = IMX_GPIO_NR(1, 2),
	},
	.sda = {
		.i2c_mode = MX6_PAD_GPIO1_IO03__I2C1_SDA | PC,
		.gpio_mode = MX6_PAD_GPIO1_IO03__GPIO1_IO03 | PC,
		.gp = IMX_GPIO_NR(1, 3),
	},
};

/* I2C3 for CN99 */
struct i2c_pads_info i2c_pad_info3 = {
	.scl = {
		.i2c_mode = MX6_PAD_LCD_DATA01__I2C3_SCL | PC,
		.gpio_mode = MX6_PAD_LCD_DATA01__GPIO3_IO06 | PC,
		.gp = IMX_GPIO_NR(3, 6),
	},
	.sda = {
		.i2c_mode = MX6_PAD_LCD_DATA00__I2C3_SDA | PC,
		.gpio_mode = MX6_PAD_LCD_DATA00__GPIO3_IO05 | PC,
		.gp = IMX_GPIO_NR(3, 5),
	},
};

int dram_init(void)
{
	gd->ram_size = (phys_size_t)CONFIG_DDR_MB * 1024 * 1024;

	return 0;
}

static iomux_v3_cfg_t const uart1_pads[] = {
	MX6_PAD_UART1_TX_DATA__UART1_DCE_TX | MUX_PAD_CTRL(UART_PAD_CTRL),
	MX6_PAD_UART1_RX_DATA__UART1_DCE_RX | MUX_PAD_CTRL(UART_PAD_CTRL),
};

static iomux_v3_cfg_t const eim_pins[] = {
	MX6_PAD_CSI_DATA00__EIM_AD00 | MUX_PAD_CTRL(MISC_PAD_CTRL),
	MX6_PAD_CSI_DATA01__EIM_AD01 | MUX_PAD_CTRL(MISC_PAD_CTRL),
	MX6_PAD_CSI_DATA02__EIM_AD02 | MUX_PAD_CTRL(MISC_PAD_CTRL),
	MX6_PAD_CSI_DATA03__EIM_AD03 | MUX_PAD_CTRL(MISC_PAD_CTRL),
	MX6_PAD_CSI_DATA04__EIM_AD04 | MUX_PAD_CTRL(MISC_PAD_CTRL),
	MX6_PAD_CSI_DATA05__EIM_AD05 | MUX_PAD_CTRL(MISC_PAD_CTRL),
	MX6_PAD_CSI_DATA06__EIM_AD06 | MUX_PAD_CTRL(MISC_PAD_CTRL),
	MX6_PAD_CSI_DATA07__EIM_AD07 | MUX_PAD_CTRL(MISC_PAD_CTRL),
	MX6_PAD_NAND_DATA00__EIM_AD08 | MUX_PAD_CTRL(MISC_PAD_CTRL),
	MX6_PAD_NAND_DATA01__EIM_AD09 | MUX_PAD_CTRL(MISC_PAD_CTRL),
	MX6_PAD_NAND_DATA02__EIM_AD10 | MUX_PAD_CTRL(MISC_PAD_CTRL),
	MX6_PAD_NAND_DATA03__EIM_AD11 | MUX_PAD_CTRL(MISC_PAD_CTRL),
	MX6_PAD_NAND_DATA04__EIM_AD12 | MUX_PAD_CTRL(MISC_PAD_CTRL),
	MX6_PAD_NAND_DATA05__EIM_AD13 | MUX_PAD_CTRL(MISC_PAD_CTRL),
	MX6_PAD_NAND_DATA06__EIM_AD14 | MUX_PAD_CTRL(MISC_PAD_CTRL),
	MX6_PAD_NAND_DATA07__EIM_AD15 | MUX_PAD_CTRL(MISC_PAD_CTRL),
	MX6_PAD_CSI_VSYNC__EIM_RW | MUX_PAD_CTRL(MISC_PAD_CTRL),
	MX6_PAD_CSI_HSYNC__EIM_LBA_B | MUX_PAD_CTRL(MISC_PAD_CTRL),
	MX6_PAD_CSI_PIXCLK__EIM_OE | MUX_PAD_CTRL(MISC_PAD_CTRL),
	MX6_PAD_NAND_DQS__EIM_WAIT | MUX_PAD_CTRL(MISC_PAD_CTRL),
	MX6_PAD_NAND_WP_B__EIM_BCLK | MUX_PAD_CTRL(MISC_PAD_PU_CTRL),

	/* FPGA_IRQ */
	MX6_PAD_SNVS_TAMPER1__GPIO5_IO01 | MUX_PAD_CTRL(MISC_PAD_PU_CTRL),
	/* FPGA_IRQ2 */
	MX6_PAD_NAND_ALE__GPIO4_IO10 | MUX_PAD_CTRL(MISC_PAD_PU_CTRL),

};

static iomux_v3_cfg_t const misc_pads[] = {
	/* GYRO_INT */
	MX6_PAD_SNVS_TAMPER0__GPIO5_IO00 | MUX_PAD_CTRL(MISC_PAD_CTRL),
	/* FPGA_FLASH_SELECT */
	MX6_PAD_NAND_RE_B__GPIO4_IO00 | MUX_PAD_CTRL(MISC_PAD_CTRL),
	/* EN_USB_5V */
	MX6_PAD_LCD_CLK__GPIO3_IO00 | MUX_PAD_CTRL(MISC_PAD_CTRL),
	/* EN_XBEE_USB */
	MX6_PAD_UART2_RX_DATA__GPIO1_IO21 | MUX_PAD_CTRL(MISC_PAD_CTRL),
	/* DETECT_94-120 */
	MX6_PAD_NAND_WE_B__GPIO4_IO01 | MUX_PAD_CTRL(MISC_PAD_CTRL),
};

/* eMMC */
static iomux_v3_cfg_t const usdhc1_emmc_pads[] = {
	MX6_PAD_SD1_CLK__USDHC1_CLK | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD1_CMD__USDHC1_CMD | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD1_DATA0__USDHC1_DATA0 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD1_DATA1__USDHC1_DATA1 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD1_DATA2__USDHC1_DATA2 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD1_DATA3__USDHC1_DATA3 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
};

static void setup_iomux_uart(void)
{
	imx_iomux_v3_setup_multiple_pads(uart1_pads, ARRAY_SIZE(uart1_pads));
}

static struct fsl_esdhc_cfg usdhc_cfg[1] = {
	{USDHC1_BASE_ADDR, 0, 4},
};

int board_mmc_getcd(struct mmc *mmc)
{
	return 1;
}

int board_mmc_init(bd_t *bis)
{
	/*
	 * According to the board_mmc_init() the following map is done:
	 * (U-boot device node)    (Physical Port)
	 * mmc0                    USDHC1 (EMMC)
	 */

	imx_iomux_v3_setup_multiple_pads(
		usdhc1_emmc_pads, ARRAY_SIZE(usdhc1_emmc_pads));
	usdhc_cfg[0].sdhc_clk = mxc_get_clock(MXC_ESDHC_CLK);

	if (fsl_esdhc_initialize(bis, &usdhc_cfg[0]))
		printf("Warning: failed to initialize eMMC dev\n");

	return 0;
}

#ifdef CONFIG_FEC_MXC

static iomux_v3_cfg_t const fec_enet_pads[] = {
	MX6_PAD_ENET1_RX_DATA0__ENET1_RDATA00 | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_ENET1_RX_DATA1__ENET1_RDATA01 | MUX_PAD_CTRL(ENET_PAD_CTRL),

	MX6_PAD_ENET1_RX_EN__ENET1_RX_EN | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_ENET1_RX_ER__ENET1_RX_ER | MUX_PAD_CTRL(ENET_PAD_CTRL),

	MX6_PAD_ENET1_TX_DATA0__ENET1_TDATA00 | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_ENET1_TX_DATA1__ENET1_TDATA01 | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_ENET1_TX_EN__ENET1_TX_EN | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_ENET1_TX_CLK__ENET1_REF_CLK1 | MUX_PAD_CTRL(ENET_CLK_PAD_CTRL),

	MX6_PAD_ENET2_RX_DATA0__ENET2_RDATA00 | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_ENET2_RX_DATA1__ENET2_RDATA01 | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_ENET2_RX_EN__ENET2_RX_EN | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_ENET2_RX_ER__ENET2_RX_ER | MUX_PAD_CTRL(ENET_PAD_CTRL),

	MX6_PAD_ENET2_TX_DATA0__ENET2_TDATA00 | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_ENET2_TX_DATA1__ENET2_TDATA01 | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_ENET2_TX_EN__ENET2_TX_EN | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_ENET2_TX_CLK__ENET2_REF_CLK2 | MUX_PAD_CTRL(ENET_PAD_CTRL),

#if (CONFIG_FEC_ENET_DEV == 0)
	MX6_PAD_GPIO1_IO06__ENET1_MDIO | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_GPIO1_IO07__ENET1_MDC | MUX_PAD_CTRL(ENET_PAD_CTRL),
#else
	MX6_PAD_GPIO1_IO06__ENET2_MDIO | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_GPIO1_IO07__ENET2_MDC | MUX_PAD_CTRL(ENET_PAD_CTRL),
#endif

};

static iomux_v3_cfg_t const fec_enet_pads1[] = {
	MX6_PAD_GPIO1_IO06__ENET1_MDIO | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_GPIO1_IO07__ENET1_MDC | MUX_PAD_CTRL(ENET_PAD_CTRL),
	/* DUPLEX */
	MX6_PAD_ENET1_RX_DATA0__GPIO2_IO00 | MUX_PAD_CTRL(ENET_PAD_CTRL),
	/* DUPLEX */
	MX6_PAD_ENET2_RX_DATA0__GPIO2_IO08 | MUX_PAD_CTRL(ENET_PAD_CTRL),
	/* PHYADDR2 */
	MX6_PAD_ENET1_RX_DATA1__GPIO2_IO01 | MUX_PAD_CTRL(ENET_PAD_CTRL),
	/* PHYADDR2 */
	MX6_PAD_ENET2_RX_DATA1__GPIO2_IO09 | MUX_PAD_CTRL(ENET_PAD_CTRL),
	/* CONFIG_2 */
	MX6_PAD_ENET1_RX_EN__GPIO2_IO02 | MUX_PAD_CTRL(ENET_PAD_CTRL),
	/* CONFIG_2 */
	MX6_PAD_ENET2_RX_EN__GPIO2_IO10 | MUX_PAD_CTRL(ENET_PAD_CTRL),
	/* Isolate */
	MX6_PAD_ENET1_RX_ER__GPIO2_IO07 | MUX_PAD_CTRL(ENET_PAD_CTRL),
	/* Isolate */
	MX6_PAD_ENET2_RX_ER__GPIO2_IO15 | MUX_PAD_CTRL(ENET_PAD_CTRL),
};

int board_eth_init(bd_t *bis)
{
	/* Set pins to strapping GPIO modes */
	imx_iomux_v3_setup_multiple_pads(fec_enet_pads1,
					 ARRAY_SIZE(fec_enet_pads1));

	fpga_dio1_dat_clr(BANK1_PHY_RESETN);
	fpga_dio1_oe_set(BANK1_PHY_RESETN);
	gpio_direction_output(PHY1_DUPLEX, 0);
	gpio_direction_output(PHY2_DUPLEX, 0);
	gpio_direction_output(PHY1_PHYADDR2, 0);
	gpio_direction_output(PHY2_PHYADDR2, 0);
	gpio_direction_output(PHY1_CONFIG_2, 0);
	gpio_direction_output(PHY2_CONFIG_2, 0);
	gpio_direction_output(PHY1_ISOLATE, 0);
	gpio_direction_output(PHY2_ISOLATE, 0);

	udelay(1); // 5ns
	fpga_dio1_dat_set(BANK1_PHY_RESETN);
	udelay(1); // 5ns

	/* Set pins to enet modes */
	imx_iomux_v3_setup_multiple_pads(fec_enet_pads,
					 ARRAY_SIZE(fec_enet_pads));

	return fecmxc_initialize_multi(bis, CONFIG_FEC_ENET_DEV,
						 CONFIG_FEC_MXC_PHYADDR, IMX_FEC_BASE);
}

static int setup_fec(int fec_id)
{
	struct iomuxc *const iomuxc_regs = (struct iomuxc *)IOMUXC_BASE_ADDR;
	int ret;

	if (check_module_fused(MX6_MODULE_ENET1))
		return -1;
	if (check_module_fused(MX6_MODULE_ENET2))
		return -1;

	/*
	 * Use 50M anatop loopback REF_CLK1 for ENET1,
	 * clear gpr1[13], set gpr1[17].
	 */
	clrsetbits_le32(&iomuxc_regs->gpr[1], IOMUX_GPR1_FEC1_MASK,
			IOMUX_GPR1_FEC1_CLOCK_MUX1_SEL_MASK);

	clrsetbits_le32(&iomuxc_regs->gpr[1], IOMUX_GPR1_FEC2_MASK,
			IOMUX_GPR1_FEC2_CLOCK_MUX1_SEL_MASK);

	ret = enable_fec_anatop_clock(0, ENET_50MHZ);
	ret |= enable_fec_anatop_clock(1, ENET_50MHZ);
	if (ret)
		return ret;

	enable_enet_clk(1);

	return 0;
}

int board_phy_config(struct phy_device *phydev)
{
	/* Reset phy 1, phy 2 is reset by default */
	phydev->bus->write(phydev->bus, 0x1, MDIO_DEVAD_NONE, 0x0, 0x8000);
	/* Disable BCAST, select RMII */
	phydev->bus->write(phydev->bus, 0x1, MDIO_DEVAD_NONE, 0x16, 0x202);
	phydev->bus->write(phydev->bus, 0x2, MDIO_DEVAD_NONE, 0x16, 0x202);
	/* Enable 50MHZ Clock */
	phydev->bus->write(phydev->bus, 0x1, MDIO_DEVAD_NONE, 0x1f, 0x8180);
	phydev->bus->write(phydev->bus, 0x2, MDIO_DEVAD_NONE, 0x1f, 0x8180);

	return 0;
}
#endif

int board_early_init_f(void)
{
	struct weim *weim_regs = (struct weim *)WEIM_BASE_ADDR;
	struct mxc_ccm_reg *const imx_ccm =
		(struct mxc_ccm_reg *)CCM_BASE_ADDR;

	/* select PLL2 PFD2 for EIM, and set divider to divide-by-4, to yield
	 * a 99MHz clock */
	clrbits_le32(0x020C401C, 0x60000000);
	setbits_le32(0x020C401C, 0x41000000);
	clrsetbits_le32(&imx_ccm->cscmr1,
			MXC_CCM_CSCMR1_ACLK_EMI_SLOW_PODF_MASK |
			MXC_CCM_CSCMR1_ACLK_EMI_SLOW_MASK,
			7 << MXC_CCM_CSCMR1_ACLK_EMI_SLOW_PODF_OFFSET |
			2 << MXC_CCM_CSCMR1_ACLK_EMI_SLOW_OFFSET);

	/* Set up EIM bus for FPGA */
	imx_iomux_v3_setup_multiple_pads(
		eim_pins, ARRAY_SIZE(eim_pins));

	writel(0x0161030F, &weim_regs->cs0gcr1);
	writel(0x00000000, &weim_regs->cs0gcr2);
	writel(0x03000000, &weim_regs->cs0rcr1);
	writel(0x00000000, &weim_regs->cs0rcr2);
	writel(0x01000000, &weim_regs->cs0wcr1);
	writel(0x00000E09, &weim_regs->wcr);

	set_chipselect_size(CS0_128);

	setup_i2c(0, CONFIG_SYS_I2C_SPEED, 0x7f, &i2c_pad_info1);
	setup_i2c(2, CONFIG_SYS_I2C_SPEED, 0x7f, &i2c_pad_info3);

	setup_iomux_uart();

	return 0;
}

int misc_init_r(void)
{
	imx_iomux_v3_setup_multiple_pads(misc_pads, ARRAY_SIZE(misc_pads));
	setenv("model", "7250v3");

	return 0;
}

int board_init(void)
{
	/* Address of boot parameters */
	gd->bd->bi_boot_params = PHYS_SDRAM + 0x100;
#ifdef CONFIG_FEC_MXC
	setup_fec(CONFIG_FEC_ENET_DEV);
#endif

	return 0;
}

int board_late_init(void)
{
	set_wdog_reset((struct wdog_regs *)WDOG1_BASE_ADDR);

	gpio_direction_output(EN_USB_5V , 1);
	gpio_direction_input(EN_XBEE_USB);

	return 0;
}

u32 get_board_rev(void)
{
	return get_cpu_rev();
}

#ifdef CONFIG_USB_EHCI_MX6
#define USB_OTHERREGS_OFFSET	0x800
#define UCTRL_PWR_POL		(1 << 9)

int board_usb_phy_mode(int port)
{
	printf("port %d\n", port);
	if(port == 0) {
		gpio_direction_input(DETECT_94120);
		if (gpio_get_value(DETECT_94120))
			return USB_INIT_DEVICE;
		return USB_INIT_HOST;
	}
	return USB_INIT_HOST;
}

#endif
