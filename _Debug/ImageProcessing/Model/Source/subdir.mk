################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../ImageProcessing/Model/Source/Config.cpp \
../ImageProcessing/Model/Source/DataArraysGroup.cpp 

OBJS += \
./ImageProcessing/Model/Source/Config.o \
./ImageProcessing/Model/Source/DataArraysGroup.o 

CPP_DEPS += \
./ImageProcessing/Model/Source/Config.d \
./ImageProcessing/Model/Source/DataArraysGroup.d 


# Each subdirectory must supply rules for building sources it contributes
ImageProcessing/Model/Source/%.o: ../ImageProcessing/Model/Source/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cygwin C++ Compiler'
	g++ -DDEBUG -I../Externals/FreeImage/Wrapper/FreeImagePlus -I../Externals/FreeImage/Dist -I../Externals/Boost -I.. -O0 -g3 -Wall -c -fmessage-length=0 -fopenmp -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


