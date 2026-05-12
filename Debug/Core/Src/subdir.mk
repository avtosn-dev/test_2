################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/debug.c \
../Core/Src/ds18b20.c \
../Core/Src/main.c \
../Core/Src/onewire.c \
../Core/Src/scheduler.c \
../Core/Src/sensor_config.c \
../Core/Src/stm32f1xx_hal_msp.c \
../Core/Src/stm32f1xx_it.c \
../Core/Src/syscalls.c \
../Core/Src/sysmem.c \
../Core/Src/system_stm32f1xx.c \
../Core/Src/task_sensor_read.c \
../Core/Src/task_uart_input.c \
../Core/Src/task_uart_print.c 

OBJS += \
./Core/Src/debug.o \
./Core/Src/ds18b20.o \
./Core/Src/main.o \
./Core/Src/onewire.o \
./Core/Src/scheduler.o \
./Core/Src/sensor_config.o \
./Core/Src/stm32f1xx_hal_msp.o \
./Core/Src/stm32f1xx_it.o \
./Core/Src/syscalls.o \
./Core/Src/sysmem.o \
./Core/Src/system_stm32f1xx.o \
./Core/Src/task_sensor_read.o \
./Core/Src/task_uart_input.o \
./Core/Src/task_uart_print.o 

C_DEPS += \
./Core/Src/debug.d \
./Core/Src/ds18b20.d \
./Core/Src/main.d \
./Core/Src/onewire.d \
./Core/Src/scheduler.d \
./Core/Src/sensor_config.d \
./Core/Src/stm32f1xx_hal_msp.d \
./Core/Src/stm32f1xx_it.d \
./Core/Src/syscalls.d \
./Core/Src/sysmem.d \
./Core/Src/system_stm32f1xx.d \
./Core/Src/task_sensor_read.d \
./Core/Src/task_uart_input.d \
./Core/Src/task_uart_print.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/%.o Core/Src/%.su Core/Src/%.cyclo: ../Core/Src/%.c Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103xB -c -I../Core/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Core-2f-Src

clean-Core-2f-Src:
	-$(RM) ./Core/Src/debug.cyclo ./Core/Src/debug.d ./Core/Src/debug.o ./Core/Src/debug.su ./Core/Src/ds18b20.cyclo ./Core/Src/ds18b20.d ./Core/Src/ds18b20.o ./Core/Src/ds18b20.su ./Core/Src/main.cyclo ./Core/Src/main.d ./Core/Src/main.o ./Core/Src/main.su ./Core/Src/onewire.cyclo ./Core/Src/onewire.d ./Core/Src/onewire.o ./Core/Src/onewire.su ./Core/Src/scheduler.cyclo ./Core/Src/scheduler.d ./Core/Src/scheduler.o ./Core/Src/scheduler.su ./Core/Src/sensor_config.cyclo ./Core/Src/sensor_config.d ./Core/Src/sensor_config.o ./Core/Src/sensor_config.su ./Core/Src/stm32f1xx_hal_msp.cyclo ./Core/Src/stm32f1xx_hal_msp.d ./Core/Src/stm32f1xx_hal_msp.o ./Core/Src/stm32f1xx_hal_msp.su ./Core/Src/stm32f1xx_it.cyclo ./Core/Src/stm32f1xx_it.d ./Core/Src/stm32f1xx_it.o ./Core/Src/stm32f1xx_it.su ./Core/Src/syscalls.cyclo ./Core/Src/syscalls.d ./Core/Src/syscalls.o ./Core/Src/syscalls.su ./Core/Src/sysmem.cyclo ./Core/Src/sysmem.d ./Core/Src/sysmem.o ./Core/Src/sysmem.su ./Core/Src/system_stm32f1xx.cyclo ./Core/Src/system_stm32f1xx.d ./Core/Src/system_stm32f1xx.o ./Core/Src/system_stm32f1xx.su ./Core/Src/task_sensor_read.cyclo ./Core/Src/task_sensor_read.d ./Core/Src/task_sensor_read.o ./Core/Src/task_sensor_read.su ./Core/Src/task_uart_input.cyclo ./Core/Src/task_uart_input.d ./Core/Src/task_uart_input.o ./Core/Src/task_uart_input.su ./Core/Src/task_uart_print.cyclo ./Core/Src/task_uart_print.d ./Core/Src/task_uart_print.o ./Core/Src/task_uart_print.su

.PHONY: clean-Core-2f-Src

