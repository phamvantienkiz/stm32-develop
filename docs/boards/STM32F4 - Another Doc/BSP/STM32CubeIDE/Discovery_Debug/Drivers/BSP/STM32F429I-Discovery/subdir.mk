################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
C:/Users/ADMIN/STM32Cube/Repository/STM32Cube_FW_F4_V1.28.3/Drivers/BSP/STM32F429I-Discovery/stm32f429i_discovery.c \
C:/Users/ADMIN/STM32Cube/Repository/STM32Cube_FW_F4_V1.28.3/Drivers/BSP/STM32F429I-Discovery/stm32f429i_discovery_eeprom.c \
C:/Users/ADMIN/STM32Cube/Repository/STM32Cube_FW_F4_V1.28.3/Drivers/BSP/STM32F429I-Discovery/stm32f429i_discovery_gyroscope.c \
C:/Users/ADMIN/STM32Cube/Repository/STM32Cube_FW_F4_V1.28.3/Drivers/BSP/STM32F429I-Discovery/stm32f429i_discovery_io.c \
C:/Users/ADMIN/STM32Cube/Repository/STM32Cube_FW_F4_V1.28.3/Drivers/BSP/STM32F429I-Discovery/stm32f429i_discovery_lcd.c \
C:/Users/ADMIN/STM32Cube/Repository/STM32Cube_FW_F4_V1.28.3/Drivers/BSP/STM32F429I-Discovery/stm32f429i_discovery_sdram.c \
C:/Users/ADMIN/STM32Cube/Repository/STM32Cube_FW_F4_V1.28.3/Drivers/BSP/STM32F429I-Discovery/stm32f429i_discovery_ts.c 

OBJS += \
./Drivers/BSP/STM32F429I-Discovery/stm32f429i_discovery.o \
./Drivers/BSP/STM32F429I-Discovery/stm32f429i_discovery_eeprom.o \
./Drivers/BSP/STM32F429I-Discovery/stm32f429i_discovery_gyroscope.o \
./Drivers/BSP/STM32F429I-Discovery/stm32f429i_discovery_io.o \
./Drivers/BSP/STM32F429I-Discovery/stm32f429i_discovery_lcd.o \
./Drivers/BSP/STM32F429I-Discovery/stm32f429i_discovery_sdram.o \
./Drivers/BSP/STM32F429I-Discovery/stm32f429i_discovery_ts.o 

C_DEPS += \
./Drivers/BSP/STM32F429I-Discovery/stm32f429i_discovery.d \
./Drivers/BSP/STM32F429I-Discovery/stm32f429i_discovery_eeprom.d \
./Drivers/BSP/STM32F429I-Discovery/stm32f429i_discovery_gyroscope.d \
./Drivers/BSP/STM32F429I-Discovery/stm32f429i_discovery_io.d \
./Drivers/BSP/STM32F429I-Discovery/stm32f429i_discovery_lcd.d \
./Drivers/BSP/STM32F429I-Discovery/stm32f429i_discovery_sdram.d \
./Drivers/BSP/STM32F429I-Discovery/stm32f429i_discovery_ts.d 


# Each subdirectory must supply rules for building sources it contributes
Drivers/BSP/STM32F429I-Discovery/stm32f429i_discovery.o: C:/Users/ADMIN/STM32Cube/Repository/STM32Cube_FW_F4_V1.28.3/Drivers/BSP/STM32F429I-Discovery/stm32f429i_discovery.c Drivers/BSP/STM32F429I-Discovery/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_STM32F429I_DISCO -DSTM32F429xx -c -I../../Inc -I../../../../../../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../../../../../../Drivers/STM32F4xx_HAL_Driver/Inc -I../../../../../../Drivers/BSP/STM32F429I-Discovery -I../../../../../../Utilities/Log -I../../../../../../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Drivers/BSP/STM32F429I-Discovery/stm32f429i_discovery_eeprom.o: C:/Users/ADMIN/STM32Cube/Repository/STM32Cube_FW_F4_V1.28.3/Drivers/BSP/STM32F429I-Discovery/stm32f429i_discovery_eeprom.c Drivers/BSP/STM32F429I-Discovery/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_STM32F429I_DISCO -DSTM32F429xx -c -I../../Inc -I../../../../../../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../../../../../../Drivers/STM32F4xx_HAL_Driver/Inc -I../../../../../../Drivers/BSP/STM32F429I-Discovery -I../../../../../../Utilities/Log -I../../../../../../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Drivers/BSP/STM32F429I-Discovery/stm32f429i_discovery_gyroscope.o: C:/Users/ADMIN/STM32Cube/Repository/STM32Cube_FW_F4_V1.28.3/Drivers/BSP/STM32F429I-Discovery/stm32f429i_discovery_gyroscope.c Drivers/BSP/STM32F429I-Discovery/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_STM32F429I_DISCO -DSTM32F429xx -c -I../../Inc -I../../../../../../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../../../../../../Drivers/STM32F4xx_HAL_Driver/Inc -I../../../../../../Drivers/BSP/STM32F429I-Discovery -I../../../../../../Utilities/Log -I../../../../../../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Drivers/BSP/STM32F429I-Discovery/stm32f429i_discovery_io.o: C:/Users/ADMIN/STM32Cube/Repository/STM32Cube_FW_F4_V1.28.3/Drivers/BSP/STM32F429I-Discovery/stm32f429i_discovery_io.c Drivers/BSP/STM32F429I-Discovery/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_STM32F429I_DISCO -DSTM32F429xx -c -I../../Inc -I../../../../../../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../../../../../../Drivers/STM32F4xx_HAL_Driver/Inc -I../../../../../../Drivers/BSP/STM32F429I-Discovery -I../../../../../../Utilities/Log -I../../../../../../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Drivers/BSP/STM32F429I-Discovery/stm32f429i_discovery_lcd.o: C:/Users/ADMIN/STM32Cube/Repository/STM32Cube_FW_F4_V1.28.3/Drivers/BSP/STM32F429I-Discovery/stm32f429i_discovery_lcd.c Drivers/BSP/STM32F429I-Discovery/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_STM32F429I_DISCO -DSTM32F429xx -c -I../../Inc -I../../../../../../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../../../../../../Drivers/STM32F4xx_HAL_Driver/Inc -I../../../../../../Drivers/BSP/STM32F429I-Discovery -I../../../../../../Utilities/Log -I../../../../../../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Drivers/BSP/STM32F429I-Discovery/stm32f429i_discovery_sdram.o: C:/Users/ADMIN/STM32Cube/Repository/STM32Cube_FW_F4_V1.28.3/Drivers/BSP/STM32F429I-Discovery/stm32f429i_discovery_sdram.c Drivers/BSP/STM32F429I-Discovery/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_STM32F429I_DISCO -DSTM32F429xx -c -I../../Inc -I../../../../../../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../../../../../../Drivers/STM32F4xx_HAL_Driver/Inc -I../../../../../../Drivers/BSP/STM32F429I-Discovery -I../../../../../../Utilities/Log -I../../../../../../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Drivers/BSP/STM32F429I-Discovery/stm32f429i_discovery_ts.o: C:/Users/ADMIN/STM32Cube/Repository/STM32Cube_FW_F4_V1.28.3/Drivers/BSP/STM32F429I-Discovery/stm32f429i_discovery_ts.c Drivers/BSP/STM32F429I-Discovery/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_STM32F429I_DISCO -DSTM32F429xx -c -I../../Inc -I../../../../../../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../../../../../../Drivers/STM32F4xx_HAL_Driver/Inc -I../../../../../../Drivers/BSP/STM32F429I-Discovery -I../../../../../../Utilities/Log -I../../../../../../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Drivers-2f-BSP-2f-STM32F429I-2d-Discovery

clean-Drivers-2f-BSP-2f-STM32F429I-2d-Discovery:
	-$(RM) ./Drivers/BSP/STM32F429I-Discovery/stm32f429i_discovery.cyclo ./Drivers/BSP/STM32F429I-Discovery/stm32f429i_discovery.d ./Drivers/BSP/STM32F429I-Discovery/stm32f429i_discovery.o ./Drivers/BSP/STM32F429I-Discovery/stm32f429i_discovery.su ./Drivers/BSP/STM32F429I-Discovery/stm32f429i_discovery_eeprom.cyclo ./Drivers/BSP/STM32F429I-Discovery/stm32f429i_discovery_eeprom.d ./Drivers/BSP/STM32F429I-Discovery/stm32f429i_discovery_eeprom.o ./Drivers/BSP/STM32F429I-Discovery/stm32f429i_discovery_eeprom.su ./Drivers/BSP/STM32F429I-Discovery/stm32f429i_discovery_gyroscope.cyclo ./Drivers/BSP/STM32F429I-Discovery/stm32f429i_discovery_gyroscope.d ./Drivers/BSP/STM32F429I-Discovery/stm32f429i_discovery_gyroscope.o ./Drivers/BSP/STM32F429I-Discovery/stm32f429i_discovery_gyroscope.su ./Drivers/BSP/STM32F429I-Discovery/stm32f429i_discovery_io.cyclo ./Drivers/BSP/STM32F429I-Discovery/stm32f429i_discovery_io.d ./Drivers/BSP/STM32F429I-Discovery/stm32f429i_discovery_io.o ./Drivers/BSP/STM32F429I-Discovery/stm32f429i_discovery_io.su ./Drivers/BSP/STM32F429I-Discovery/stm32f429i_discovery_lcd.cyclo ./Drivers/BSP/STM32F429I-Discovery/stm32f429i_discovery_lcd.d ./Drivers/BSP/STM32F429I-Discovery/stm32f429i_discovery_lcd.o ./Drivers/BSP/STM32F429I-Discovery/stm32f429i_discovery_lcd.su ./Drivers/BSP/STM32F429I-Discovery/stm32f429i_discovery_sdram.cyclo ./Drivers/BSP/STM32F429I-Discovery/stm32f429i_discovery_sdram.d ./Drivers/BSP/STM32F429I-Discovery/stm32f429i_discovery_sdram.o ./Drivers/BSP/STM32F429I-Discovery/stm32f429i_discovery_sdram.su ./Drivers/BSP/STM32F429I-Discovery/stm32f429i_discovery_ts.cyclo ./Drivers/BSP/STM32F429I-Discovery/stm32f429i_discovery_ts.d ./Drivers/BSP/STM32F429I-Discovery/stm32f429i_discovery_ts.o ./Drivers/BSP/STM32F429I-Discovery/stm32f429i_discovery_ts.su

.PHONY: clean-Drivers-2f-BSP-2f-STM32F429I-2d-Discovery

