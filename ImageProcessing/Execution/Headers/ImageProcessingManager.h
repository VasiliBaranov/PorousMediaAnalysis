// Copyright (c) 2013 Vasili Baranau
// Distributed under the MIT software license
// See the accompanying file License.txt or http://opensource.org/licenses/MIT

#ifndef Execution_ImageProcessingManager_h
#define Execution_ImageProcessingManager_h

#include "Core/Headers/Macros.h"
#include "ImageProcessing/Model/Headers/Types.h"
#include "ImageProcessing/Model/Headers/Config.h"

namespace Services { class Serializer; }
namespace Services { class ImageResampler; }
namespace Services { class EuclideanDistanceComputer; }
namespace Services { class ContainingBallsAssigner; }
namespace Services { class PoreAndThroatComputer; }
namespace Services { class EmptyPoresRemover; }
namespace Services { class WatershedComputer; }
namespace Services { class ShortestPathComputer; }

namespace Execution
{
    class ImageProcessingManager
    {
    private:
        Services::Serializer* serializer;
        Services::ImageResampler* imageResampler;
        Services::EuclideanDistanceComputer* euclideanDistanceComputer;
        Services::ContainingBallsAssigner* containingBallsAssigner;
        Services::PoreAndThroatComputer* poreAndThroatComputer;
        Services::EmptyPoresRemover* emptyPoresRemover;
        Services::WatershedComputer* watershedComputer;
        Services::ShortestPathComputer* shortestPathComputer;

        Model::Config config;

    public:
        ImageProcessingManager(Services::Serializer* serializer,
                Services::ImageResampler* imageResampler,
                Services::EuclideanDistanceComputer* euclideanDistanceComputer,
                Services::ContainingBallsAssigner* containingBallsAssigner,
                Services::PoreAndThroatComputer* poreAndThroatComputer,
                Services::EmptyPoresRemover* emptyPoresRemover,
                Services::WatershedComputer* watershedComputer,
                Services::ShortestPathComputer* shortestPathComputer);

        void SetUserConfig(const Model::Config& userConfig);

        void DoAll();

        void CalculateEuclideanDistanceTransform();

        void RemoveInnerBalls();

        void AssignPoresAndThroats();

        void CalculateShortestPathDistances();

        virtual ~ImageProcessingManager();

    private:
        DISALLOW_COPY_AND_ASSIGN(ImageProcessingManager);
    };
}

#endif /* Execution_ImageProcessingManager_h */

