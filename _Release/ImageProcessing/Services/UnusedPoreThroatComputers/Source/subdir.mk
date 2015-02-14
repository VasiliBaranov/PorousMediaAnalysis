################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../ImageProcessing/Services/UnusedPoreThroatComputers/Source/IncorrectWatershedComputer.cpp \
../ImageProcessing/Services/UnusedPoreThroatComputers/Source/OnePixelBoundaryThroatComputer.cpp \
../ImageProcessing/Services/UnusedPoreThroatComputers/Source/SteepestDescentPoreAndThroatComputer.cpp \
../ImageProcessing/Services/UnusedPoreThroatComputers/Source/WatershedComputerByEuclideanDistance.cpp 

OBJS += \
./ImageProcessing/Services/UnusedPoreThroatComputers/Source/IncorrectWatershedComputer.o \
./ImageProcessing/Services/UnusedPoreThroatComputers/Source/OnePixelBoundaryThroatComputer.o \
./ImageProcessing/Services/UnusedPoreThroatComputers/Source/SteepestDescentPoreAndThroatComputer.o \
./ImageProcessing/Services/UnusedPoreThroatComputers/Source/WatershedComputerByEuclideanDistance.o 

CPP_DEPS += \
./ImageProcessing/Services/UnusedPoreThroatComputers/Source/IncorrectWatershedComputer.d \
./ImageProcessing/Services/UnusedPoreThroatComputers/Source/OnePixelBoundaryThroatComputer.d \
./ImageProcessing/Services/UnusedPoreThroatComputers/Source/SteepestDescentPoreAndThroatComputer.d \
./ImageProcessing/Services/UnusedPoreThroatComputers/Source/WatershedComputerByEuclideanDistance.d 


# Each subdirectory must supply rules for building sources it contributes
ImageProcessing/Services/UnusedPoreThroatComputers/Source/%.o: ../ImageProcessing/Services/UnusedPoreThroatComputers/Source/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cygwin C++ Compiler'
	g++ -DBOOST_DISABLE_ASSERTS -DNDEBUG -I../Externals/FreeImage/Wrapper/FreeImagePlus -I../Externals/FreeImage/Dist -I../Externals/Boost -I.. -O3 -funroll-loops -Wall -c -fmessage-length=0 -fopenmp -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


