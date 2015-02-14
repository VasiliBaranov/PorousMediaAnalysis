// Copyright (c) 2013 Vasili Baranau
// Distributed under the MIT software license
// See the accompanying file License.txt or http://opensource.org/licenses/MIT

#include "../Headers/EuclideanDistanceComputer.h"

#include <string>
#include <cmath>
#include "stdio.h"

#include "Core/Headers/OpenMpManager.h"
#include "Core/Headers/Path.h"
#include "ImageProcessing/Services/Headers/ActiveAreaComputer.h"
#include "ImageProcessing/Services/Headers/Serializer.h"
#include "ImageProcessing/Model/Headers/Config.h"

using namespace std;
using namespace Core;
using namespace Model;
using namespace Services;

namespace Services
{
    EuclideanDistanceComputer::EuclideanDistanceComputer(const ActiveAreaComputer& currentActiveAreaComputer, const Serializer& currentSerializer) :
        BaseDataArrayProcessor(currentActiveAreaComputer, currentSerializer)
    {
        dataArrays.AddActiveDataArrayTypes(DataArrayFlags::IsSolidMask | DataArrayFlags::EuclideanDistanceSquares);
    }

    void EuclideanDistanceComputer::ComputeEuclideanDistances(const Config& config) const
    {
        this->config = &config;
        FillActiveAreas();

        dataArrays.Initialize(config, activeAreas);

        maxEuclideanDistanceSquare = 0;
        euclideanDistanceSquareCounts.clear();
        FindAllDistancesToSolidsInXY();
        FindAllDistancesToSolidsInXYZ();

        WriteResults();
        dataArrays.Clear();
    }

    void EuclideanDistanceComputer::FillActiveAreas() const
    {
        int bytesPerPixel = dataArrays.GetBytesPerPixel();

        vector<Axis::Type> priorityAxes(2);

        priorityAxes[0] = Axis::X;
        priorityAxes[1] = Axis::Y;

        Margin margin(Margin::Pixels, 0);
        activeAreaComputer.ComputeActiveAreas(*config, priorityAxes, margin, bytesPerPixel, &activeAreas);
        xyActiveAreasSize = activeAreas.size();

        priorityAxes.resize(1);
        priorityAxes[0] = Axis::Z;

        vector<ActiveArea> zActiveAreas;
        activeAreaComputer.ComputeActiveAreas(*config, priorityAxes, margin, bytesPerPixel, &zActiveAreas);
        activeAreas.insert(activeAreas.end(), zActiveAreas.begin(), zActiveAreas.end());
    }

    void EuclideanDistanceComputer::WriteResults() const
    {
        WriteIntermediateStatistics();

        WriteEuclideanDistanceSquareCounts();
    }

    void EuclideanDistanceComputer::WriteIntermediateStatistics() const
    {
        // NOTE: here i assume that this class is the first that creates intermediate statistics. TODO: remove this dependency, read this file.
        IntermediateStatistics intermediateStatistics;
        intermediateStatistics.maxEuclideanDistanceSquare = maxEuclideanDistanceSquare;
        intermediateStatistics.maximumBallsCount = 0;
        BaseDataArrayProcessor::WriteIntermediateStatistics(intermediateStatistics);
    }

    void EuclideanDistanceComputer::WriteEuclideanDistanceSquareCounts() const
    {
        int endIndex = euclideanDistanceSquareCounts.size() - 1;
        int lastNonZeroIndex = -1;
        for (int index = endIndex; index >= 0; --index)
        {
            size_t value = euclideanDistanceSquareCounts[index];
            if (value != 0)
            {
                lastNonZeroIndex = index;
                break;
            }
        }

        if (lastNonZeroIndex == -1)
        {
            // just to show in the saved file that it's empty because there are no pores (that do not cross the boundary)
            euclideanDistanceSquareCounts.resize(1, 0);
        }
        else
        {
            int nonZeroElementsCount = lastNonZeroIndex + 1;
            int nonZeroElementsCountPlusOneZero = nonZeroElementsCount + 1;
            euclideanDistanceSquareCounts.resize(nonZeroElementsCountPlusOneZero, 0);
        }

        serializer.WriteEuclideanDistanceSquareCounts(Path::Append(config->baseFolder, EUCLIDEAN_DISTANCE_SQUARE_COUNTS_FILE_NAME), euclideanDistanceSquareCounts);
    }

    void EuclideanDistanceComputer::FindAllDistancesToSolidsInXYZ() const
    {
        printf("Finding all distances to solids...\n");

        for (size_t activeAreaIndex = xyActiveAreasSize; activeAreaIndex < activeAreas.size(); ++activeAreaIndex)
        {
            printf("Active area " SIZE_T_FORMAT " / " SIZE_T_FORMAT "...\n", activeAreaIndex + 1 - xyActiveAreasSize, activeAreas.size() - xyActiveAreasSize);

            dataArrays.ChangeActiveArea(activeAreaIndex);

            activeArea = &activeAreas[activeAreaIndex];

            FindDistancesToSolidsInXYZ();
        }

        dataArrays.WriteCurrentActiveArea();
    }

    void EuclideanDistanceComputer::FindAllDistancesToSolidsInXY() const
    {
        printf("Finding all distances to solids in the XY plane...\n");

        for (size_t activeAreaIndex = 0; activeAreaIndex < xyActiveAreasSize; ++activeAreaIndex)
        {
            printf("Active area " SIZE_T_FORMAT " / " SIZE_T_FORMAT "...\n", activeAreaIndex + 1, xyActiveAreasSize);

            dataArrays.ChangeActiveArea(activeAreaIndex);

            activeArea = &activeAreas[activeAreaIndex];

            FindDistancesToSolidsAlongX();
            FindDistancesToSolidsInXY();
        }

        dataArrays.WriteCurrentActiveArea();
    }

    void EuclideanDistanceComputer::FindDistancesToSolidsAlongX() const
    {
        printf("Finding all distances to solids along X in the active area...\n");
        ResetPercentagePrinting(Axis::Y);

        // Each process shall get and set non-intersecting parts of euclideanDistanceSquares. Thus, i do not use any critical sections.
        // We ensure that the entire y dimension is loaded when computing active areas
        #pragma omp parallel for schedule(static)
        for (int y = 0; y < config->imageSize[Axis::Y]; ++y)
        {
            PrintPercentageCompleted();

            for (int z = activeArea->activeBox.leftCorner[Axis::Z]; z < activeArea->activeBox.exclusiveRightCorner[Axis::Z]; ++z)
            {
                // Forward scan
                EuclideanDistanceSquareType distanceToSolid = BOUNDARIES_ARE_SOLID ? 0 : (config->imageSize[Axis::X] - 1);
                for (int x = 0; x < config->imageSize[Axis::X]; ++x)
                {
                    if (dataArrays.isSolidMask.GetPixel(x, y, z) == 0)
                    {
                        distanceToSolid++;
                    }
                    else
                    {
                        distanceToSolid = 0;
                    }

                    dataArrays.euclideanDistanceSquares.SetPixel(x, y, z, distanceToSolid * distanceToSolid);
                }

                // Backward scan
                distanceToSolid = BOUNDARIES_ARE_SOLID ? 0 : (config->imageSize[Axis::X] - 1);
                for (int x = config->imageSize[Axis::X] - 1; x >= 0; --x)
                {
                    if (dataArrays.isSolidMask.GetPixel(x, y, z) == 0)
                    {
                        distanceToSolid++;
                    }
                    else
                    {
                        distanceToSolid = 0;
                    }

                    EuclideanDistanceSquareType forwardScanValue = dataArrays.euclideanDistanceSquares.GetPixel(x, y, z);
                    EuclideanDistanceSquareType backwardScanValue = distanceToSolid * distanceToSolid;

                    if (backwardScanValue < forwardScanValue)
                    {
                        dataArrays.euclideanDistanceSquares.SetPixel(x, y, z, backwardScanValue);
                    }
                }
            }
        }
    }

    void EuclideanDistanceComputer::FindDistancesToSolidsInXY() const
    {
        printf("Finding all distances to solids in the XY plane in the active area...\n");
        ResetPercentagePrinting(Axis::X);

        // Each process shall get and set non-intersecting parts of euclideanDistanceSquares. Thus, i do not use any critical sections.
        #pragma omp parallel for schedule(static)
        for (int x = 0; x < config->imageSize[Axis::X]; ++x)
        {
            PrintPercentageCompleted();

            for (int z = activeArea->activeBox.leftCorner[Axis::Z]; z < activeArea->activeBox.exclusiveRightCorner[Axis::Z]; ++z)
            {
                vector<EuclideanDistanceSquareType> distanceToSolidSquaresAlongX(config->imageSize[Axis::Y]);
                for (int y = 0; y < config->imageSize[Axis::Y]; ++y)
                {
                    distanceToSolidSquaresAlongX[y] = dataArrays.euclideanDistanceSquares.GetPixel(x, y, z);
                }

                for (int y = 0; y < config->imageSize[Axis::Y]; ++y)
                {
                    EuclideanDistanceSquareType distanceToSolidSquare = distanceToSolidSquaresAlongX[y];

                    // Solid
                    if (distanceToSolidSquare == 0)
                    {
                        // euclideanDistanceSquares pixel remain zero
                        continue;
                    }

                    int maxDistanceToSolidY = sqrt(static_cast<FLOAT_TYPE>(distanceToSolidSquare)) + 1;
                    int minPixelDifferenceY = std::min(maxDistanceToSolidY, y);
                    int maxPixelDifferenceY = std::min(maxDistanceToSolidY, (config->imageSize[Axis::Y] - y));
                    for (int pixelDifferenceY = -minPixelDifferenceY; pixelDifferenceY < maxPixelDifferenceY; ++pixelDifferenceY)
                    {
                        EuclideanDistanceSquareType currentDistanceToSolidSquare = distanceToSolidSquaresAlongX[y + pixelDifferenceY] + pixelDifferenceY * pixelDifferenceY;
                        if (currentDistanceToSolidSquare < distanceToSolidSquare)
                        {
                            distanceToSolidSquare = currentDistanceToSolidSquare;
                        }
                    }

                    dataArrays.euclideanDistanceSquares.SetPixel(x, y, z, distanceToSolidSquare);
                }
            }
        }
    }

    // TODO: refactor
    void EuclideanDistanceComputer::FindDistancesToSolidsInXYZ() const
    {
        vector< vector<EuclideanDistanceSquareType> > distanceToSolidSquaresAlongXyByThread(OpenMpManager::GetMaxPossibleNumberOfThreads());
        vector< vector<size_t> > euclideanDistanceSquareCountsByThread(OpenMpManager::GetMaxPossibleNumberOfThreads());
        vector<Model::EuclideanDistanceSquareType> maxEuclideanDistanceSquaresByThread(OpenMpManager::GetMaxPossibleNumberOfThreads(), 0);

        for (size_t i = 0; i < distanceToSolidSquaresAlongXyByThread.size(); ++i)
        {
            distanceToSolidSquaresAlongXyByThread[i].resize(config->imageSize[Axis::Z]);
        }

        printf("Finding all distances to solids in the active area...\n");
        ResetPercentagePrinting(Axis::X);

        // Each process shall get and set non-intersecting parts of euclideanDistanceSquares. Thus, i do not use any critical sections.
        #pragma omp parallel for shared(distanceToSolidSquaresAlongXyByThread, euclideanDistanceSquareCountsByThread, maxEuclideanDistanceSquaresByThread) schedule(static)
        for (int x = activeArea->activeBox.leftCorner[Axis::X]; x < activeArea->activeBox.exclusiveRightCorner[Axis::X]; ++x)
        {
            PrintPercentageCompleted();

            for (int y = activeArea->activeBox.leftCorner[Axis::Y]; y < activeArea->activeBox.exclusiveRightCorner[Axis::Y]; ++y)
            {
                vector<EuclideanDistanceSquareType>& distanceToSolidSquaresAlongXY = distanceToSolidSquaresAlongXyByThread[OpenMpManager::GetCurrentThreadIndex()];
                vector<size_t>& euclideanDistanceSquareCounts = euclideanDistanceSquareCountsByThread[OpenMpManager::GetCurrentThreadIndex()];
                Model::EuclideanDistanceSquareType& maxEuclideanDistanceSquare = maxEuclideanDistanceSquaresByThread[OpenMpManager::GetCurrentThreadIndex()];

                for (int z = 0; z < config->imageSize[Axis::Z]; ++z)
                {
                    distanceToSolidSquaresAlongXY[z] = dataArrays.euclideanDistanceSquares.GetPixel(x, y, z);
                }

                for (int z = 0; z < config->imageSize[Axis::Z]; ++z)
                {
                    EuclideanDistanceSquareType distanceToSolidSquare = distanceToSolidSquaresAlongXY[z];

                    // Solid
                    if (distanceToSolidSquare == 0)
                    {
                        // euclideanDistanceSquares pixel remain zero
                        continue;
                    }

                    int maxDistanceToSolidZ = sqrt(static_cast<FLOAT_TYPE>(distanceToSolidSquare)) + 1;
                    int minPixelDifferenceZ = std::min(maxDistanceToSolidZ, z);
                    int maxPixelDifferenceZ = std::min(maxDistanceToSolidZ, (config->imageSize[Axis::Z] - z));

                    for (int pixelDifferenceZ = -minPixelDifferenceZ; pixelDifferenceZ < maxPixelDifferenceZ; ++pixelDifferenceZ)
                    {
                        EuclideanDistanceSquareType currentDistanceToSolidSquare = distanceToSolidSquaresAlongXY[z + pixelDifferenceZ] + pixelDifferenceZ * pixelDifferenceZ;
                        if (currentDistanceToSolidSquare < distanceToSolidSquare)
                        {
                            distanceToSolidSquare = currentDistanceToSolidSquare;
                        }
                    }

                    dataArrays.euclideanDistanceSquares.SetPixel(x, y, z, distanceToSolidSquare);

                    if (maxEuclideanDistanceSquare < distanceToSolidSquare)
                    {
                        maxEuclideanDistanceSquare = distanceToSolidSquare;
                        euclideanDistanceSquareCounts.resize(maxEuclideanDistanceSquare + 1, 0);
                    }

                    FLOAT_TYPE distanceToSolid = sqrt(static_cast<FLOAT_TYPE>(distanceToSolidSquare));
                    bool crossImageBoundary = distanceToSolid > x ||
                            distanceToSolid > y ||
                            distanceToSolid > z ||
                            (x + distanceToSolid > (config->imageSize[Axis::X] - 1)) ||
                            (y + distanceToSolid > (config->imageSize[Axis::Y] - 1)) ||
                            (z + distanceToSolid > (config->imageSize[Axis::Z] - 1));

                    if (!crossImageBoundary)
                    {
                        euclideanDistanceSquareCounts[distanceToSolidSquare]++;
                    }
                }
            }
        }

        maxEuclideanDistanceSquare = StlUtilities::FindMaxElement(maxEuclideanDistanceSquaresByThread);
        euclideanDistanceSquareCounts.resize(maxEuclideanDistanceSquare + 1);

        for (size_t i = 0; i < euclideanDistanceSquareCountsByThread.size(); ++i)
        {
            vector<size_t>& currentEuclideanDistanceSquareCounts = euclideanDistanceSquareCountsByThread[i];
            for (size_t euclideanDistanceSquare = 0; euclideanDistanceSquare < currentEuclideanDistanceSquareCounts.size(); ++euclideanDistanceSquare)
            {
                euclideanDistanceSquareCounts[euclideanDistanceSquare] += currentEuclideanDistanceSquareCounts[euclideanDistanceSquare];
            }
        }
    }

    EuclideanDistanceComputer::~EuclideanDistanceComputer()
    {
    }
}

