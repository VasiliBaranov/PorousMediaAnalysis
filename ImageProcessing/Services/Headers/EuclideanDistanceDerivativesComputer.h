// Copyright (c) 2013 Vasili Baranau
// Distributed under the MIT software license
// See the accompanying file License.txt or http://opensource.org/licenses/MIT

#ifndef ImageProcessing_Services_Headers_EuclideanDistanceDerivativesComputer_h
#define ImageProcessing_Services_Headers_EuclideanDistanceDerivativesComputer_h

#include "Core/Headers/Macros.h"
#include "ImageProcessing/Model/Headers/Types.h"

namespace Services { class Serializer; }
namespace Model { class Config; }

namespace Services
{
    // Computes lower bound of maximum ball radius, as explained in Dong and Blunt (2009) Pore-network extraction from micro-computerized-tomography images, Section II A.
    class EuclideanDistanceDerivativesComputer
    {
    public:
        EuclideanDistanceDerivativesComputer();

        void ComputeEuclideanDistanceDerivatives(Model::EuclideanDistanceSquareType maxNeededEuclideanDistanceSquare,
                std::vector<Model::EuclideanDistanceType>* euclideanDistancesByEuclideanDistanceSquares,
                std::vector<Model::EuclideanDistanceSquareType>* ballLowerRadiiSquaresByEuclideanDistanceSquares,
                std::vector<Model::EuclideanDistanceType>* ballLowerRadiiByEuclideanDistanceSquares) const;

        virtual ~EuclideanDistanceDerivativesComputer();

    private:

        void FillSumOfSquaresMask(std::vector<bool>* isSumOfSquaresMask) const;

        void FillBallLowerRadiiSquaresByEuclideanDistanceSquares(const std::vector<bool>& isSumOfSquaresMask,
                std::vector<Model::EuclideanDistanceSquareType>* ballLowerRadiiSquaresByEuclideanDistanceSquares) const;

        DISALLOW_COPY_AND_ASSIGN(EuclideanDistanceDerivativesComputer);
    };
}

#endif /* ImageProcessing_Services_Headers_EuclideanDistanceDerivativesComputer_h */

