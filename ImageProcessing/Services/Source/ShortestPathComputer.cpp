// Copyright (c) 2013 Vasili Baranau
// Distributed under the MIT software license
// See the accompanying file License.txt or http://opensource.org/licenses/MIT

#include "../Headers/ShortestPathComputer.h"

#include <string>
#include <cmath>
#include "stdio.h"

#include "Core/Headers/OpenMpManager.h"
#include "Core/Headers/Path.h"
#include "Core/Headers/OrderedPriorityQueue.h"
#include "ImageProcessing/Services/Headers/ActiveAreaComputer.h"
#include "ImageProcessing/Services/Headers/Serializer.h"
#include "ImageProcessing/Model/Headers/Config.h"
#include "ImageProcessing/Model/Headers/ModelUtilities.h"

using namespace std;
using namespace Core;
using namespace Model;
using namespace Services;

namespace Services
{
    ShortestPathComputer::ShortestPathComputer(const ActiveAreaComputer& currentActiveAreaComputer, const Serializer& currentSerializer) :
        BaseDataArrayProcessor(currentActiveAreaComputer, currentSerializer)
    {
        dataArrays.AddActiveDataArrayTypes(DataArrayFlags::IsSolidMask | DataArrayFlags::ShortestPathDistances | DataArrayFlags::CavityIds);

        int maxDistanceSquare = 3; // 1 * 1 + 1 *1 + 1 * 1
        pixelDifferenceSqrts.resize(maxDistanceSquare + 1);
        for (size_t distanceSquare = 0; distanceSquare < pixelDifferenceSqrts.size(); ++distanceSquare)
        {
            pixelDifferenceSqrts[distanceSquare] = sqrt(static_cast<ShortestPathDistanceType>(distanceSquare));
        }
    }

    void ShortestPathComputer::ComputeShortestPathDistances(const Config& config) const
    {
        reachedBoundaryPlanes.clear();

        this->config = &config;
        FillActiveAreas();

        dataArrays.Initialize(config, activeAreas);
        // activeAreas.size() is currently always 1
        dataArrays.ChangeActiveArea(0);

        printf("Computing shortest path distances...\n");
        vector<CavityDescription> cavityDescriptions;
        ComputeShortestPathDistances(&cavityDescriptions);

        serializer.WriteCavityDescriptions(Path::Append(config.baseFolder, CAVITY_DESCRIPTIONS_FILE_NAME), cavityDescriptions);
        dataArrays.WriteCurrentActiveArea();
        dataArrays.Clear();
    }

    void ShortestPathComputer::FillActiveAreas() const
    {
        int bytesPerPixel = dataArrays.GetBytesPerPixel();
        // Node.position + Node.isprocessedOrSolid + OrderedPriorityQueue.valueIndexes + OrderedPriorityQueue.permutation
        bytesPerPixel += (3 * sizeof(int) + sizeof(bool) + sizeof(size_t) + sizeof(size_t));

        vector<Axis::Type> priorityAxes;
        Margin margin(Margin::Pixels, 0);
        activeAreaComputer.ComputeActiveAreas(*config, priorityAxes, margin, bytesPerPixel, &activeAreas);

        if (activeAreas.size() > 1)
        {
            // TODO: search for the shortest path algorithms that can operate "piece by piece"
            // TODO: search also for the algorithms that can be parallelized (inside a single piece or between pieces)
            // TODO: implement parallel version that operates "piece by piece"
            throw InvalidOperationException("Not enough available memory. Shortest path distances can be computed only when the entire monolith is loaded.");
        }
    }

    void ShortestPathComputer::ComputeShortestPathDistances(vector<CavityDescription>* cavities) const
    {
        int boxHalfSize = 0;
        bool firstIteration = true;
        cavities->clear();

        int wallsCount = GetWallsCount();

        vector<Node> unvisitedNodes;
        OrderedPriorityQueue<vector<Node>, NodeComparer> unvisitedNodesQueue;
        InitializeDataArraysAndUnvisitedNodesAndQueue(&unvisitedNodes, &unvisitedNodesQueue);

        currentCavityId = 0;
        processedPixelsCount = 0;
        size_t previousProcessedPixelsCount = 0;

        while (true)
        {
            // Select a void pixel in the center of the monolith
            DiscreteSpatialVector startingPosition;
            boxHalfSize++;
            bool positionFound = SelectStartingPosition(&startingPosition, &boxHalfSize);
            if (!positionFound)
            {
                if (firstIteration)
                {
                    // throw InvalidOperationException("There are no void pixels in the monolith");
                    printf("WARNING: There are no void pixels in the monolith\n");
                    break;
                }
                else
                {
                    printf("WARNING: There are no cavities that touch all the %d boundary walls simultaneously\n", wallsCount);
                    break;
                }
            }

            // Compute shortest paths
            reachedBoundaryPlanes.clear();
            currentCavityId += 127;
            ComputeShortestPathDistances(startingPosition, &unvisitedNodes, &unvisitedNodesQueue);

            printf("Finished computing shortest path distances for current starting pixel\n");

            AddCavityDescription(startingPosition, previousProcessedPixelsCount, processedPixelsCount, cavities);
            previousProcessedPixelsCount = processedPixelsCount;

            // Check if have reached all the walls
            bool reachedAllBoundaryPlanes = reachedBoundaryPlanes.size() == wallsCount;

            firstIteration = false;
            if (!reachedAllBoundaryPlanes)
            {
                printf("Could not reach all the %d boundary walls from this starting pixel. Selecting another starting pixel\n", wallsCount);
            }
            else
            {
                // We may continue here until we process all the void pixels. It may be convenient with monoliths with several large cavities.
                // In our case we usually have one large pore network and several cavities, so one pore network is enough. And it is faster.
                // And almost all the cavities can be viewed as unknown distances in the shortest path distances images.
                break;
            }
        }
    }

    void ShortestPathComputer::AddCavityDescription(const DiscreteSpatialVector& startingPosition, size_t previousProcessedPixelsCount, size_t processedPixelsCount, vector<CavityDescription>* cavities) const
    {
        int wallsCount = GetWallsCount();

        CavityDescription cavityDescription;
        cavityDescription.id = currentCavityId;
        cavityDescription.seed = startingPosition;
        cavityDescription.reachedWallsCount = reachedBoundaryPlanes.size();
        cavityDescription.isReachingAllWalls = reachedBoundaryPlanes.size() == wallsCount;
        cavityDescription.voidPixelsCount = processedPixelsCount - previousProcessedPixelsCount;

        DiscreteSpatialVector boxCenter;
        FillMonolithCenter(&boxCenter);

        DiscreteSpatialVector difference;
        VectorUtilities::Subtract(startingPosition, boxCenter, &difference);
        int lengthSquare = VectorUtilities::GetSelfDotProduct(difference);
        double distanceToCenter = sqrt(static_cast<double>(lengthSquare));

        cavityDescription.euclideanDistanceToMonolithCenter = distanceToCenter;
        cavities->push_back(cavityDescription);
    }

    // Just took Dijkstra' algorithm from http://en.wikipedia.org/wiki/Dijkstra%27s_algorithm
    void ShortestPathComputer::ComputeShortestPathDistances(const DiscreteSpatialVector& startingPosition, vector<Node>* unvisitedNodes, OrderedPriorityQueue<vector<Node>, NodeComparer>* unvisitedNodesQueue) const
    {
        vector<Node>& unvisitedNodesRef = *unvisitedNodes;
        OrderedPriorityQueue<vector<Node>, NodeComparer>& unvisitedNodesQueueRef = *unvisitedNodesQueue;

        dataArrays.shortestPathDistances.SetPixel(startingPosition, 0);

        size_t neighborLinearIndex = GetLinearIndex(startingPosition);
        unvisitedNodesQueue->HandleUpdate(neighborLinearIndex);

        while(true)
        {
            size_t topIndex = unvisitedNodesQueueRef.GetTopIndex();
            Node& closestNode = unvisitedNodesRef[topIndex];
            ShortestPathDistanceType distanceToCurrentPixel = dataArrays.shortestPathDistances.GetPixel(closestNode.position);
            
            bool processedAllVoidNodes = closestNode.processedOrSolid;
            bool processedAllAchievableVoidNodes = distanceToCurrentPixel == UNKNOWN_SHORTEST_PATH_DISTANCE;
            // If closestNode is solid, distanceToCurrentPixel has a special invalid value UNKNOWN_SHORTEST_PATH_DISTANCE.
            // We do not check it, as it is handled during processedAllVoidNodes check.
            if (processedAllVoidNodes || processedAllAchievableVoidNodes)
            {
                break;
            }

            dataArrays.cavityIds.SetPixel(closestNode.position, currentCavityId);

            closestNode.processedOrSolid = true;
            unvisitedNodesQueueRef.HandleUpdate(topIndex);

            processedPixelsCount++;

            if (processedPixelsCount % 10000 == 0)
            {
                printf("Processed " SIZE_T_FORMAT " void pixels of " SIZE_T_FORMAT " void pixels\n", processedPixelsCount, voidPixelsCount);
            }

            UpdateReachedBoundaryPlanes(closestNode.position);

            UpdateDistancesToNeighbors(closestNode, distanceToCurrentPixel, unvisitedNodesQueue);
        }
    }

    void ShortestPathComputer::UpdateReachedBoundaryPlanes(const DiscreteSpatialVector& position) const
    {
        for (int i = 0; i < DIMENSIONS; ++i)
        {
            if (position[i] == 0 || position[i] == (config->imageSize[i] - 1))
            {
                BoxPlane boundaryPlane;
                boundaryPlane.normalAxis = (Axis::Type)i;
                boundaryPlane.coordinateOnNormalAxis = position[i];

                bool planeVisited = StlUtilities::Contains(reachedBoundaryPlanes, boundaryPlane);
                if (!planeVisited)
                {
                    reachedBoundaryPlanes.push_back(boundaryPlane);
                }
            }
        }
    }

    int ShortestPathComputer::GetWallsCount() const
    {
        vector<BoxPlane> totalWalls;
        FillAllBoundaryPlanes(&totalWalls);
        int wallsCount = totalWalls.size();

        return wallsCount;
    }

    // Usually there are six unique boundary walls. But if the monolith has width 1 along any dimension, there may be less unique boundary walls
    void ShortestPathComputer::FillAllBoundaryPlanes(vector<BoxPlane>* boundaryPlanes) const
    {
        boundaryPlanes->clear();
        for (int i = 0; i < DIMENSIONS; ++i)
        {
            vector<int> coordinateOnNormalAxis(2);
            coordinateOnNormalAxis[0] = 0;
            coordinateOnNormalAxis[1] = config->imageSize[i] - 1;

            for (int j = 0; j < coordinateOnNormalAxis.size(); ++j)
            {
                BoxPlane boundaryPlane;
                boundaryPlane.normalAxis = (Axis::Type)i;
                boundaryPlane.coordinateOnNormalAxis = coordinateOnNormalAxis[j];

                bool planeVisited = StlUtilities::Contains(*boundaryPlanes, boundaryPlane);
                if (!planeVisited)
                {
                    boundaryPlanes->push_back(boundaryPlane);
                }
            }
        }
    }

    void ShortestPathComputer::UpdateDistancesToNeighbors(Node& closestNode, ShortestPathDistanceType distanceToCurrentPixel, OrderedPriorityQueue<vector<Node>, NodeComparer>* unvisitedNodesQueue) const
    {
        for (int neighborX = closestNode.position[Axis::X] - 1; neighborX <= closestNode.position[Axis::X] + 1; ++neighborX)
        {
            for (int neighborY = closestNode.position[Axis::Y] - 1; neighborY <= closestNode.position[Axis::Y] + 1; ++neighborY)
            {
                for (int neighborZ = closestNode.position[Axis::Z] - 1; neighborZ <= closestNode.position[Axis::Z] + 1; ++neighborZ)
                {
                    DiscreteSpatialVector neighborVector = {{neighborX, neighborY, neighborZ}};

                    if (closestNode.position == neighborVector)
                    {
                        continue;
                    }

                    if ((config->stencilType == StencilType::D3Q7) &&
                                ModelUtilities::IsDiagonalStencilVector(closestNode.position, neighborVector))
                    {
                        continue;
                    }

                    if (ModelUtilities::IsOuterPixel(neighborVector, *config))
                    {
                        continue;
                    }

                    bool neighborIsSolid = dataArrays.isSolidMask.GetPixel(neighborVector) != 0;
                    if (neighborIsSolid)
                    {
                        continue;
                    }

                    ShortestPathDistanceType distanceAddition = GetDistanceToNeighbor(closestNode.position, neighborVector);
                    ShortestPathDistanceType distanceToNeighbor = dataArrays.shortestPathDistances.GetPixel(neighborX, neighborY, neighborZ);
                    ShortestPathDistanceType newDistanceToNeighbor = distanceToCurrentPixel + distanceAddition;

                    if (newDistanceToNeighbor < distanceToNeighbor)
                    {
                        dataArrays.shortestPathDistances.SetPixel(neighborVector, newDistanceToNeighbor);

                        size_t neighborLinearIndex = GetLinearIndex(neighborVector);
                        unvisitedNodesQueue->HandleUpdate(neighborLinearIndex);
                    }
                }
            }
        }
    }

    size_t ShortestPathComputer::GetLinearIndex(const Core::DiscreteSpatialVector& position) const
    {
        size_t linearIndex =
            static_cast<size_t>(config->imageSize[Axis::Y]) * config->imageSize[Axis::Z] * position[Axis::X] +
            static_cast<size_t>(config->imageSize[Axis::Z]) * position[Axis::Y] +
            position[Axis::Z];

        return linearIndex;
    }

    ShortestPathDistanceType ShortestPathComputer::GetDistanceToNeighbor(const DiscreteSpatialVector& nodePosition, const DiscreteSpatialVector& neighborPosition) const
    {
        if (config->stencilType == StencilType::D3Q7)
        {
            return 1.0;
        }

        DiscreteSpatialVector difference;
        VectorUtilities::Subtract(nodePosition, neighborPosition, &difference);

        // We know that difference components can be only -1, 0, and +1.
        // So instead of taking dot product we may simply sum absolute values
        VectorUtilities::Abs(difference, &difference);
        int differenceSquare = VectorUtilities::Sum(difference);

        return pixelDifferenceSqrts[differenceSquare];
    }

    void ShortestPathComputer::InitializeDataArraysAndUnvisitedNodes(vector<Node>* unvisitedNodes) const
    {
        size_t pixelsCount = ModelUtilities::GetPixelsCount(*config);
        voidPixelsCount = 0;
        unvisitedNodes->clear();
        unvisitedNodes->reserve(pixelsCount);

        printf("Initializing unvisited nodes...");

        for (int x = 0; x < config->imageSize[Axis::X]; ++x)
        {
            for (int y = 0; y < config->imageSize[Axis::Y]; ++y)
            {
                for (int z = 0; z < config->imageSize[Axis::Z]; ++z)
                {
                    bool isSolid = dataArrays.isSolidMask.GetPixel(x, y, z) != 0;
                    DiscreteSpatialVector position = {{x, y, z}};

                    Node node;
                    node.position = position;

                    // Void pixels are all set to unprocessed
                    // Put solid pixels as well to be able to recover pixel position in the queue by pixel coordinates. 
                    // They are marked as processed since the very beginning, so they will always be at the end of the queue
                    node.processedOrSolid = isSolid;

                    unvisitedNodes->push_back(node);

                    // Set different values to solid pixels to make them distinguishable in images from unreached pixels
                    ShortestPathDistanceType shortestPathPixelValue = isSolid ? SOLID_SHORTEST_PATH_DISTANCE : UNKNOWN_SHORTEST_PATH_DISTANCE;
                    dataArrays.shortestPathDistances.SetPixel(position, shortestPathPixelValue);

                    CavityIdType cavityIdPixelValue = isSolid ? SOLID_CAVITY_ID : UNKNOWN_CAVITY_ID;
                    dataArrays.cavityIds.SetPixel(position, cavityIdPixelValue);

                    if (!isSolid)
                    {
                        voidPixelsCount++;
                    }
                }
            }

            printf("Finished %d layers of %d...\n", x + 1, config->imageSize[Axis::X]);
        }
    }

    void ShortestPathComputer::InitializeDataArraysAndUnvisitedNodesAndQueue(vector<Node>* unvisitedNodes, OrderedPriorityQueue<vector<Node>, NodeComparer>* unvisitedNodesQueue) const
    {
        InitializeDataArraysAndUnvisitedNodes(unvisitedNodes);

        NodeComparer comparer;
        comparer.shortestPathDistances = &dataArrays.shortestPathDistances;

        printf("Initializing unvisited nodes queue\n");
        unvisitedNodesQueue->Initialize(unvisitedNodes, comparer);
    }

    bool ShortestPathComputer::SelectStartingPosition(DiscreteSpatialVector* startingPosition, int* boxHalfSize) const
    {
        // Select a pixel in the center
        DiscreteSpatialVector boxCenter;
        FillMonolithCenter(&boxCenter);

        // Select a void pixel closest to the center
        bool isVoid = dataArrays.isSolidMask.GetPixel(boxCenter) == 0;
        bool isNotProcessed = dataArrays.shortestPathDistances.GetPixel(boxCenter) == UNKNOWN_SHORTEST_PATH_DISTANCE;
        if (isVoid && isNotProcessed)
        {
            StlUtilities::Copy(boxCenter, startingPosition);
            return true;
        }

        int maxBoxHalfSize = GetMaxBoxHalfSize();

        int& boxHalfSizeRef = *boxHalfSize;
        int startingBoxHalfSize = boxHalfSizeRef;

        for (boxHalfSizeRef = startingBoxHalfSize; boxHalfSizeRef < maxBoxHalfSize; ++boxHalfSizeRef)
        {
            bool voidFound = FindVoidPixelOnBoxBoundaries(boxCenter, boxHalfSizeRef, startingPosition);
            if (voidFound)
            {
                return true;
            }
        }

        return false;
    }

    int ShortestPathComputer::GetMaxBoxHalfSize() const
    {
        DiscreteSpatialVector boxCenter;
        FillMonolithCenter(&boxCenter);

        DiscreteSpatialVector imageSizeInclusive;
        FillImageSizeInclusive(&imageSizeInclusive);
        DiscreteSpatialVector distanceToMaxCorner;
        VectorUtilities::Subtract(imageSizeInclusive, boxCenter, &distanceToMaxCorner);
        DiscreteSpatialVector distanceToMinCorner = boxCenter;
        int maxBoxHalfSize = max(VectorUtilities::GetMaxValue(distanceToMaxCorner), VectorUtilities::GetMaxValue(distanceToMinCorner));

        return maxBoxHalfSize;
    }

    void ShortestPathComputer::FillMonolithCenter(DiscreteSpatialVector* center) const
    {
        SpatialVector continuousCenter;
        VectorUtilities::MultiplyByValue(config->imageSize, 0.5, &continuousCenter);

        VectorUtilities::Round(continuousCenter, center);

        DiscreteSpatialVector imageSizeInclusive;
        FillImageSizeInclusive(&imageSizeInclusive);
        VectorUtilities::Min(*center, imageSizeInclusive, center);
    }

    void ShortestPathComputer::FillImageSizeInclusive(DiscreteSpatialVector* imageSizeInclusive) const
    {
        VectorUtilities::SubtractValue(config->imageSize, 1, imageSizeInclusive);
    }

    bool ShortestPathComputer::FindVoidPixelOnBoxBoundaries(const DiscreteSpatialVector& boxCenter, int boxHalfSize, DiscreteSpatialVector* position) const
    {
        DiscreteSpatialVector currentPosition;
        for (int constCoordinateIndex = 0; constCoordinateIndex < DIMENSIONS; ++constCoordinateIndex)
        {
            vector<int> coordiateIndexes(3);
            VectorUtilities::FillLinearScale(0, &coordiateIndexes);
            StlUtilities::Remove(&coordiateIndexes, constCoordinateIndex);

            int minFirstCoordinate = max(boxCenter[coordiateIndexes[0]] - boxHalfSize, 0);
            int maxFirstCoordinate = min(boxCenter[coordiateIndexes[0]] + boxHalfSize, config->imageSize[coordiateIndexes[0]] - 1);

            int minSecondCoordinate = max(boxCenter[coordiateIndexes[1]] - boxHalfSize, 0);
            int maxSecondCoordinate = min(boxCenter[coordiateIndexes[1]] + boxHalfSize, config->imageSize[coordiateIndexes[1]] - 1);

            vector<int> fixedCoordinates(2);
            fixedCoordinates[0] = max(boxCenter[constCoordinateIndex] - boxHalfSize, 0);
            fixedCoordinates[1] = min(boxCenter[constCoordinateIndex] + boxHalfSize, config->imageSize[constCoordinateIndex] - 1);
            if (fixedCoordinates[0] == fixedCoordinates[1])
            {
                StlUtilities::RemoveAt(&fixedCoordinates, 1);
            }

            for (size_t fixedCoordinateArrayIndex = 0; fixedCoordinateArrayIndex < fixedCoordinates.size(); ++fixedCoordinateArrayIndex)
            {
                int fixedCoordinate = fixedCoordinates[fixedCoordinateArrayIndex];

                for (int firstCoordinate = minFirstCoordinate; firstCoordinate <= maxFirstCoordinate; ++firstCoordinate)
                {
                    for (int secondCoordinate = minSecondCoordinate; secondCoordinate <= maxSecondCoordinate; ++secondCoordinate)
                    {
                        currentPosition[coordiateIndexes[0]] = firstCoordinate;
                        currentPosition[coordiateIndexes[1]] = secondCoordinate;
                        currentPosition[constCoordinateIndex] = fixedCoordinate;

                        bool isVoid = dataArrays.isSolidMask.GetPixel(currentPosition) == 0;
                        bool isNotProcessed = dataArrays.shortestPathDistances.GetPixel(currentPosition) == UNKNOWN_SHORTEST_PATH_DISTANCE;

                        // Pixel can be processed if we already started shortest path calculation but propagation did not reach all the box boundaries,
                        // so we restart it from another pixel
                        if (isVoid && isNotProcessed)
                        {
                            StlUtilities::Copy(currentPosition, position);
                            return true;
                        }
                    }
                }
            }
        }

        return false;
    }

    ShortestPathComputer::~ShortestPathComputer()
    {
    }
}

