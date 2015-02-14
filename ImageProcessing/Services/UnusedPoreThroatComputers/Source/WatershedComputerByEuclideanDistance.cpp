//// Copyright (c) 2013 Vasili Baranau
//// Distributed under the MIT software license
//// See the accompanying file License.txt or http://opensource.org/licenses/MIT
//
//#include "../Headers/WatershedComputerByEuclideanDistance.h"
//
//#include <string>
//#include <cmath>
//#include "stdio.h"
//#include <map>
//#include <stack>
//#include <list>
//#include <set>
//#include <queue>
//
//#include "Core/Headers/Path.h"
//#include "ImageProcessing/Model/Headers/Config.h"
//#include "ImageProcessing/Model/Headers/DataArray.h"
//#include "ImageProcessing/Services/Headers/ActiveAreaComputer.h"
//#include "ImageProcessing/Services/Headers/Serializer.h"
//#include "ImageProcessing/Services/Headers/InnerBallsRemover.h"
//#include "ImageProcessing/Services/Headers/PoreAndThroatComputer.h"
//#include "ImageProcessing/Services/Headers/ContainingBallsAssigner.h"
//
//using namespace std;
//using namespace Core;
//using namespace Model;
//using namespace Services;
//
//namespace Services
//{
//    const WatershedIterationType WatershedComputerByEuclideanDistance::UNKNOWN_ITERATION = numeric_limits<WatershedIterationType>::max();
//
//    WatershedComputerByEuclideanDistance::WatershedComputerByEuclideanDistance(const ActiveAreaComputer& currentActiveAreaComputer, const Serializer& currentSerializer) :
//            activeAreaComputer(currentActiveAreaComputer), serializer(currentSerializer)
//    {
//    }
//
//    void WatershedComputerByEuclideanDistance::AssignPoreAndThroatIds(const Config& config) const
//    {
//        this->config = &config;
//
//        firstSharedId = std::numeric_limits<PoreThroatIdType>::max();
//        sharedPixelTypesByIds.clear();
//        sharedPixelTypeIdsBySharedPores.clear();
//
//        IntermediateStatistics intermediateStatistics;
//        string intermediateStatisticsPath = Path::Append(config.baseFolder, INTERMEDIATE_STATISTICS_FILE_NAME);
//        serializer.ReadIntermediateStatistics(intermediateStatisticsPath, &intermediateStatistics);
//        FLOAT_TYPE marginInPixels = config.maxPoreRadiusToSeedRadiusRatio * sqrt(intermediateStatistics.maxEuclideanDistanceSquare);
//        discreteMargin = marginInPixels * 0.8;
//        Margin margin(Margin::Pixels, marginInPixels);
//
//        boost::array<size_t, 3> imageSize;
//        StlUtilities::Copy(config.imageSize, &imageSize);
//        size_t pixelsCount = VectorUtilities::GetProduct(imageSize);
//        FLOAT_TYPE maximumBallsFraction = static_cast<FLOAT_TYPE>(intermediateStatistics.maximumBallsCount) / pixelsCount;
//        FLOAT_TYPE averageBytesPerPixel = sizeof(IsMaximumBallMaskType) + sizeof(EuclideanDistanceSquareType) + sizeof(PoreThroatIdType) +
//                sizeof(WatershedIterationType) + maximumBallsFraction * sizeof(MaximumBall);
//
//        vector<Axis::Type> priorityAxes;
//        vector<ActiveArea> activeAreas;
//        activeAreaComputer.ComputeActiveAreas(config, priorityAxes, margin, averageBytesPerPixel, &activeAreas);
//
//        string maximumBallsMaskPath = Path::Append(config.baseFolder, IS_MAXIMUM_BALL_MASK_FOLDER_NAME);
//        DataArray<IsMaximumBallMaskType> maximumBallsMaskLocal(serializer, config, maximumBallsMaskPath, activeAreas);
//        maximumBallsMask = &maximumBallsMaskLocal;
//
//        string euclideanDistanceSquaresPath = Path::Append(config.baseFolder, EUCLIDEAN_DISTANCE_SQUARES_FOLDER_NAME);
//        DataArray<EuclideanDistanceSquareType> euclideanDistanceSquaresLocal(serializer, config, euclideanDistanceSquaresPath, activeAreas);
//        euclideanDistanceSquares = &euclideanDistanceSquaresLocal;
//
//        string poreThroatIdsWatershedPath = Path::Append(config.baseFolder, WATERSHED_BY_EUCLIDEAN_DISTANC_PORE_THROAT_IDS_FOLDER_NAME);
//        DataArray<PoreThroatIdType> poreThroatIdsLocal(serializer, config, poreThroatIdsWatershedPath, activeAreas);
//        poreThroatIdsLocal.defaultValue = PoreAndThroatComputer::UNKNOWN_INDEX;
//        poreThroatIds = &poreThroatIdsLocal;
//
//        string watershedIterationsPath = Path::Append(config.baseFolder, WATERSHED_BY_EUCLIDEAN_DISTANC_ITERATIONS_FOLDER_NAME);
//        DataArray<WatershedIterationType> watershedIterationsLocal(serializer, config, watershedIterationsPath, activeAreas);
//        watershedIterationsLocal.defaultValue = UNKNOWN_ITERATION;
//        localWatershedIterations = &watershedIterationsLocal;
//
//        AssignPoreIds(activeAreas);
//    }
//
//    void WatershedComputerByEuclideanDistance::AssignPoreIds(const vector<ActiveArea>& activeAreas) const
//    {
//        printf("Assigning pore indexes in the entire image...\n");
//        PoreThroatIdType startingPoreId = PoreAndThroatComputer::UNKNOWN_INDEX + 127; // add 127 to make even the first pore visible, if indexes are interpreted as CMYK colors (127 is a rather strong cyan)
//        for (size_t activeAreaIndex = 0; activeAreaIndex < activeAreas.size(); ++activeAreaIndex)
//        {
//            printf("Active area %d / %d...\n", activeAreaIndex + 1, activeAreas.size());
//            this->activeArea = &activeAreas[activeAreaIndex];
//
//            maximumBallsMask->ChangeActiveArea(activeAreaIndex);
//            euclideanDistanceSquares->ChangeActiveArea(activeAreaIndex);
//            poreThroatIds->ChangeActiveArea(activeAreaIndex);
//            localWatershedIterations->ChangeActiveArea(activeAreaIndex);
//
//            startingPoreId = AssignPoreIdsInActiveArea(startingPoreId);
//        }
//
//        poreThroatIds->WriteCurrentActiveArea();
//        localWatershedIterations->WriteCurrentActiveArea();
//
//        string poresFilePath = Path::Append(config->baseFolder, WATERSHED_BY_EUCLIDEAN_DISTANCE_PORES_FILE_NAME);
//        serializer.WritePores(poresFilePath, poresByIds);
//
//        string connectingBallsFilePath = Path::Append(config->baseFolder, WATERSHED_BY_EUCLIDEAN_DISTANCE_SHARED_PIXEL_TYPES_FILE_NAME);
//        serializer.WriteSharedPixelTypes(connectingBallsFilePath, sharedPixelTypesByIds);
//    }
//
//    PoreThroatIdType WatershedComputerByEuclideanDistance::AssignPoreIdsInActiveArea(PoreThroatIdType startingPoreId) const
//    {
//        // Sort maximum balls
//        printf("Filling and sorting maximum balls...\n");
//        vector<MaximumBall> maximumBalls;
//        FillDescendingMaximumBalls(&maximumBalls);
//
//        printf("Assigning pore indexes in active area...\n");
//        startingPoreId = AssignPoreIdsInActiveArea(maximumBalls, startingPoreId);
//        return startingPoreId;
//    }
//
//    void WatershedComputerByEuclideanDistance::FillDescendingMaximumBalls(vector<MaximumBall>* maximumBalls) const
//    {
////        EuclideanDistanceSquareType minPoreSeedRadiusSquare = static_cast<EuclideanDistanceSquareType>(config->minPoreSeedRadiusInPixels) * config->minPoreSeedRadiusInPixels;
////        for (int z = activeArea->activeBox.leftCorner[Core::Axis::Z]; z < activeArea->activeBox.exclusiveRightCorner[Core::Axis::Z]; ++z)
////        {
////            for (int x = activeArea->activeBox.leftCorner[Core::Axis::X]; x < activeArea->activeBox.exclusiveRightCorner[Core::Axis::X]; ++x)
////            {
////                for (int y = activeArea->activeBox.leftCorner[Core::Axis::Y]; y < activeArea->activeBox.exclusiveRightCorner[Core::Axis::Y]; ++y)
////                {
////                    IsMaximumBallMaskType maximumBallMask = maximumBallsMask->GetPixel(x, y, z);
////                    EuclideanDistanceSquareType ballRadiusSquare = euclideanDistanceSquares->GetPixel(x, y, z);
////
////                    if ((maximumBallMask != InnerBallsRemover::OUTER_BALL_MASK) || (ballRadiusSquare < minPoreSeedRadiusSquare))
////                    {
////                        continue;
////                    }
////
////                    MaximumBall maximumBall;
////                    maximumBall.x = x;
////                    maximumBall.y = y;
////                    maximumBall.z = z;
////                    maximumBall.ballRadiusSquare = ballRadiusSquare;
////                    maximumBalls->push_back(maximumBall);
////                }
////            }
////        }
//
//        EuclideanDistanceSquareType minPoreSeedRadiusSquare = static_cast<EuclideanDistanceSquareType>(config->minPoreSeedRadiusInPixels) * config->minPoreSeedRadiusInPixels;
//        for (int z = activeArea->activeBox.leftCorner[Core::Axis::Z]; z < activeArea->activeBox.exclusiveRightCorner[Core::Axis::Z]; ++z)
//        {
//            for (int x = activeArea->activeBox.leftCorner[Core::Axis::X]; x < activeArea->activeBox.exclusiveRightCorner[Core::Axis::X]; ++x)
//            {
//                for (int y = activeArea->activeBox.leftCorner[Core::Axis::Y]; y < activeArea->activeBox.exclusiveRightCorner[Core::Axis::Y]; ++y)
//                {
//                    EuclideanDistanceSquareType ballRadiusSquare = euclideanDistanceSquares->GetPixel(x, y, z);
//
//                    if (ballRadiusSquare == 0 || ballRadiusSquare < minPoreSeedRadiusSquare)
//                    {
//                        continue;
//                    }
//
//                    int minX = std::max(0, x - 1);
//                    int maxX = std::min(config->imageSize[Axis::X] - 1, x + 1);
//
//                    int minY = std::max(0, y - 1);
//                    int maxY = std::min(config->imageSize[Axis::Y] - 1, y + 1);
//
//                    int minZ = std::max(0, z - 1);
//                    int maxZ = std::min(config->imageSize[Axis::Z] - 1, z + 1);
//
//                    bool isLocalMaximum = true;
//                    for (int neighborX = minX; neighborX <= maxX; ++neighborX)
//                    {
//                        if (!isLocalMaximum)
//                        {
//                            break;
//                        }
//                        for (int neighborY = minY; neighborY <= maxY; ++neighborY)
//                        {
//                            if (!isLocalMaximum)
//                            {
//                                break;
//                            }
//                            for (int neighborZ = minZ; neighborZ <= maxZ; ++neighborZ)
//                            {
//                                EuclideanDistanceSquareType neighborRadiusSquare = euclideanDistanceSquares->GetPixel(neighborX, neighborY, neighborZ);
//                                if (neighborRadiusSquare > ballRadiusSquare)
//                                {
//                                    isLocalMaximum = false;
//                                    break;
//                                }
//                            }
//                        }
//                    }
//
//                    if (!isLocalMaximum)
//                    {
//                        continue;
//                    }
//
//                    MaximumBall maximumBall;
//                    maximumBall.x = x;
//                    maximumBall.y = y;
//                    maximumBall.z = z;
//                    maximumBall.ballRadiusSquare = ballRadiusSquare;
//                    maximumBalls->push_back(maximumBall);
//                }
//            }
//        }
//
//        MaximumBallComparer comparer;
//        StlUtilities::Sort(maximumBalls, comparer);
//    }
//
//    PoreThroatIdType WatershedComputerByEuclideanDistance::AssignPoreIdsInActiveArea(const vector<MaximumBall>& maximumBalls, PoreThroatIdType startingPoreId) const
//    {
//        numberOfAssignedPixels = 0;
//
//        PoreThroatIdType poreId = startingPoreId;
//        for (size_t i = 0; i < maximumBalls.size(); ++i)
//        {
//            const MaximumBall& maximumBall = maximumBalls[i];
//
//            PoreThroatIdType currentPoreId = poreThroatIds->GetPixel(maximumBall.x, maximumBall.y, maximumBall.z);
//            if (currentPoreId != PoreAndThroatComputer::UNKNOWN_INDEX)
//            {
//                continue;
//            }
//
//            printf("Assigning pore index %d for maximum ball %d / %d\n", poreId, i + 1, maximumBalls.size());
//
//            Pore& pore = poresByIds[poreId];
//            pore.seedPosition[Axis::X] = maximumBall.x;
//            pore.seedPosition[Axis::Y] = maximumBall.y;
//            pore.seedPosition[Axis::Z] = maximumBall.z;
//            pore.id = poreId;
//            pore.volume = 0;
//
//            // Pore indexes are saved as 32 bit file, which is interpreted as CMYK file (4 bytes of colors).
//            // If, e.g., poreId == 255 now (strong cyan) and we increment by 1, we get 1 in the magenta channel and 0 in the cyan channel.
//            // Thus, we ensure that at least cyan channel will be visible after increment.
//            PoreThroatIdType addition = (poreId % 255 == 0) ? 127 : 1;
//            poreId += addition;
//
//            AssignPoreIdRecursively(&pore);
//
//            RemoveEmptySharedPixelTypes();
//
//            printf("Number of assigned pixels is %d\n", numberOfAssignedPixels);
////            poreThroatIds->WriteCurrentActiveArea(); // TODO: remove in final code
//        }
//
//        return poreId;
//    }
//
//    // I could track empty throats every time when i decrease the volume of the shared pixels during recursion,
//    // but each pore may be comprised of several millions of pixels, while there always no more than several thousands of shared pixel types,
//    // so doing it here seems to be a better solution.
//    void WatershedComputerByEuclideanDistance::RemoveEmptySharedPixelTypes() const
//    {
//        vector<PoreThroatIdType> emptySharedPixelTypeIds;
//        for (map<PoreThroatIdType, SharedPixelType>::iterator it = sharedPixelTypesByIds.begin(); it != sharedPixelTypesByIds.end(); ++it)
//        {
//            const SharedPixelType& sharedPixelType = (*it).second;
//
//            if (sharedPixelType.volume == 0)
//            {
//                emptySharedPixelTypeIds.push_back(sharedPixelType.id);
//            }
//        }
//
//        for (size_t i = 0; i < emptySharedPixelTypeIds.size(); ++i)
//        {
//            PoreThroatIdType id = emptySharedPixelTypeIds[i];
//            const SharedPixelType& sharedPixelType = sharedPixelTypesByIds[id];
//
//            sharedPixelTypeIdsBySharedPores.erase(sharedPixelType.poreIds);
//            sharedPixelTypesByIds.erase(id);
//        }
//    }
//
//    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////     Propagate even after shared pixels, but ensure steepest descent.
//    void WatershedComputerByEuclideanDistance::AssignPoreIdRecursively(Pore* pore) const
//    {
//        EuclideanDistanceSquareType euclideanDistanceSquare = euclideanDistanceSquares->GetPixel(pore->seedPosition);
//        NeighborNode firstParams(pore->seedPosition[Axis::X], pore->seedPosition[Axis::Y], pore->seedPosition[Axis::Z]); // the first pore ball is by definition not shared, so we won't need parent at all
//        firstParams.depth = euclideanDistanceSquare;
//        firstParams.euclideanDistanceSquare = euclideanDistanceSquare;
//        firstParams.parentEuclideanDistanceSquare = euclideanDistanceSquare;
//        firstParams.localIteration = 0;
//        firstParams.globalIteration = 0;
//        firstParams.initialId = poreThroatIds->GetPixel(pore->seedPosition);
//
//        vector<queue<NeighborNode> > nodesToVisitByDepth(firstParams.depth + 1); // usually currentDepth is no more that ~1000
//        queue<NeighborNode>& nodesToVisit = nodesToVisitByDepth[firstParams.depth];
//        nodesToVisit.push(firstParams);
//
//        bool shouldContiueRecursion = UpdatePixelAndSharedBalls(firstParams, pore);
//
//        if (numberOfAssignedPixels % 10000 == 0)
//        {
//            printf("Number of assigned pixels is %d\n", numberOfAssignedPixels);
//        }
//
//        if (!shouldContiueRecursion)
//        {
//            return;
//        }
//
//        for (int depth = firstParams.depth; depth >= 0; --depth) // can't use unsigned depth, as operation "--depth" will produce max_int, when depth == 0, and the cycle will continue (with access violations)
//        {
//            queue<NeighborNode>& nodesToVisit = nodesToVisitByDepth[depth];
//
//            while (!nodesToVisit.empty())
//            {
//                // TODO: improve, use NodeToVisit& node = nodesToVisit.front(), call pop later, when node is not needed anymore
//                NeighborNode node = nodesToVisit.front();
//                nodesToVisit.pop();
//
//                int x = node.x;
//                int y = node.y;
//                int z = node.z;
//
//                int minX = std::max(0, x - 1);
//                int maxX = std::min(config->imageSize[Axis::X] - 1, x + 1);
//
//                int minY = std::max(0, y - 1);
//                int maxY = std::min(config->imageSize[Axis::Y] - 1, y + 1);
//
//                int minZ = std::max(0, z - 1);
//                int maxZ = std::min(config->imageSize[Axis::Z] - 1, z + 1);
//
//                vector<NeighborNode> validNeighbors;
//
//                bool hasUnsetNeighborWithLargerDepth = false;
//
//                // for the points that are in the cube for this pixel
//                for (int neighborX = minX; neighborX <= maxX; ++neighborX)
//                {
//                    if (hasUnsetNeighborWithLargerDepth)
//                    {
//                        break;
//                    }
//
//                    for (int neighborY = minY; neighborY <= maxY; ++neighborY)
//                    {
//                        if (hasUnsetNeighborWithLargerDepth)
//                        {
//                            break;
//                        }
//
//                        for (int neighborZ = minZ; neighborZ <= maxZ; ++neighborZ)
//                        {
//                            if (neighborX == x && neighborY == y && neighborZ == z)
//                            {
//                                continue;
//                            }
//
//                            EuclideanDistanceSquareType neighborBallRadiusSquare = euclideanDistanceSquares->GetPixel(neighborX, neighborY, neighborZ);
//                            bool neighborVoid = neighborBallRadiusSquare > 0;
//                            if (!neighborVoid)
//                            {
//                                continue;
//                            }
//
//                            bool canPropagate = neighborBallRadiusSquare <= node.euclideanDistanceSquare;
//                            PoreThroatIdType neighborId = poreThroatIds->GetPixel(neighborX, neighborY, neighborZ);
//
//                            // Encounter a neighbor that has a larger euclideanDistanceSquare
//                            if (!canPropagate)
//                            {
//                                bool neighborIsTrueSteepestDescent = neighborBallRadiusSquare > node.parentEuclideanDistanceSquare;
//                                if (neighborIsTrueSteepestDescent)
//                                {
//                                    // Check if it contains the current pore
//                                    bool neighborContainsCurrentPore = false;
//                                    if (neighborId == pore->id)
//                                    {
//                                        neighborContainsCurrentPore = true;
//                                    }
//                                    else if (neighborId >= firstSharedId)
//                                    {
//                                        SharedPixelType& sharedPixelType = sharedPixelTypesByIds[neighborId];
//                                        if (StlUtilities::Contains(sharedPixelType.poreIds, pore->id))
//                                        {
//                                            neighborContainsCurrentPore = true;
//                                        }
//                                    }
//
//                                    // If no, the current node does not belong to the pore under the steepest descent criterion
//                                    // (because the steepest descent neighbor does not belong to the pore. If it is reachable by the pore, it should have been already set,
//                                    // as far as we are using watershed breadth-first search).
//                                    if (!neighborContainsCurrentPore)
//                                    {
//                                        hasUnsetNeighborWithLargerDepth = true;
//                                        break;
//                                    }
//                                }
//                            }
//                            else
//                            {
//                                // Add the node to the validNeighbors
//                                NeighborNode neighbor(neighborX, neighborY, neighborZ);
//                                neighbor.euclideanDistanceSquare = neighborBallRadiusSquare;
//                                neighbor.depth = neighborBallRadiusSquare;
//                                neighbor.globalIteration = 0;
//                                neighbor.localIteration = 0;
//                                neighbor.initialId = neighborId;
//                                neighbor.parentEuclideanDistanceSquare = node.euclideanDistanceSquare;
//
//                                validNeighbors.push_back(neighbor);
//                            }
//                        }
//                    }
//                }
//
//                if (hasUnsetNeighborWithLargerDepth)
//                {
//                    // Reset the pixel to the original value
//                    poreThroatIds->SetPixel(x, y, z, node.initialId);
//                }
//                else
//                {
//                    for (size_t i = 0; i < validNeighbors.size(); ++i)
//                    {
//                        NeighborNode& neighbor = validNeighbors[i];
//
//                        bool shouldContiueRecursion = UpdatePixelAndSharedBalls(neighbor, pore);
//                        if (shouldContiueRecursion)
//                        {
//                            queue<NeighborNode>& neighbotNodesToVisit = nodesToVisitByDepth[neighbor.depth];
//                            neighbotNodesToVisit.push(neighbor);
//
//                            if (numberOfAssignedPixels % 10000 == 0)
//                            {
//                                printf("Number of assigned pixels is %d\n", numberOfAssignedPixels);
//                            }
//                        }
//                    }
//                }
//            }
//        }
//
//
//
//        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//        // second propagation to ensure that all the pixels are interior
//
//        localWatershedIterations->SetPixel(pore->seedPosition, 1);
//
//        queue<NeighborNode> nodesToVisitForBoundary;
//        nodesToVisitForBoundary.push(firstParams);
//
//        while (!nodesToVisitForBoundary.empty())
//        {
//            NeighborNode node = nodesToVisitForBoundary.front();
//            nodesToVisitForBoundary.pop();
//
//            int x = node.x;
//            int y = node.y;
//            int z = node.z;
//
//            int minX = std::max(0, x - 1);
//            int maxX = std::min(config->imageSize[Axis::X] - 1, x + 1);
//
//            int minY = std::max(0, y - 1);
//            int maxY = std::min(config->imageSize[Axis::Y] - 1, y + 1);
//
//            int minZ = std::max(0, z - 1);
//            int maxZ = std::min(config->imageSize[Axis::Z] - 1, z + 1);
//
//            // for the points that are in the cube for this pixel
//            for (int neighborX = minX; neighborX <= maxX; ++neighborX)
//            {
//                for (int neighborY = minY; neighborY <= maxY; ++neighborY)
//                {
//                    for (int neighborZ = minZ; neighborZ <= maxZ; ++neighborZ)
//                    {
//                        if (neighborX == x && neighborY == y && neighborZ == z)
//                        {
//                            continue;
//                        }
//
//                        EuclideanDistanceSquareType neighborBallRadiusSquare = euclideanDistanceSquares->GetPixel(neighborX, neighborY, neighborZ);
//                        bool neighborVoid = neighborBallRadiusSquare > 0;
//                        if (!neighborVoid)
//                        {
//                            continue;
//                        }
//
//                        PoreThroatIdType neighborId = poreThroatIds->GetPixel(neighborX, neighborY, neighborZ);
//                        if (neighborId == pore->id)
//                        {
//                            WatershedIterationType neighborIteration = localWatershedIterations->GetPixel(neighborX, neighborY, neighborZ);
//                            bool unprocessed = (neighborIteration == 0);
//                            if (unprocessed)
//                            {
//                                localWatershedIterations->SetPixel(neighborX, neighborY, neighborZ, 1);
//                                NeighborNode neighbor(neighborX, neighborY, neighborZ);
//                                nodesToVisitForBoundary.push(neighbor);
//                            }
//                        }
//                        else if (neighborId == PoreAndThroatComputer::UNKNOWN_INDEX)
//                        {
//                            continue; // this neighbor pixel will be set by another pore. And may be expanded, if it will not become shared.
//                        }
//                        else if (neighborId < firstSharedId) // another pore
//                        {
//                            vector<PoreThroatIdType> sharedPoreIds;
//                            sharedPoreIds.push_back(neighborId);
//                            sharedPoreIds.push_back(pore->id);
//
//                            // get or create shared pixel type
//                            SharedPixelType* newSharedPixelType = GetOrAddSharedPixelType(sharedPoreIds);
//
//                            // set the pixel
//                            poreThroatIds->SetPixel(x, y, z, newSharedPixelType->id);
//
//                            // increment this pixel's volume
//                            newSharedPixelType->volume++;
//
//                            // decrement the volume of the other entity
//                            Pore& currentPore = poresByIds[neighborId];
//                            currentPore.volume--;
//                            localWatershedIterations->SetPixel(neighborX, neighborY, neighborZ, 1);
//                        }
//                        else // shared pixel
//                        {
//                            SharedPixelType& sharedPixelType = sharedPixelTypesByIds[neighborId];
//                            if (StlUtilities::Contains(sharedPixelType.poreIds, pore->id))
//                            {
//                                continue;
//                            }
//
//                            vector<PoreThroatIdType> sharedPoreIds = sharedPixelType.poreIds;
//                            sharedPoreIds.push_back(pore->id);
//
//                            // get or create shared pixel type
//                            SharedPixelType* newSharedPixelType = GetOrAddSharedPixelType(sharedPoreIds);
//
//                            // set the pixel
//                            poreThroatIds->SetPixel(x, y, z, newSharedPixelType->id);
//
//                            // increment this pixel's volume
//                            newSharedPixelType->volume++;
//                            sharedPixelType.volume--;
//
//                            localWatershedIterations->SetPixel(neighborX, neighborY, neighborZ, 1);
//                        }
//                    }
//                }
//            }
//        }
//    }
//
////    ////////////////////////////////
//////     Initial watershed through containing ball radii
////    void WatershedComputerByEuclideanDistance::AssignPoreIdRecursively(Pore* pore) const
////    {
////        EuclideanDistanceSquareType euclideanDistanceSquare = euclideanDistanceSquares->GetPixel(pore->seedPosition);
////        NeighborNode firstParams(pore->seedPosition[Axis::X], pore->seedPosition[Axis::Y], pore->seedPosition[Axis::Z]); // the first pore ball is by definition not shared, so we won't need parent at all
////        firstParams.depth = euclideanDistanceSquare;
////        firstParams.euclideanDistanceSquare = euclideanDistanceSquare;
////        firstParams.localIteration = 0;
////        firstParams.globalIteration = 0;
////
////        vector<queue<NeighborNode> > nodesToVisitByDepth(firstParams.depth + 1); // usually currentDepth is no more that ~1000
////        queue<NeighborNode>& nodesToVisit = nodesToVisitByDepth[firstParams.depth];
////        nodesToVisit.push(firstParams);
////
////        bool shouldContiueRecursion = UpdatePixelAndSharedBalls(firstParams, pore);
////
////        if (numberOfAssignedPixels % 10000 == 0)
////        {
////            printf("Number of assigned pixels is %d\n", numberOfAssignedPixels);
////        }
////
////        if (!shouldContiueRecursion)
////        {
////            return;
////        }
////
////        for (int depth = firstParams.depth; depth >= 0; --depth) // can't use unsigned depth, as operation "--depth" will produce max_int, when depth == 0, and the cycle will continue (with access violations)
////        {
////            queue<NeighborNode>& nodesToVisit = nodesToVisitByDepth[depth];
////
////            while (!nodesToVisit.empty())
////            {
////                // TODO: improve, use NodeToVisit& node = nodesToVisit.front(), call pop later, when node is not needed anymore
////                NeighborNode node = nodesToVisit.front();
////                nodesToVisit.pop();
////
////                int x = node.x;
////                int y = node.y;
////                int z = node.z;
////
////                int minX = std::max(0, x - 1);
////                int maxX = std::min(config->imageSize[Axis::X] - 1, x + 1);
////
////                int minY = std::max(0, y - 1);
////                int maxY = std::min(config->imageSize[Axis::Y] - 1, y + 1);
////
////                int minZ = std::max(0, z - 1);
////                int maxZ = std::min(config->imageSize[Axis::Z] - 1, z + 1);
////
////                // for the points that are in the cube for this pixel
////                for (int neighborX = minX; neighborX <= maxX; ++neighborX)
////                {
////                    for (int neighborY = minY; neighborY <= maxY; ++neighborY)
////                    {
////                        for (int neighborZ = minZ; neighborZ <= maxZ; ++neighborZ)
////                        {
////                            if (neighborX == x && neighborY == y && neighborZ == z)
////                            {
////                                continue;
////                            }
////
////                            EuclideanDistanceSquareType neighborBallRadiusSquare = euclideanDistanceSquares->GetPixel(neighborX, neighborY, neighborZ);
////                            bool neighborVoid = neighborBallRadiusSquare > 0;
////                            bool canPropagate = neighborVoid && (neighborBallRadiusSquare <= node.euclideanDistanceSquare);
////
////                            if (canPropagate)
////                            {
////                                NeighborNode neighbor(neighborX, neighborY, neighborZ);
////                                neighbor.euclideanDistanceSquare = neighborBallRadiusSquare;
////                                neighbor.depth = neighborBallRadiusSquare;
////                                neighbor.globalIteration = node.globalIteration + 1;
////    //                            neighbor.localIteration = (neighbor.depth < depth) ? 0 : node.localIteration + 1;
////                                neighbor.localIteration = neighbor.globalIteration;
////
////                                bool shouldContiueRecursion = UpdatePixelAndSharedBalls(neighbor, pore);
////                                shouldContiueRecursion &= (neighbor.globalIteration < discreteMargin);
////
////                                if (shouldContiueRecursion)
////                                {
////                                    queue<NeighborNode>& neighbotNodesToVisit = nodesToVisitByDepth[neighbor.depth];
////                                    neighbotNodesToVisit.push(neighbor);
////
////                                    if (numberOfAssignedPixels % 10000 == 0)
////                                    {
////                                        printf("Number of assigned pixels is %d\n", numberOfAssignedPixels);
////                                    }
////                                }
////                            }
////                            else
////                            {
////
////                            }
////                        }
////                    }
////                }
////            }
////        }
////    }
//
//    // Returns "shall continue recursion"
//    bool WatershedComputerByEuclideanDistance::UpdatePixelAndSharedBalls(const NeighborNode& node, Pore* pore) const
//    {
//        int x = node.x;
//        int y = node.y;
//        int z = node.z;
//        PoreThroatIdType currentId = poreThroatIds->GetPixel(x, y, z);
//
//        // Pixel is empty
//        if (currentId == PoreAndThroatComputer::UNKNOWN_INDEX) // unknown index
//        {
//            poreThroatIds->SetPixel(x, y, z, pore->id);
//            localWatershedIterations->SetPixel(x, y, z, node.localIteration);
//            pore->volume++;
//            numberOfAssignedPixels++;
//            return true;
//        }
//
//        ////////////////////////////
//        // Pixel is occupied
//
//        SharedPixelType* sharedPixelType = NULL;
//        Pore* currentPore = NULL;
//
//        if (currentId < firstSharedId)
//        {
//            currentPore = &poresByIds[currentId];
//        }
//        else
//        {
//            sharedPixelType = &sharedPixelTypesByIds[currentId];
//        }
//
//        // Pixel contains the current pore -> do nothing, shall stop recursion
//        if (currentId == pore->id)
//        {
//            return false;
//        }
//        else if (currentId >= firstSharedId)
//        {
//            if (StlUtilities::Contains(sharedPixelType->poreIds, pore->id))
//            {
//                return false;
//            }
//        }
//
////        WatershedIterationType currentLocalWatershedIteration = localWatershedIterations->GetPixel(x, y, z);
//
////        if (currentDepth != node->depth)
////        {
////            // If pixel height is selected as containing ball radius, there shall always be at least one path from a pore to the pixel, so that watershed height is equal to the pixel containing ball radius,
////            // and watershed shall go through one of these paths at first and set the pixel. Thus, two pores, that share the pixel, shall reach it at equal heights.
////            throw InvalidOperationException("Pixel is reached at different watershed heights from different pores");
////        }
//
////        // Have not reached the watershed, can just rewrite the pixel
////        if (node.localIteration < currentLocalWatershedIteration)
////        {
////            poreThroatIds->SetPixel(x, y, z, pore->id);
////            localWatershedIterations->SetPixel(x, y, z, node.localIteration);
////
////            pore->volume++;
////
////            // decrement the volume of the other entity
////            if (currentId < firstSharedId)
////            {
////                currentPore->volume--;
////            }
////            else
////            {
////                sharedPixelType->volume--;
////            }
////
////            return true;
////        }
////
//////        bool isWatershed = localWatershedIteration == currentLocalWatershedIteration;
////        bool isWatershed = node.localIteration >= currentLocalWatershedIteration;
//
//        bool isWatershed = true;
//        if (isWatershed)
//        {
//            vector<PoreThroatIdType> sharedPoreIds;
//            if (currentId < firstSharedId)
//            {
//                sharedPoreIds.push_back(currentId);
//            }
//            else
//            {
//                sharedPoreIds = sharedPixelType->poreIds;
//            }
//
//            sharedPoreIds.push_back(pore->id);
//
//            // get or create shared pixel type
//            SharedPixelType* newSharedPixelType = GetOrAddSharedPixelType(sharedPoreIds);
//
//            // set the pixel
//            poreThroatIds->SetPixel(x, y, z, newSharedPixelType->id);
//
//            // leave the old local iteration in the pixel, so that the pixel contains the minimum local iteration of all the pores that it shares
////            localWatershedIterations->SetPixel(x, y, z, localWatershedIteration); // set the iteration number for the last pore in the shared list
//
//            // increment this pixel's volume
//            newSharedPixelType->volume++;
//
//            // decrement the volume of the other entity
//            if (currentId < firstSharedId)
//            {
//                currentPore->volume--;
//            }
//            else
//            {
//                sharedPixelType->volume--;
//            }
//
////            return false;
//            return true;
//        }
//
////        if (localWatershedIteration > currentLocalWatershedIteration)
////        {
////            // There shall be no path to the pixel from a pore, where there is no watershed pixel,
////            // so that the local iteration achieved at this pixel is larger than the local iteration set.
////            throw InvalidOperationException("Pixel is reached at larger local iteration, than is set in the pixel");
////        }
//
//        return false;
//    }
//
//    SharedPixelType* WatershedComputerByEuclideanDistance::GetOrAddSharedPixelType(const vector<PoreThroatIdType>& sharedPoreIds) const
//    {
//        map<vector<PoreThroatIdType>, PoreThroatIdType>::iterator it = sharedPixelTypeIdsBySharedPores.find(sharedPoreIds);
//        if (it != sharedPixelTypeIdsBySharedPores.end())
//        {
//            PoreThroatIdType id = (*it).second;
//            return &sharedPixelTypesByIds[id];
//        }
//        else
//        {
//            firstSharedId--;
//            SharedPixelType& sharedPixelType = sharedPixelTypesByIds[firstSharedId];
//
//            sharedPixelType.id = firstSharedId;
//            sharedPixelType.volume = 0;
//            sharedPixelType.poreIds = sharedPoreIds;
//
//            sharedPixelTypeIdsBySharedPores[sharedPoreIds] = firstSharedId;
//
//            return &sharedPixelType;
//        }
//    }
//
//    WatershedComputerByEuclideanDistance::~WatershedComputerByEuclideanDistance()
//    {
//    }
//}
