// Copyright (c) 2013 Vasili Baranau
// Distributed under the MIT software license
// See the accompanying file License.txt or http://opensource.org/licenses/MIT

#ifndef ImageProcessing_Services_Headers_ImageResampler_h
#define ImageProcessing_Services_Headers_ImageResampler_h

#include "Core/Headers/Macros.h"
#include "ImageProcessing/Model/Headers/Types.h"

namespace Services { class ActiveAreaComputer; }
namespace Model { class Config; }

namespace Services
{
    class ImageResampler
    {
    private:
        mutable Model::Config const * config;

    public:
        ImageResampler();

        void ResampleImages(const Model::Config& config) const;

        virtual ~ImageResampler();

    private:
        void ResampleImagesByZ(const std::vector<std::string>& imagePathsBeforeResamplingWithDuplicates,
                const std::vector<std::string>& imagePathsAfterResamplingWithPostfixes) const;

        void ResampleImagesByXY(const Core::DiscreteSpatialVector& actualDimensions, const std::vector<std::string>& imagePathsBeforeResamplingWithDuplicates,
                const std::vector<std::string>& imagePathsAfterResamplingWithPostfixes) const;

        void ResampleImageByXY(const Core::DiscreteSpatialVector& actualDimensions, Model::IsSolidMaskType** initialImage, Model::IsSolidMaskType** finalImage) const;

        void FillActualDimensions(Core::DiscreteSpatialVector* actualDimensions) const;

        void FillImageMapping(std::vector<std::string>* imagePathsBeforeResamplingWithDuplicates,
                        std::vector<std::string>* imagePathsAfterResamplingWithPostfixes) const;

        void FillImagePathsAfterResamplingWithPostfixes(const std::vector<std::string>& imagePathsBeforeResamplingWithDuplicates,
                std::vector<std::string>* imagePathsAfterResamplingWithPostfixes) const;

        void FillImagePathsBeforeResamplingWithDuplicates(const std::vector<std::string>& actualImagePaths, std::vector<std::string>* imagePathsBeforeResamplingWithDuplicates) const;

        DISALLOW_COPY_AND_ASSIGN(ImageResampler);
    };
}

#endif /* ImageProcessing_Services_Headers_ImageResampler_h */

