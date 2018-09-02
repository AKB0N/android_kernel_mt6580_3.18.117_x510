# Linux driver folder

ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/GT9XX_hotknot_x510
ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek
ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/GT9XX_hotknot_x510/include/$(CONFIG_GT9XX_FIRMWARE)
ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/GT9XX_hotknot_x510/include/firmware1
ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/GT9XX_hotknot_x510/include/$(CONFIG_GT9XX_CONFIG)
ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/GT9XX_hotknot_x510/include/config1

ccflags-y += -Wno-implicit-function-declaration -g
ccflags-y += -Wno-date-time -g

obj-y	+=  goodix_tool.o
obj-y	+=  gt9xx_driver.o
obj-y	+=  gt9xx_update.o

