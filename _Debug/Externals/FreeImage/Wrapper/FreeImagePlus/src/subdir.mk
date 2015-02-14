################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../Externals/FreeImage/Wrapper/FreeImagePlus/src/FreeImagePlus.cpp \
../Externals/FreeImage/Wrapper/FreeImagePlus/src/fipImage.cpp \
../Externals/FreeImage/Wrapper/FreeImagePlus/src/fipMemoryIO.cpp \
../Externals/FreeImage/Wrapper/FreeImagePlus/src/fipMetadataFind.cpp \
../Externals/FreeImage/Wrapper/FreeImagePlus/src/fipMultiPage.cpp \
../Externals/FreeImage/Wrapper/FreeImagePlus/src/fipTag.cpp \
../Externals/FreeImage/Wrapper/FreeImagePlus/src/fipWinImage.cpp 

OBJS += \
./Externals/FreeImage/Wrapper/FreeImagePlus/src/FreeImagePlus.o \
./Externals/FreeImage/Wrapper/FreeImagePlus/src/fipImage.o \
./Externals/FreeImage/Wrapper/FreeImagePlus/src/fipMemoryIO.o \
./Externals/FreeImage/Wrapper/FreeImagePlus/src/fipMetadataFind.o \
./Externals/FreeImage/Wrapper/FreeImagePlus/src/fipMultiPage.o \
./Externals/FreeImage/Wrapper/FreeImagePlus/src/fipTag.o \
./Externals/FreeImage/Wrapper/FreeImagePlus/src/fipWinImage.o 

CPP_DEPS += \
./Externals/FreeImage/Wrapper/FreeImagePlus/src/FreeImagePlus.d \
./Externals/FreeImage/Wrapper/FreeImagePlus/src/fipImage.d \
./Externals/FreeImage/Wrapper/FreeImagePlus/src/fipMemoryIO.d \
./Externals/FreeImage/Wrapper/FreeImagePlus/src/fipMetadataFind.d \
./Externals/FreeImage/Wrapper/FreeImagePlus/src/fipMultiPage.d \
./Externals/FreeImage/Wrapper/FreeImagePlus/src/fipTag.d \
./Externals/FreeImage/Wrapper/FreeImagePlus/src/fipWinImage.d 


# Each subdirectory must supply rules for building sources it contributes
Externals/FreeImage/Wrapper/FreeImagePlus/src/%.o: ../Externals/FreeImage/Wrapper/FreeImagePlus/src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cygwin C++ Compiler'
	g++ -DDEBUG -I../Externals/FreeImage/Wrapper/FreeImagePlus -I../Externals/FreeImage/Dist -I../Externals/Boost -I.. -O0 -g3 -Wall -c -fmessage-length=0 -fopenmp -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


