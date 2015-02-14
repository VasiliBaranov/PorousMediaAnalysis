// Copyright (c) 2013 Vasili Baranau
// Distributed under the MIT software license
// See the accompanying file License.txt or http://opensource.org/licenses/MIT

#ifndef ImageProcessing_Services_Headers_SinglePixelProcessor_h
#define ImageProcessing_Services_Headers_SinglePixelProcessor_h

#include <cmath>
#include "Core/Headers/Macros.h"
#include "ImageProcessing/Model/Headers/Types.h"
#include "ImageProcessing/Model/Headers/DataArray.h"
#include "ImageProcessing/Services/Headers/ActiveAreaComputer.h"
#include "ImageProcessing/Services/Headers/Serializer.h"

namespace Services
{
    template<class TInput, class TOutput>
    class SinglePixelProcessor
    {
    private:
        // Services
        const ActiveAreaComputer& activeAreaComputer;
        const Serializer& serializer;

        std::string inputFolderName;
        std::string outputFolderName;
        TOutput (*processPixel)(TInput inputPixel); // pointer to a function that accepts TInput, returns TOutput. The pointer has a name processPixel

    public:
        SinglePixelProcessor(const ActiveAreaComputer& currentActiveAreaComputer, const Serializer& currentSerializer,
                std::string currentInputFolderName, std::string currentOutputFolderName,
                TOutput (*currentProcessPixel)(TInput inputPixel)) // // pointer to a function that accepts TInput, returns TOutput. The pointer has a name currentProcessPixel
        : activeAreaComputer(currentActiveAreaComputer), serializer(currentSerializer), inputFolderName(currentInputFolderName), outputFolderName(currentOutputFolderName), processPixel(currentProcessPixel)
        {
        }

        void ProcessPixels(const Model::Config& config) const
        {
            Model::Margin margin(Model::Margin::Pixels, 0);

            std::vector<Model::ActiveArea> activeAreas;
            int bytesPerPixel = sizeof(TInput) + sizeof(TOutput);
            std::vector<Core::Axis::Type> priorityAxes(2);
            priorityAxes[0] = Core::Axis::X;
            priorityAxes[1] = Core::Axis::Y;
            activeAreaComputer.ComputeActiveAreas(config, priorityAxes, margin, bytesPerPixel, &activeAreas);

            std::string sourcePath = Core::Path::Append(config.baseFolder, inputFolderName);
            Model::DataArray<TInput> inputData(serializer, config, sourcePath, activeAreas);

            std::string targetPath = Core::Path::Append(config.baseFolder, outputFolderName);
            Model::DataArray<TOutput> outputData(serializer, config, targetPath, activeAreas);

            for (size_t activeAreaIndex = 0; activeAreaIndex < activeAreas.size(); ++activeAreaIndex)
            {
                printf("Active area %d / %d...\n", activeAreaIndex + 1, activeAreas.size());
                Model::ActiveArea& activeArea = activeAreas[activeAreaIndex];

                inputData.ChangeActiveArea(activeAreaIndex);
                outputData.ChangeActiveArea(activeAreaIndex);

                for (int z = activeArea.boxWithMargins.leftCorner[Core::Axis::Z]; z < activeArea.boxWithMargins.exclusiveRightCorner[Core::Axis::Z]; ++z)
                {
                    printf("Processing pixels in z layer: %d / %d\n", z + 1, config.imageSize[Core::Axis::Z]);
                    for (int x = activeArea.boxWithMargins.leftCorner[Core::Axis::X]; x < activeArea.boxWithMargins.exclusiveRightCorner[Core::Axis::X]; ++x)
                    {
                        for (int y = activeArea.boxWithMargins.leftCorner[Core::Axis::Y]; y < activeArea.boxWithMargins.exclusiveRightCorner[Core::Axis::Y]; ++y)
                        {
                            TInput inputValue = inputData.GetPixel(x, y, z);
                            TOutput outputValue = processPixel(inputValue);
                            outputData.SetPixel(x, y, z, outputValue);
                        }
                    }
                }
            }

            outputData.WriteCurrentActiveArea();
        }

        static TOutput GetSqrt(TInput inputValue)
        {
            TOutput outputValue = (inputValue == 0) ? 0 : sqrt(inputValue);
            return outputValue;
        }

        virtual ~SinglePixelProcessor()
        {

        }

        DISALLOW_COPY_AND_ASSIGN(SinglePixelProcessor);
    };
}

#endif /* ImageProcessing_Services_Headers_SinglePixelProcessor_h */

