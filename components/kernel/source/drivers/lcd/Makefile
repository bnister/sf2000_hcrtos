lcd_main-y := lcd_main.o
lcd_main-$(CONFIG_DRV_LCD_DEV) += lcddev.o
lcd_main-$(CONFIG_DRV_LCD_NT35510) += 	display/nt35510.o
lcd_main-$(CONFIG_DRV_LCD_GC9505) += 	display/gc9505.o
lcd_main-$(CONFIG_DRV_LCD_JDI026) += 	display/jdi026.o
lcd_main-$(CONFIG_DRV_LCD_SC5014A) += 	display/sc5014a.o
lcd_main-$(CONFIG_DRV_LCD_JD9168)  += 	display/jd9168.o
lcd_main-$(CONFIG_DRV_LCD_ST7789V2) += display/st7789v2.o
lcd_main-$(CONFIG_DRV_LCD_N116BGE)  += 	display/n116bge.o

lcd_main-$(CONFIG_DRV_BACKLIGHT) += backlight.o
obj-y += lcd_main.o
