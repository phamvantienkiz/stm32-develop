# Post 1

## Author

PFlor.2

## Threads

STM32H563 LCD BSP file to use with STM LCD Utility

I acquired a Utility to display simple text on a display with the STM32H743 using the attached LCD driver Utility files from ST MCD Application Team.  I would like to also use this on my bootloader for the STM32H563, does anyone have the files to be able to use this with the STM32H5xx and ST7789V LCD?  The STM32H743 has stm32h7xx_lcd.h/.c files that I had found for that micro that supports LTDC, the H5 does not have LTDC and I'm using an SPI interface to the ST7789V LCD display.

The stm32_lcd.c/.h files use the following BSB functions that I need to support for the STM32H5xx, I have the driver files for the ST7789V as I'm already using this display with TouchGFX for the application, but for the bootloader just need to display lines of text using UTIL_LCD_DisplayStringAtNextLine() with this utility.

BSP_LCD_DrawBitmap
BSP_LCD_DrawHLine
BSP_LCD_DrawVLine
BSP_LCD_FillRect
BSP_LCD_ReadPixel
BSP_LCD_WritePixel
BSP_LCD_GetXSize
BSP_LCD_GetYSize
BSP_LCD_SetActiveLayer

### Attackments

[stm32_lcd.h](docs\boards\STM32F4 - Another Doc\references\stm32_lcd.h)
[stm32_lcd.c](docs\boards\STM32F4 - Another Doc\references\stm32_lcd.c)

## Replies

Hello @PFlor.2

For the STM32H5xx, especially the STM32H573I-Discovery board, there is a BSP driver that includes the LCD files you need. The files are called:

stm32h573i_discovery_lcd.c
stm32h573i_discovery_lcd.h
You can find them in this GitHub repository:
https://github.com/STMicroelectronics/stm32h573i-discovery-bsp/tree/6998f72f8c327b9e7e674a7a1b1eb5e2bccacc9a
