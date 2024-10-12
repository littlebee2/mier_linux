#!/bin/sh
make distclean
make imx_alientek_emmc_defconfig
make menuconfig
make all -j16
