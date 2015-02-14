################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../Core/Source/EndiannessProvider.cpp \
../Core/Source/Exceptions.cpp \
../Core/Source/Math.cpp \
../Core/Source/MpiManager.cpp \
../Core/Source/OpenMpManager.cpp \
../Core/Source/Path.cpp \
../Core/Source/Utilities.cpp \
../Core/Source/VectorUtilities.cpp 

OBJS += \
./Core/Source/EndiannessProvider.o \
./Core/Source/Exceptions.o \
./Core/Source/Math.o \
./Core/Source/MpiManager.o \
./Core/Source/OpenMpManager.o \
./Core/Source/Path.o \
./Core/Source/Utilities.o \
./Core/Source/VectorUtilities.o 

CPP_DEPS += \
./Core/Source/EndiannessProvider.d \
./Core/Source/Exceptions.d \
./Core/Source/Math.d \
./Core/Source/MpiManager.d \
./Core/Source/OpenMpManager.d \
./Core/Source/Path.d \
./Core/Source/Utilities.d \
./Core/Source/VectorUtilities.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Source/%.o: ../Core/Source/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cygwin C++ Compiler'
	g++ -DBOOST_DISABLE_ASSERTS -DNDEBUG -I../Externals/FreeImage/Wrapper/FreeImagePlus -I../Externals/FreeImage/Dist -I../Externals/Boost -I.. -O3 -funroll-loops -Wall -c -fmessage-length=0 -fopenmp -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


