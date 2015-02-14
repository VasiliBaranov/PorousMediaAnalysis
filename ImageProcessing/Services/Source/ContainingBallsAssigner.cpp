// Copyright (c) 2013 Vasili Baranau
// Distributed under the MIT software license
// See the accompanying file License.txt or http://opensource.org/licenses/MIT

#include "../Headers/ContainingBallsAssigner.h"

#include <limits>
#include <string>
#include <cmath>
#include "stdio.h"

#include "Core/Headers/OpenMpManager.h"
#include "Core/Headers/Path.h"
#include "ImageProcessing/Services/Headers/ActiveAreaComputer.h"
#include "ImageProcessing/Model/Headers/Config.h"
#include "ImageProcessing/Model/Headers/ModelUtilities.h"

using namespace std;
using namespace Core;
using namespace Model;
using namespace Services;

namespace Services
{
    ContainingBallsAssigner::ContainingBallsAssigner(const ActiveAreaComputer& currentActiveAreaComputer, const Serializer& currentSerializer) :
        BaseDataArrayProcessor(currentActiveAreaComputer, currentSerializer)
    {
        dataArrays.AddActiveDataArrayTypes(DataArrayFlags::EuclideanDistanceSquares |
                DataArrayFlags::ContainingBallRadiiSquares |
                DataArrayFlags::ContainingBallCoordinatesX |
                DataArrayFlags::ContainingBallCoordinatesY |
                DataArrayFlags::ContainingBallCoordinatesZ |
                DataArrayFlags::MaximumBallsMask);
    }

    void ContainingBallsAssigner::SetContainingBalls(const Config& config) const
    {
        this->config = &config;

        IntermediateStatistics intermediateStatistics = ReadIntermediateStatistics();

        InitializeEuclideanDistanceDerivatives(intermediateStatistics.maxEuclideanDistanceSquare);

        vector<ActiveArea> activeAreas;
        FillActiveAreas(intermediateStatistics, &activeAreas);

        dataArrays.Initialize(config, activeAreas);

        SetContainingBalls(activeAreas);

        // Can not set maximum balls simultaneously with containing balls (i.e., set containing balls in the active area, then set maximum balls),
        // as balls from neighboring active areas may rewrite some pixels and make the given ball inner.
        maximumBallsCount = 0;
        SetMaximumBallsMask(activeAreas);

        intermediateStatistics.maximumBallsCount = maximumBallsCount;
        WriteIntermediateStatistics(intermediateStatistics);

        dataArrays.Clear();
    }

    void ContainingBallsAssigner::FillActiveAreas(const IntermediateStatistics& intermediateStatistics, vector<ActiveArea>* activeAreas) const
    {
        Margin margin(Margin::Pixels, sqrt(static_cast<FLOAT_TYPE>(intermediateStatistics.maxEuclideanDistanceSquare)) + 1);
        vector<Axis::Type> priorityAxes;
        activeAreaComputer.ComputeActiveAreas(*config, priorityAxes, margin, dataArrays.GetBytesPerPixel(), activeAreas);
    }

    void ContainingBallsAssigner::SetContainingBalls(const vector<ActiveArea>& activeAreas) const
    {
        printf("Setting containing balls...\n");
        for (size_t activeAreaIndex = 0; activeAreaIndex < activeAreas.size(); ++activeAreaIndex)
        {
            printf("Active area " SIZE_T_FORMAT " / " SIZE_T_FORMAT "...\n", activeAreaIndex + 1, activeAreas.size());
            this->activeArea = &activeAreas[activeAreaIndex];

            dataArrays.ChangeActiveArea(activeAreaIndex);

            SetContainingBallsInActiveArea();
        }

        dataArrays.WriteCurrentActiveArea();
    }

    void ContainingBallsAssigner::SetMaximumBallsMask(const vector<ActiveArea>& activeAreas) const
    {
        printf("Setting maximum balls mask...\n");
        for (size_t activeAreaIndex = 0; activeAreaIndex < activeAreas.size(); ++activeAreaIndex)
        {
            printf("Active area " SIZE_T_FORMAT " / " SIZE_T_FORMAT "...\n", activeAreaIndex + 1, activeAreas.size());
            this->activeArea = &activeAreas[activeAreaIndex];

            dataArrays.ChangeActiveArea(activeAreaIndex);

            SetMaximumBallsMaskInActiveArea();
        }

        dataArrays.WriteCurrentActiveArea();
    }

    void ContainingBallsAssigner::SetContainingBallsInActiveArea() const
    {
        printf("Setting containing balls in the active area...\n");

        ResetPercentagePrinting(Axis::X);

        // Visual Studio at the time of writing (2014) supported only OpenMP 2.0, which does not include "collapse" clause.
        // The easiest solution is to parallelize only along one dimension.
        // Monoliths are often short along the z dimension, so we parallelize along x or y,
        // though it is slightly less convenient to imagine (different z coordinates correspond to different images).
        #pragma omp parallel for schedule(static)
        for (int x = activeArea->activeBox.leftCorner[Axis::X]; x < activeArea->activeBox.exclusiveRightCorner[Axis::X]; ++x)
        {
            PrintPercentageCompleted();

            for (int y = activeArea->activeBox.leftCorner[Axis::Y]; y < activeArea->activeBox.exclusiveRightCorner[Axis::Y]; ++y)
            {
                for (int z = activeArea->activeBox.leftCorner[Axis::Z]; z < activeArea->activeBox.exclusiveRightCorner[Axis::Z]; ++z)
                {
                    EuclideanDistanceSquareType ballRadiusSquare = dataArrays.euclideanDistanceSquares.GetPixel(x, y, z);

                    bool isSolidPixel = ballRadiusSquare == 0;
                    if (isSolidPixel)
                    {
                        continue;
                    }

                    SetContainingBall(x, y, z, ballRadiusSquare);
                }
            }
        }
    }

    // TODO: this code is not very thread-safe. If parts of containingBallRadiiSquares are cached by different threads,
    // race conditions may still occur and some pixels may be assigned to radii which are lower than needed.
    void ContainingBallsAssigner::SetContainingBall(int x, int y, int z, EuclideanDistanceSquareType ballRadiusSquare) const
    {
        DiscreteSpatialVector position = {{x, y, z}};

        DiscreteSpatialVector minCoordinates;
        DiscreteSpatialVector maxCoordinates;
        EuclideanDistanceType ballRadius = euclideanDistancesByEuclideanDistanceSquares[ballRadiusSquare];
        int discreteBallRadius = ballRadius + 1;
        ModelUtilities::FillInclusiveStencilBoundaries(position, *config, discreteBallRadius, &minCoordinates, &maxCoordinates);

        // for the points that are in the cube for this pixel
        for (int innerPixelX = minCoordinates[Axis::X]; innerPixelX <= maxCoordinates[Axis::X]; ++innerPixelX)
        {
            for (int innerPixelY = minCoordinates[Axis::Y]; innerPixelY <= maxCoordinates[Axis::Y]; ++innerPixelY)
            {
                for (int innerPixelZ = minCoordinates[Axis::Z]; innerPixelZ <= maxCoordinates[Axis::Z]; ++innerPixelZ)
                {
                    // Shall set radius and pixel index even for the center pixel, thus do not exclude the pixel with x == innerPixelX and so on

                    EuclideanDistanceSquareType distanceBetweenCentersSquare = (innerPixelX - x) * (innerPixelX - x) +
                            (innerPixelY - y) * (innerPixelY - y) + (innerPixelZ - z) * (innerPixelZ - z);
                    if (distanceBetweenCentersSquare > ballRadiusSquare)
                    {
                        continue;
                    }

                    EuclideanDistanceSquareType innerBallRadiusSquare = dataArrays.euclideanDistanceSquares.GetPixel(innerPixelX, innerPixelY, innerPixelZ);
                    bool isSolidPixel = innerBallRadiusSquare == 0;
                    if (isSolidPixel)
                    {
                        continue;
                    }

                    EuclideanDistanceSquareType containingBallRadiusSquare = dataArrays.containingBallRadiiSquares.GetPixel(innerPixelX, innerPixelY, innerPixelZ);
                    if (containingBallRadiusSquare < ballRadiusSquare)
                    {
                        dataArrays.containingBallRadiiSquares.SetPixel(innerPixelX, innerPixelY, innerPixelZ, ballRadiusSquare);
                        #pragma omp flush

                        dataArrays.containingBallCoordinatesX.SetPixel(innerPixelX, innerPixelY, innerPixelZ, x);
                        dataArrays.containingBallCoordinatesY.SetPixel(innerPixelX, innerPixelY, innerPixelZ, y);
                        dataArrays.containingBallCoordinatesZ.SetPixel(innerPixelX, innerPixelY, innerPixelZ, z);
                        #pragma omp flush
                    }
                }
            }
        }
    }

    // TODO: make this more efficient. Currently i'm using active areas from the containing balls computation,
    // which have margins, though these margins are not needed in this function.
    // If there are many active areas, there will be overhead on reading margins.
    // ALso, i need only containingBallPixelIndexes and maximumBallsMask here.
    // Move to another service.
    void ContainingBallsAssigner::SetMaximumBallsMaskInActiveArea() const
    {
        printf("Initializing maximum balls mask in the active area...\n");

        ResetPercentagePrinting(Axis::X);

        #pragma omp parallel for schedule(static)
        for (int x = activeArea->activeBox.leftCorner[Axis::X]; x < activeArea->activeBox.exclusiveRightCorner[Axis::X]; ++x)
        {
            PrintPercentageCompleted();

            for (int y = activeArea->activeBox.leftCorner[Axis::Y]; y < activeArea->activeBox.exclusiveRightCorner[Axis::Y]; ++y)
            {
                for (int z = activeArea->activeBox.leftCorner[Axis::Z]; z < activeArea->activeBox.exclusiveRightCorner[Axis::Z]; ++z)
                {
                    PixelCoordinateType pixelCoordinateX = dataArrays.containingBallCoordinatesX.GetPixel(x, y, z);

                    bool isSolidPixel = (pixelCoordinateX == UNKNOWN_CONTAINING_BALL_COORDINATE);
                    IsMaximumBallMaskType maximumBallMask = isSolidPixel ? SOLID_BALL_MASK : INNER_BALL_MASK;
                    dataArrays.maximumBallsMask.SetPixel(x, y, z, maximumBallMask);
                    // don't have to flush, as these pixels will never be met again
                }
            }
        }

        ResetPercentagePrinting(Axis::X);
        size_t localMaximumBallsCount = 0;

        printf("Setting maximum balls mask in the active area...\n");
        #pragma omp parallel for schedule(static) reduction(+:localMaximumBallsCount)
        for (int x = activeArea->activeBox.leftCorner[Core::Axis::X]; x < activeArea->activeBox.exclusiveRightCorner[Core::Axis::X]; ++x)
        {
            PrintPercentageCompleted();

            for (int y = activeArea->activeBox.leftCorner[Core::Axis::Y]; y < activeArea->activeBox.exclusiveRightCorner[Core::Axis::Y]; ++y)
            {
                for (int z = activeArea->activeBox.leftCorner[Core::Axis::Z]; z < activeArea->activeBox.exclusiveRightCorner[Core::Axis::Z]; ++z)
                {
                    PixelCoordinateType pixelCoordinateX = dataArrays.containingBallCoordinatesX.GetPixel(x, y, z);
                    bool isSolidPixel = (pixelCoordinateX == UNKNOWN_CONTAINING_BALL_COORDINATE);
                    if (isSolidPixel)
                    {
                        continue;
                    }

                    PixelCoordinateType pixelCoordinateY = dataArrays.containingBallCoordinatesY.GetPixel(x, y, z);
                    PixelCoordinateType pixelCoordinateZ = dataArrays.containingBallCoordinatesZ.GetPixel(x, y, z);

                    IsMaximumBallMaskType maximumBallMask = dataArrays.maximumBallsMask.GetPixel(pixelCoordinateX, pixelCoordinateY, pixelCoordinateZ);
                    if (maximumBallMask == INNER_BALL_MASK)
                    {
                        dataArrays.maximumBallsMask.SetPixel(pixelCoordinateX, pixelCoordinateY, pixelCoordinateZ, OUTER_BALL_MASK);
                        #pragma omp flush // may even omit this flush. This pixel will just be possibly marked as outer several times
                        localMaximumBallsCount++;
                    }
                }
            }
        }

        maximumBallsCount += localMaximumBallsCount;
    }

    ContainingBallsAssigner::~ContainingBallsAssigner()
    {
    }
}

