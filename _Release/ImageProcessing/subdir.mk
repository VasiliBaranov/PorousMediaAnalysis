################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../ImageProcessing/Main.cpp 

OBJS += \
./ImageProcessing/Main.o 

CPP_DEPS += \
./ImageProcessing/Main.d 


# Each subdirectory must supply rules for building sources it contributes
ImageProcessing/%.o: ../ImageProcessing/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cygwin C++ Compiler'
	g++ -DBOOST_DISABLE_ASSERTS -DNDEBUG -I../Externals/FreeImage/Wrapper/FreeImagePlus -I../Externals/FreeImage/Dist -I../Externals/Boost -I.. -O3 -funroll-loops -Wall -c -fmessage-length=0 -fopenmp -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


