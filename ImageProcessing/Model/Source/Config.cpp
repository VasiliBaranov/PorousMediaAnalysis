// Copyright (c) 2013 Vasili Baranau
// Distributed under the MIT software license
// See the accompanying file License.txt or http://opensource.org/licenses/MIT

#include "../Headers/Config.h"
#include <string>
#include "Core/Headers/VectorUtilities.h"

using namespace Core;

namespace Model
{
    Config::Config()
    {
        Reset();
    }

    void Config::Reset()
    {
        VectorUtilities::InitializeWith(&normalizedResolutionOfInitialImages, -1);
        baseFolder.clear();
        VectorUtilities::InitializeWith(&imageSize, -1);
        availableMemoryInMegabytes = -1;
        maxPoreRadiusToSeedRadiusRatio = -1;
        stencilType = StencilType::Unknown;
        numberOfThreads = -1;
    }

    void Config::MergeWith(const Config& config)
    {
        if (normalizedResolutionOfInitialImages[0] < 0)
        {
            normalizedResolutionOfInitialImages = config.normalizedResolutionOfInitialImages;
        }

        if (baseFolder.empty())
        {
            baseFolder = config.baseFolder;
        }

        if (imageSize[0] < 0)
        {
            imageSize = config.imageSize;
        }

        if (availableMemoryInMegabytes < 0)
        {
            availableMemoryInMegabytes = config.availableMemoryInMegabytes;
        }

        if (maxPoreRadiusToSeedRadiusRatio < 0)
        {
            maxPoreRadiusToSeedRadiusRatio = config.maxPoreRadiusToSeedRadiusRatio;
        }

        if (stencilType == StencilType::Unknown)
        {
            stencilType = config.stencilType;
        }

        if (numberOfThreads < 0)
        {
            numberOfThreads = config.numberOfThreads;
        }
    }
}

