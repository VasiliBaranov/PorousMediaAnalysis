################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../ImageProcessing/Execution/Source/CommandLineDispatcher.cpp \
../ImageProcessing/Execution/Source/ImageProcessingManager.cpp 

OBJS += \
./ImageProcessing/Execution/Source/CommandLineDispatcher.o \
./ImageProcessing/Execution/Source/ImageProcessingManager.o 

CPP_DEPS += \
./ImageProcessing/Execution/Source/CommandLineDispatcher.d \
./ImageProcessing/Execution/Source/ImageProcessingManager.d 


# Each subdirectory must supply rules for building sources it contributes
ImageProcessing/Execution/Source/%.o: ../ImageProcessing/Execution/Source/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cygwin C++ Compiler'
	g++ -DBOOST_DISABLE_ASSERTS -DNDEBUG -I../Externals/FreeImage/Wrapper/FreeImagePlus -I../Externals/FreeImage/Dist -I../Externals/Boost -I.. -O3 -funroll-loops -Wall -c -fmessage-length=0 -fopenmp -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


