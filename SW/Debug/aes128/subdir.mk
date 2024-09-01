################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (11.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../aes128/aes128.c \
../aes128/aes128_test.c 

OBJS += \
./aes128/aes128.o \
./aes128/aes128_test.o 

C_DEPS += \
./aes128/aes128.d \
./aes128/aes128_test.d 


# Each subdirectory must supply rules for building sources it contributes
aes128/%.o aes128/%.su aes128/%.cyclo: ../aes128/%.c aes128/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F411xE -c -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I../USB_DEVICE/App -I../USB_DEVICE/Target -I../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-aes128

clean-aes128:
	-$(RM) ./aes128/aes128.cyclo ./aes128/aes128.d ./aes128/aes128.o ./aes128/aes128.su ./aes128/aes128_test.cyclo ./aes128/aes128_test.d ./aes128/aes128_test.o ./aes128/aes128_test.su

.PHONY: clean-aes128

