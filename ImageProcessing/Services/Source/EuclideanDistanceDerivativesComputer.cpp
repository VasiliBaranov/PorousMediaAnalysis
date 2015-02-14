// Copyright (c) 2013 Vasili Baranau
// Distributed under the MIT software license
// See the accompanying file License.txt or http://opensource.org/licenses/MIT

#include "../Headers/EuclideanDistanceDerivativesComputer.h"

#include <cmath>

using namespace std;
using namespace Core;
using namespace Model;
using namespace Services;

namespace Services
{
    EuclideanDistanceDerivativesComputer::EuclideanDistanceDerivativesComputer()
    {
    }

    void EuclideanDistanceDerivativesComputer::ComputeEuclideanDistanceDerivatives(EuclideanDistanceSquareType maxNeededEuclideanDistanceSquare,
                    vector<EuclideanDistanceType>* euclideanDistancesByEuclideanDistanceSquares,
                    vector<EuclideanDistanceSquareType>* ballLowerRadiiSquaresByEuclideanDistanceSquares,
                    vector<EuclideanDistanceType>* ballLowerRadiiByEuclideanDistanceSquares) const
    {
        vector<EuclideanDistanceType>& euclideanDistancesByEuclideanDistanceSquaresRef = *euclideanDistancesByEuclideanDistanceSquares;
        vector<EuclideanDistanceSquareType>& ballLowerRadiiSquaresByEuclideanDistanceSquaresRef = *ballLowerRadiiSquaresByEuclideanDistanceSquares;
        vector<EuclideanDistanceType>& ballLowerRadiiByEuclideanDistanceSquaresRef = *ballLowerRadiiByEuclideanDistanceSquares;

        euclideanDistancesByEuclideanDistanceSquaresRef.resize(maxNeededEuclideanDistanceSquare + 1);
        ballLowerRadiiSquaresByEuclideanDistanceSquaresRef.resize(maxNeededEuclideanDistanceSquare + 1);
        ballLowerRadiiByEuclideanDistanceSquaresRef.resize(maxNeededEuclideanDistanceSquare + 1);

        for (EuclideanDistanceSquareType i = 0; i <= maxNeededEuclideanDistanceSquare; ++i)
        {
            euclideanDistancesByEuclideanDistanceSquaresRef[i] = sqrt(static_cast<EuclideanDistanceType>(i));
        }

        vector<bool> isSumOfSquaresMask(maxNeededEuclideanDistanceSquare + 1, false);
        FillSumOfSquaresMask(&isSumOfSquaresMask);
        FillBallLowerRadiiSquaresByEuclideanDistanceSquares(isSumOfSquaresMask, ballLowerRadiiSquaresByEuclideanDistanceSquares);

        for (EuclideanDistanceSquareType i = 0; i <= maxNeededEuclideanDistanceSquare; ++i)
        {
            EuclideanDistanceSquareType ballLowerRadiusSquare = ballLowerRadiiSquaresByEuclideanDistanceSquaresRef[i];

            // ballLowerRadiusSquare can only be lower as euclideanDistanceSquare, so may use euclideanDistancesByEuclideanDistanceSquaresRef
            ballLowerRadiiByEuclideanDistanceSquaresRef[i] = euclideanDistancesByEuclideanDistanceSquaresRef[ballLowerRadiusSquare];
        }
    }

    void EuclideanDistanceDerivativesComputer::FillBallLowerRadiiSquaresByEuclideanDistanceSquares(const vector<bool>& isSumOfSquaresMask,
            vector<EuclideanDistanceSquareType>* ballLowerRadiiSquaresByEuclideanDistanceSquares) const
    {
        vector<EuclideanDistanceSquareType>& ballLowerRadiiSquaresByEuclideanDistanceSquaresRef = *ballLowerRadiiSquaresByEuclideanDistanceSquares;
        ballLowerRadiiSquaresByEuclideanDistanceSquaresRef[0] = 0;

        EuclideanDistanceSquareType previousEuclideanDistanceSquare = 0;
        for (EuclideanDistanceSquareType i = 1; i < isSumOfSquaresMask.size(); ++i)
        {
            ballLowerRadiiSquaresByEuclideanDistanceSquaresRef[i] = previousEuclideanDistanceSquare;

            if (isSumOfSquaresMask[i])
            {
                previousEuclideanDistanceSquare = i;
            }
        }
    }

    void EuclideanDistanceDerivativesComputer::FillSumOfSquaresMask(vector<bool>* isSumOfSquaresMask) const
    {
        vector<bool>& isSumOfSquaresMaskRef = *isSumOfSquaresMask;
        EuclideanDistanceSquareType maxNeededEuclideanDistanceSquare = isSumOfSquaresMaskRef.size() - 1;
        int maxRadiusInPixels = ceil(sqrt(static_cast<EuclideanDistanceType>(maxNeededEuclideanDistanceSquare))) + 1;
        for (int x = 0; x <= maxRadiusInPixels; ++x)
        {
            for (int y = 0; y <= maxRadiusInPixels; ++y)
            {
                for (int z = 0; z <= maxRadiusInPixels; ++z)
                {
                    EuclideanDistanceSquareType distanceSquare = x * x + y * y + z * z;

                    if (distanceSquare < isSumOfSquaresMaskRef.size())
                    {
                        isSumOfSquaresMaskRef[distanceSquare] = true;
                    }
                }
            }
        }
    }

    EuclideanDistanceDerivativesComputer::~EuclideanDistanceDerivativesComputer()
    {
    }
}

