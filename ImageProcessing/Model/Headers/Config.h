// Copyright (c) 2013 Vasili Baranau
// Distributed under the MIT software license
// See the accompanying file License.txt or http://opensource.org/licenses/MIT

#ifndef ImageProcessing_Model_Headers_Config_h
#define ImageProcessing_Model_Headers_Config_h

#include "Types.h"
#include "Core/Headers/Macros.h"

namespace Model
{
    template<class TConfig>
    class IConfig
    {
    public:
        virtual void MergeWith(const TConfig& config) = 0;

        virtual void Reset() = 0;

        virtual ~IConfig(){ };
    };

    class Config : public virtual IConfig<Config>
    {
    public:
        Core::SpatialVector normalizedResolutionOfInitialImages;

        std::string baseFolder;

        Core::DiscreteSpatialVector imageSize;

        int availableMemoryInMegabytes;

        // Ratio of the maximum expected distance from pore center to the last pore ball (including balls in shared throats)
        // to the radius of the starting pore ball (seed).
        Core::FLOAT_TYPE maxPoreRadiusToSeedRadiusRatio;

        StencilType::Type stencilType;

        int numberOfThreads;

    public:
        Config();

        OVERRIDE void Reset();

        OVERRIDE void MergeWith(const Config& config);

    private:
        DISALLOW_COPY_AND_ASSIGN(Config);
    };
}

#endif /* ImageProcessing_Model_Headers_Config_h */

