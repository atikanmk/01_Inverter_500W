################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Src/adc.c \
../Src/cnv.c \
../Src/com.c \
../Src/config.c \
../Src/foc.c \
../Src/hall.c \
../Src/main.c \
../Src/pwm.c \
../Src/syscalls.c \
../Src/sysmem.c \
../Src/task.c 

OBJS += \
./Src/adc.o \
./Src/cnv.o \
./Src/com.o \
./Src/config.o \
./Src/foc.o \
./Src/hall.o \
./Src/main.o \
./Src/pwm.o \
./Src/syscalls.o \
./Src/sysmem.o \
./Src/task.o 

C_DEPS += \
./Src/adc.d \
./Src/cnv.d \
./Src/com.d \
./Src/config.d \
./Src/foc.d \
./Src/hall.d \
./Src/main.d \
./Src/pwm.d \
./Src/syscalls.d \
./Src/sysmem.d \
./Src/task.d 


# Each subdirectory must supply rules for building sources it contributes
Src/%.o Src/%.su Src/%.cyclo: ../Src/%.c Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DSTM32 -DSTM32F1 -DSTM32F103C8Tx -c -I../Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Src

clean-Src:
	-$(RM) ./Src/adc.cyclo ./Src/adc.d ./Src/adc.o ./Src/adc.su ./Src/cnv.cyclo ./Src/cnv.d ./Src/cnv.o ./Src/cnv.su ./Src/com.cyclo ./Src/com.d ./Src/com.o ./Src/com.su ./Src/config.cyclo ./Src/config.d ./Src/config.o ./Src/config.su ./Src/foc.cyclo ./Src/foc.d ./Src/foc.o ./Src/foc.su ./Src/hall.cyclo ./Src/hall.d ./Src/hall.o ./Src/hall.su ./Src/main.cyclo ./Src/main.d ./Src/main.o ./Src/main.su ./Src/pwm.cyclo ./Src/pwm.d ./Src/pwm.o ./Src/pwm.su ./Src/syscalls.cyclo ./Src/syscalls.d ./Src/syscalls.o ./Src/syscalls.su ./Src/sysmem.cyclo ./Src/sysmem.d ./Src/sysmem.o ./Src/sysmem.su ./Src/task.cyclo ./Src/task.d ./Src/task.o ./Src/task.su

.PHONY: clean-Src

