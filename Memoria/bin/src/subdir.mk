################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
/home/utnso/workspace/tp-2017-1c-Los-Mantecosos/Bibliotecas/Helper.c \
../src/Interface.c \
../src/Memoria.c \
/home/utnso/workspace/tp-2017-1c-Los-Mantecosos/Bibliotecas/SocketsL.c \
../src/ThreadsMemoria.c 

OBJS += \
./src/Helper.o \
./src/Interface.o \
./src/Memoria.o \
./src/SocketsL.o \
./src/ThreadsMemoria.o 

C_DEPS += \
./src/Helper.d \
./src/Interface.d \
./src/Memoria.d \
./src/SocketsL.d \
./src/ThreadsMemoria.d 


# Each subdirectory must supply rules for building sources it contributes
src/Helper.o: /home/utnso/workspace/tp-2017-1c-Los-Mantecosos/Bibliotecas/Helper.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"../../Bibliotecas" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"../../Bibliotecas" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/SocketsL.o: /home/utnso/workspace/tp-2017-1c-Los-Mantecosos/Bibliotecas/SocketsL.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"../../Bibliotecas" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


