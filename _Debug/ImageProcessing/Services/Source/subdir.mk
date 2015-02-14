################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../ImageProcessing/Services/Source/ActiveAreaComputer.cpp \
../ImageProcessing/Services/Source/BaseDataArrayProcessor.cpp \
../ImageProcessing/Services/Source/BasePoreAndThroatComputer.cpp \
../ImageProcessing/Services/Source/ContainingBallsAssigner.cpp \
../ImageProcessing/Services/Source/EmptyPoresRemover.cpp \
../ImageProcessing/Services/Source/EuclideanDistanceComputer.cpp \
../ImageProcessing/Services/Source/EuclideanDistanceDerivativesComputer.cpp \
../ImageProcessing/Services/Source/ImageResampler.cpp \
../ImageProcessing/Services/Source/PoreAndThroatComputer.cpp \
../ImageProcessing/Services/Source/Serializer.cpp \
../ImageProcessing/Services/Source/ShortestPathComputer.cpp \
../ImageProcessing/Services/Source/WatershedComputer.cpp 

OBJS += \
./ImageProcessing/Services/Source/ActiveAreaComputer.o \
./ImageProcessing/Services/Source/BaseDataArrayProcessor.o \
./ImageProcessing/Services/Source/BasePoreAndThroatComputer.o \
./ImageProcessing/Services/Source/ContainingBallsAssigner.o \
./ImageProcessing/Services/Source/EmptyPoresRemover.o \
./ImageProcessing/Services/Source/EuclideanDistanceComputer.o \
./ImageProcessing/Services/Source/EuclideanDistanceDerivativesComputer.o \
./ImageProcessing/Services/Source/ImageResampler.o \
./ImageProcessing/Services/Source/PoreAndThroatComputer.o \
./ImageProcessing/Services/Source/Serializer.o \
./ImageProcessing/Services/Source/ShortestPathComputer.o \
./ImageProcessing/Services/Source/WatershedComputer.o 

CPP_DEPS += \
./ImageProcessing/Services/Source/ActiveAreaComputer.d \
./ImageProcessing/Services/Source/BaseDataArrayProcessor.d \
./ImageProcessing/Services/Source/BasePoreAndThroatComputer.d \
./ImageProcessing/Services/Source/ContainingBallsAssigner.d \
./ImageProcessing/Services/Source/EmptyPoresRemover.d \
./ImageProcessing/Services/Source/EuclideanDistanceComputer.d \
./ImageProcessing/Services/Source/EuclideanDistanceDerivativesComputer.d \
./ImageProcessing/Services/Source/ImageResampler.d \
./ImageProcessing/Services/Source/PoreAndThroatComputer.d \
./ImageProcessing/Services/Source/Serializer.d \
./ImageProcessing/Services/Source/ShortestPathComputer.d \
./ImageProcessing/Services/Source/WatershedComputer.d 


# Each subdirectory must supply rules for building sources it contributes
ImageProcessing/Services/Source/%.o: ../ImageProcessing/Services/Source/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cygwin C++ Compiler'
	g++ -DDEBUG -I../Externals/FreeImage/Wrapper/FreeImagePlus -I../Externals/FreeImage/Dist -I../Externals/Boost -I.. -O0 -g3 -Wall -c -fmessage-length=0 -fopenmp -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


