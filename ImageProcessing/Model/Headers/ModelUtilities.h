// Copyright (c) 2013 Vasili Baranau
// Distributed under the MIT software license
// See the accompanying file License.txt or http://opensource.org/licenses/MIT

#ifndef ImageProcessing_Model_Headers_ModelUtilities_h
#define ImageProcessing_Model_Headers_ModelUtilities_h

#include "Core/Headers/Types.h"
#include "Core/Headers/StlUtilities.h"
#include "Core/Headers/VectorUtilities.h"

namespace Model
{
    class ModelUtilities
    {
    public:
        static bool IsDiagonalStencilVector(const Core::DiscreteSpatialVector& stencilCenter, const Core::DiscreteSpatialVector& neighbor)
        {
            return IsDiagonalStencilVector(stencilCenter, neighbor[Core::Axis::X], neighbor[Core::Axis::Y], neighbor[Core::Axis::Z]);
        }

        static bool IsDiagonalStencilVector(const Core::DiscreteSpatialVector& stencilCenter, int neighborX, int neighborY, int neighborZ)
        {
            bool result = ((neighborX != stencilCenter[Core::Axis::X] && neighborY != stencilCenter[Core::Axis::Y]) ||
                    (neighborX != stencilCenter[Core::Axis::X] && neighborZ != stencilCenter[Core::Axis::Z]) ||
                    (neighborY != stencilCenter[Core::Axis::Y] && neighborZ != stencilCenter[Core::Axis::Z]));

            return result;
        }

        static bool IsOuterPixel(const Core::DiscreteSpatialVector& pixel, const Config& config)
        {
            return IsOuterPixel(pixel[Core::Axis::X], pixel[Core::Axis::Y], pixel[Core::Axis::Z], config);
        }

        static bool IsOuterPixel(int x, int y, int z, const Config& config)
        {
            // TODO: vectorize
            bool isOutside = x < 0 || x >= config.imageSize[Core::Axis::X] ||
                    y < 0 || y >= config.imageSize[Core::Axis::Y] ||
                    z < 0 || z >= config.imageSize[Core::Axis::Z];
            return isOutside;
        }

        static void FillInclusiveStencilBoundaries(const Core::DiscreteSpatialVector& position, const Config& config, int halfLength,
                Core::DiscreteSpatialVector* minCoordinates, Core::DiscreteSpatialVector* maxCoordinates)
        {
            Core::DiscreteSpatialVector& minCoordinatesRef = *minCoordinates;
            Core::DiscreteSpatialVector& maxCoordinatesRef = *maxCoordinates;

            for (int i = 0; i < DIMENSIONS; ++i)
            {
                minCoordinatesRef[i] = std::max(0, position[i] - halfLength);
                maxCoordinatesRef[i] = std::min(config.imageSize[i] - 1, position[i] + halfLength);
            }
        };

        static size_t GetPixelsCount(const Config& config)
        {
            return Core::VectorUtilities::GetProductGeneric<Core::DiscreteSpatialVector, size_t>(config.imageSize);
        }

        static Core::FLOAT_TYPE GetMaximumBallsFraction(const Config& config, const IntermediateStatistics& intermediateStatistics)
        {
            size_t pixelsCount = GetPixelsCount(config);
            Core::FLOAT_TYPE maximumBallsFraction = static_cast<Core::FLOAT_TYPE>(intermediateStatistics.maximumBallsCount) / pixelsCount;
            return maximumBallsFraction;
        }
    };
}

#endif /* ImageProcessing_Model_Headers_ModelUtilities_h */

