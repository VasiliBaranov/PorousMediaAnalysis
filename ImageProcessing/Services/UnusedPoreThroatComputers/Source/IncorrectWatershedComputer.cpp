//// Copyright (c) 2013 Vasili Baranau
//// Distributed under the MIT software license
//// See the accompanying file License.txt or http://opensource.org/licenses/MIT
//
//#include "../Headers/WatershedComputer.h"
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
//    const WatershedIterationType WatershedComputer::UNKNOWN_ITERATION = numeric_limits<WatershedIterationType>::max();
//
//    WatershedComputer::WatershedComputer(const ActiveAreaComputer& currentActiveAreaComputer, const Serializer& currentSerializer) :
//            activeAreaComputer(currentActiveAreaComputer), serializer(currentSerializer)
//    {
//    }
//
//    void WatershedComputer::AssignPoreAndThroatIds(const Config& config) const
//    {
//        this->config = &config;
//
//        firstSharedId = std::numeric_limits<PoreThroatIdType>::max();
//        sharedPixelTypesByIds.clear();
//        oldSharedPixelTypesByIds.clear();
//        sharedPixelTypeIdsBySharedPores.clear();
//        numberOfAssignedPores = 0;
//
//        FillPoresByIds();
//
//        vector<ActiveArea> activeAreas;
//        IntermediateStatistics intermediateStatistics;
//        string intermediateStatisticsPath = Path::Append(config.baseFolder, INTERMEDIATE_STATISTICS_FILE_NAME);
//        serializer.ReadIntermediateStatistics(intermediateStatisticsPath, &intermediateStatistics);
//        FLOAT_TYPE marginInPixels = config.maxPoreRadiusToSeedRadiusRatio * sqrt(intermediateStatistics.maxEuclideanDistanceSquare);
//        Margin margin(Margin::Pixels, marginInPixels);
//        discreteMargin = marginInPixels * 0.8;
//
////        FLOAT_TYPE averageBytesPerPixel = sizeof(EuclideanDistanceSquareType) + sizeof(PoreThroatIdType) + sizeof(PoreThroatIdType) + sizeof(WatershedIterationType);
//        FLOAT_TYPE averageBytesPerPixel = sizeof(PoreThroatIdType) + sizeof(PoreThroatIdType) + sizeof(WatershedIterationType);
//
//        vector<Axis::Type> priorityAxes;
//        activeAreaComputer.ComputeActiveAreas(config, priorityAxes, margin, averageBytesPerPixel, &activeAreas);
//
////        string containingBallRadiiSquaresPath = Path::Append(config.baseFolder, CONTAINING_BALL_RADII_SQUARES_FOLDER_NAME);
////        DataArray<EuclideanDistanceSquareType> containingBallRadiiSquaresLocal(serializer, config, containingBallRadiiSquaresPath, activeAreas);
////        containingBallRadiiSquares = &containingBallRadiiSquaresLocal;
//
//        string oldPoreThroatIdsPath = Path::Append(config.baseFolder, PORE_THROAT_IDS_FOLDER_NAME);
//        DataArray<PoreThroatIdType> oldPoreThroatIdsLocal(serializer, config, oldPoreThroatIdsPath, activeAreas);
//        oldPoreThroatIdsLocal.defaultValue = PoreAndThroatComputer::UNKNOWN_INDEX;
//        oldPoreThroatIds = &oldPoreThroatIdsLocal;
//
//        string poreThroatIdsWatershedPath = Path::Append(config.baseFolder, WATERSHED_PORE_THROAT_IDS_FOLDER_NAME);
//        DataArray<PoreThroatIdType> poreThroatIdsLocal(serializer, config, poreThroatIdsWatershedPath, activeAreas);
//        poreThroatIdsLocal.defaultValue = PoreAndThroatComputer::UNKNOWN_INDEX;
//        poreThroatIds = &poreThroatIdsLocal;
//
//        string watershedIterationsPath = Path::Append(config.baseFolder, WATERSHED_ITERATIONS_FOLDER_NAME);
//        DataArray<WatershedIterationType> watershedIterationsLocal(serializer, config, watershedIterationsPath, activeAreas);
//        watershedIterationsLocal.defaultValue = UNKNOWN_ITERATION;
//        localWatershedIterations = &watershedIterationsLocal;
//
//        AssignPoreIds(activeAreas);
//    }
//
//    void WatershedComputer::FillPoresByIds() const
//    {
//        string connectingBallsFilePath = Path::Append(config->baseFolder, SHARED_PIXEL_TYPES_FILE_NAME);
//        serializer.ReadSharedPixelTypes(connectingBallsFilePath, &oldSharedPixelTypesByIds);
//
//        string poresFilePath = Path::Append(config->baseFolder, PORES_FILE_NAME);
//        map<PoreThroatIdType, Pore> simplePoresByIds;
//        serializer.ReadPores(poresFilePath, &simplePoresByIds);
//
//        poresByIds.clear();
//
//        lastPoreId = 0;
//        for (map<PoreThroatIdType, Pore>::iterator it = simplePoresByIds.begin(); it != simplePoresByIds.end(); ++it)
//        {
//            Pore& pore = (*it).second;
//
//            PoreWithRadius poreWithRadius;
//            poreWithRadius.id = pore.id;
//            poreWithRadius.seedPosition = pore.seedPosition;
//            poreWithRadius.volume = 0; // will fill during other methods
//            poreWithRadius.seedRadius = 0; // will fill right now
//
//            poresByIds[pore.id] = poreWithRadius;
//
//            if (pore.id > lastPoreId)
//            {
//                lastPoreId = pore.id;
//            }
//        }
//
//        ///////////////////
//        // Reading pore radii
//        Margin margin(Margin::Pixels, 0);
//        FLOAT_TYPE averageBytesPerPixel = sizeof(EuclideanDistanceSquareType);
//        vector<Axis::Type> priorityAxes(2);
//        priorityAxes[0] = Axis::X;
//        priorityAxes[1] = Axis::Y;
//        vector<ActiveArea> activeAreas;
//        activeAreaComputer.ComputeActiveAreas(*config, priorityAxes, margin, averageBytesPerPixel, &activeAreas);
//
//        string euclideanDistanceSquaresSquaresPath = Path::Append(config->baseFolder, EUCLIDEAN_DISTANCE_SQUARES_FOLDER_NAME);
//        DataArray<EuclideanDistanceSquareType> euclideanDistanceSquares(serializer, *config, euclideanDistanceSquaresSquaresPath, activeAreas);
//
//        for (size_t activeAreaIndex = 0; activeAreaIndex < activeAreas.size(); ++activeAreaIndex)
//        {
//            euclideanDistanceSquares.ChangeActiveArea(activeAreaIndex);
//            for (map<PoreThroatIdType, PoreWithRadius>::iterator it = poresByIds.begin(); it != poresByIds.end(); ++it)
//            {
//                PoreWithRadius& pore = (*it).second;
//
//                if (activeAreas[activeAreaIndex].boxWithMargins.Contains(pore.seedPosition))
//                {
//                    pore.seedRadius = euclideanDistanceSquares.GetPixel(pore.seedPosition);
//                }
//            }
//        }
//    }
//
//    void WatershedComputer::AssignPoreIds(const vector<ActiveArea>& activeAreas) const
//    {
//        printf("Assigning pore indexes in the entire image...\n");
//        for (size_t activeAreaIndex = 0; activeAreaIndex < activeAreas.size(); ++activeAreaIndex)
//        {
//            printf("Active area %d / %d...\n", activeAreaIndex + 1, activeAreas.size());
//            this->activeArea = &activeAreas[activeAreaIndex];
//
////            containingBallRadiiSquares->ChangeActiveArea(activeAreaIndex);
//            oldPoreThroatIds->ChangeActiveArea(activeAreaIndex);
//            poreThroatIds->ChangeActiveArea(activeAreaIndex);
//            localWatershedIterations->ChangeActiveArea(activeAreaIndex);
//
//            AssignPoreIdsInActiveArea();
//        }
//
//        poreThroatIds->WriteCurrentActiveArea();
//        localWatershedIterations->WriteCurrentActiveArea();
//
//        // Write pores to a file
//        map<PoreThroatIdType, Pore> simplePoresByIds;
//        for (map<PoreThroatIdType, PoreWithRadius>::iterator it = poresByIds.begin(); it != poresByIds.end(); ++it)
//        {
//            PoreWithRadius& poreWithRadius = (*it).second;
//
//            Pore pore;
//            pore.id = poreWithRadius.id;
//            pore.seedPosition = poreWithRadius.seedPosition;
//            pore.volume = poreWithRadius.volume; // will fill during other methods
//            simplePoresByIds[pore.id] = pore;
//        }
//        string poresFilePath = Path::Append(config->baseFolder, WATERSHED_PORES_FILE_NAME);
//        serializer.WritePores(poresFilePath, simplePoresByIds);
//
//        string connectingBallsFilePath = Path::Append(config->baseFolder, WATERSHED_SHARED_PIXEL_TYPES_FILE_NAME);
//        serializer.WriteSharedPixelTypes(connectingBallsFilePath, sharedPixelTypesByIds);
//    }
//
//    void WatershedComputer::AssignPoreIdsInActiveArea() const
//    {
//        printf("Assigning pore indexes in active area...\n");
//        numberOfAssignedPixels = 0;
//
//        for (map<PoreThroatIdType, PoreWithRadius>::iterator it = poresByIds.begin(); it != poresByIds.end(); ++it)
//        {
//            PoreWithRadius& pore = (*it).second;
//            if (activeArea->activeBox.Contains(pore.seedPosition))
//            {
//                printf("Assigning pore index %d / %d\n", numberOfAssignedPores, poresByIds.size());
//
//                AssignPoreIdRecursively(&pore);
//
//                RemoveEmptySharedPixelTypes();
//
//                printf("Number of assigned pixels is %d\n", numberOfAssignedPixels);
//                numberOfAssignedPores++;
//            }
//        }
//    }
//
//    // I could track empty throats every time when i decrease the volume of the shared pixels during recursion,
//    // but each pore may be comprised of several millions of pixels, while there always no more than several thousands of shared pixel types,
//    // so doing it here seems to be a better solution.
//    void WatershedComputer::RemoveEmptySharedPixelTypes() const
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
////    ///////////////////////////
////    // Propagate through shared pixels without restriction on containing ball radii
////    void WatershedComputer::AssignPoreIdRecursively(PoreWithRadius* pore) const
////    {
////        NeighborNode firstParams(pore->seedPosition[Axis::X], pore->seedPosition[Axis::Y], pore->seedPosition[Axis::Z]); // the first pore ball is by definition not shared, so we won't need parent at all
////        firstParams.depth = 1;
////        firstParams.localIteration = 0;
////        firstParams.globalIteration = 0;
////
////        vector<queue<NeighborNode> > nodesToVisitByDepth(firstParams.depth + 1);
////        queue<NeighborNode>& nodesToVisit = nodesToVisitByDepth[firstParams.depth];
////        nodesToVisit.push(firstParams);
////
////        bool shouldContiueRecursion = UpdatePixelAndSharedBalls(firstParams.localIteration, firstParams, pore);
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
//////                            EuclideanDistanceSquareType neighborBallRadiusSquare = containingBallRadiiSquares->GetPixel(neighborX, neighborY, neighborZ);
//////                            bool neighborVoid = neighborBallRadiusSquare > 0;
//////                            if (!neighborVoid || neighborBallRadiusSquare > node.containingBallRadiusSquare)
//////                            {
//////                                continue;
//////                            }
////                            PoreThroatIdType neighborOldId = oldPoreThroatIds->GetPixel(neighborX, neighborY, neighborZ);
////                            bool neighborVoid = neighborOldId != PoreAndThroatComputer::UNKNOWN_INDEX;
////                            if (!neighborVoid)
////                            {
////                                continue;
////                            }
////
////                            bool isSharedPixel = neighborOldId > lastPoreId;
////
////                            // node is shared, neighbor is not shared
////                            if (node.depth == 0 && !isSharedPixel)
////                            {
////                                continue;
////                            }
////
////                            NeighborNode neighbor(neighborX, neighborY, neighborZ);
////                            neighbor.depth = isSharedPixel ? 0 : 1;
////                            neighbor.localIteration = (neighbor.depth < depth) ? 0 : node.localIteration + 1;
////                            neighbor.globalIteration = node.globalIteration + 1;
////
////                            bool shouldContiueRecursion = UpdatePixelAndSharedBalls(neighbor.localIteration, neighbor, pore);
////                            shouldContiueRecursion &= (neighbor.globalIteration < discreteMargin);
////
////                            if (shouldContiueRecursion)
////                            {
////                                queue<NeighborNode>& neighbotNodesToVisit = nodesToVisitByDepth[neighbor.depth];
////                                neighbotNodesToVisit.push(neighbor);
////
////                                if (numberOfAssignedPixels % 10000 == 0)
////                                {
////                                    printf("Number of assigned pixels is %d\n", numberOfAssignedPixels);
////                                }
////                            }
////                        }
////                    }
////                }
////            }
////        }
////    }
//
//    ////////////////////////////////////
//    // Have just two (or even one) watershed levels: pure and shared
//    void WatershedComputer::AssignPoreIdRecursively(PoreWithRadius* pore) const
//    {
//        map<PoreThroatIdType, SharedPixelType>::iterator it = oldSharedPixelTypesByIds.begin();
//        PoreThroatIdType minSharedPixelTypeId = (*it).first;
//
//        it = oldSharedPixelTypesByIds.end();
//        it--;
//        PoreThroatIdType maxSharedPixelTypeId = (*it).first;
//
//        vector<bool> oldSharedPixelContainsPore(maxSharedPixelTypeId - minSharedPixelTypeId + 1, false);
//        for (it = oldSharedPixelTypesByIds.begin(); it != oldSharedPixelTypesByIds.end(); ++it)
//        {
//            SharedPixelType& sharedPixelType = (*it).second;
//            if (StlUtilities::Contains(sharedPixelType.poreIds, pore->id)) // pixel is shared, but not with the current pore
//            {
//                oldSharedPixelContainsPore[sharedPixelType.id - minSharedPixelTypeId] = true;
//            }
//        }
//
////        EuclideanDistanceSquareType containingBallRadiiSquare = containingBallRadiiSquares->GetPixel(pore->seedPosition[Axis::X], pore->seedPosition[Axis::Y], pore->seedPosition[Axis::Z]);
//        NeighborNode firstParams(pore->seedPosition[Axis::X], pore->seedPosition[Axis::Y], pore->seedPosition[Axis::Z]); // the first pore ball is by definition not shared, so we won't need parent at all
////        firstParams.containingBallRadiusSquare = containingBallRadiiSquare;
//        firstParams.depth = 1;
//        firstParams.localIteration = 0;
//
//        vector<queue<NeighborNode> > nodesToVisitByDepth(firstParams.depth + 1);
//        queue<NeighborNode>& nodesToVisit = nodesToVisitByDepth[firstParams.depth];
//        nodesToVisit.push(firstParams);
//
//        bool shouldContiueRecursion = UpdatePixelAndSharedBalls(&firstParams, pore);
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
//                // for the points that are in the cube for this pixel
//                for (int neighborX = minX; neighborX <= maxX; ++neighborX)
//                {
//                    for (int neighborY = minY; neighborY <= maxY; ++neighborY)
//                    {
//                        for (int neighborZ = minZ; neighborZ <= maxZ; ++neighborZ)
//                        {
//                            if (neighborX == x && neighborY == y && neighborZ == z)
//                            {
//                                continue;
//                            }
//
////                            EuclideanDistanceSquareType neighborBallRadiusSquare = containingBallRadiiSquares->GetPixel(neighborX, neighborY, neighborZ);
////                            bool neighborVoid = neighborBallRadiusSquare > 0;
////                            if (!neighborVoid || neighborBallRadiusSquare > node.containingBallRadiusSquare)
////                            {
////                                continue;
////                            }
//
//                            PoreThroatIdType oldNeighborId = oldPoreThroatIds->GetPixel(neighborX, neighborY, neighborZ);
//                            if (oldNeighborId == PoreAndThroatComputer::UNKNOWN_INDEX) // solid
//                            {
//                                continue;
//                            }
////                            if (oldNeighborId != pore->id && oldNeighborId <= lastPoreId) // another pore
////                            {
////                                continue;
////                            }
////                            if (oldNeighborId != pore->id) // shared pixel
////                            {
//////                                SharedPixelType& sharedPixel = oldSharedPixelTypesByIds[oldNeighborId];
//////                                if (!StlUtilities::Exists(sharedPixel.poreIds, pore->id)) // pixel is shared, but not with the current pore
//////                                {
//////                                    continue;
//////                                }
////
////                                if (!oldSharedPixelContainsPore[oldNeighborId - minSharedPixelTypeId]) // pixel is shared, but not with the current pore
////                                {
////                                    continue;
////                                }
////                            }
//
//                            bool canPropagate = true;
//                            if (oldNeighborId != pore->id && oldNeighborId <= lastPoreId) // another pore
//                            {
//                                canPropagate = false;
//                            }
//                            if (oldNeighborId != pore->id) // shared pixel
//                            {
//                                if (!oldSharedPixelContainsPore[oldNeighborId - minSharedPixelTypeId]) // pixel is shared, but not with the current pore
//                                {
//                                    canPropagate = false;
//                                }
//                            }
//
//                            if (canPropagate)
//                            {
//                                bool wasSharedPixel = oldNeighborId != pore->id;
//
//                                NeighborNode neighbor(neighborX, neighborY, neighborZ);
//    //                            neighbor.containingBallRadiusSquare = neighborBallRadiusSquare;
//                                neighbor.depth = wasSharedPixel ? 0 : 1;
//                                neighbor.localIteration = (neighbor.depth < depth) ? 0 : node.localIteration + 1;
//
//                                bool shouldContiueRecursion = UpdatePixelAndSharedBalls(&neighbor, pore);
//
//                                if (shouldContiueRecursion)
//                                {
//                                    queue<NeighborNode>& neighbotNodesToVisit = nodesToVisitByDepth[neighbor.depth];
//                                    neighbotNodesToVisit.push(neighbor);
//
//                                    if (numberOfAssignedPixels % 10000 == 0)
//                                    {
//                                        printf("Number of assigned pixels is %d\n", numberOfAssignedPixels);
//                                    }
//                                }
//                            }
//                            else
//                            {
//                                // Can not continue to the next neighbor. But pore pixels shall always be surrounded either with solid pixels,
//                                // or with current pore pixels, or with pixels shared with the current pore.
//                                if (node.finalId == pore->id)
//                                {
//                                    NeighborNode neighbor(neighborX, neighborY, neighborZ);
//                                    neighbor.depth = 1;
//                                    neighbor.localIteration = numeric_limits<WatershedIterationType>::max();
//
//                                    UpdatePixelAndSharedBalls(&neighbor, pore);
//                                }
//                            }
//                        }
//                    }
//                }
//            }
//        }
//    }
//
//    //////////////////////////////////
//    // Initial watershed through containing ball radii
////    void WatershedComputer::AssignPoreIdRecursively(PoreWithRadius* pore) const
////    {
////        EuclideanDistanceSquareType maxDepth = containingBallRadiiSquares->GetPixel(pore->seedPosition[Axis::X], pore->seedPosition[Axis::Y], pore->seedPosition[Axis::Z]);
////        vector<queue<NeighborNode> > nodesToVisitByDepth(maxDepth + 1); // usually currentDepth is no more that ~1000
////
////        NeighborNode firstParams(pore->seedPosition[Axis::X], pore->seedPosition[Axis::Y], pore->seedPosition[Axis::Z]); // the first pore ball is by definition not shared, so we won't need parent at all
////        firstParams.depth = maxDepth;
////        firstParams.depth = maxDepth;
////        firstParams.localIteration = 0;
////        queue<NeighborNode>& nodesToVisit = nodesToVisitByDepth[maxDepth];
////        nodesToVisit.push(firstParams);
////
////        bool shouldContiueRecursion = UpdatePixelAndSharedBalls(firstParams.localIteration, firstParams, pore);
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
////        for (int depth = maxDepth; depth >= 0; --depth) // can't use unsigned depth, as operation "--depth" will produce max_int, when depth == 0, and the cycle will continue (with access violations)
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
////                            EuclideanDistanceSquareType neighborBallRadiusSquare = containingBallRadiiSquares->GetPixel(neighborX, neighborY, neighborZ);
////                            bool neighborVoid = neighborBallRadiusSquare > 0;
////                            if (!neighborVoid || neighborBallRadiusSquare > node.depth)
////                            {
////                                continue;
////                            }
////
////                            NeighborNode neighbor(neighborX, neighborY, neighborZ);
////                            neighbor.depth = neighborBallRadiusSquare;
////                            neighbor.localIteration = (neighborBallRadiusSquare < depth) ? 0 : node.localIteration + 1;
////
////                            bool shouldContiueRecursion = UpdatePixelAndSharedBalls(neighbor.localIteration, neighbor, pore);
////
////                            if (shouldContiueRecursion)
////                            {
////                                queue<NeighborNode>& neighbotNodesToVisit = nodesToVisitByDepth[neighborBallRadiusSquare];
////                                neighbotNodesToVisit.push(neighbor);
////
////                                if (numberOfAssignedPixels % 10000 == 0)
////                                {
////                                    printf("Number of assigned pixels is %d\n", numberOfAssignedPixels);
////                                }
////                            }
////                        }
////                    }
////                }
////            }
////        }
////    }
//
//    // Returns "shall continue recursion"
//    bool WatershedComputer::UpdatePixelAndSharedBalls(NeighborNode* node, Pore* pore) const
//    {
//        int x = node->x;
//        int y = node->y;
//        int z = node->z;
//        PoreThroatIdType currentId = poreThroatIds->GetPixel(x, y, z);
//
//        // Pixel is empty
//        if (currentId == PoreAndThroatComputer::UNKNOWN_INDEX) // unknown index
//        {
//            poreThroatIds->SetPixel(x, y, z, pore->id);
//            localWatershedIterations->SetPixel(x, y, z, node->localIteration);
//            node->finalId = pore->id;
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
//        if (currentId <= lastPoreId)
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
//        else if (currentId > lastPoreId)
//        {
//            if (StlUtilities::Contains(sharedPixelType->poreIds, pore->id))
//            {
//                return false;
//            }
//        }
//
//        WatershedIterationType currentLocalWatershedIteration = localWatershedIterations->GetPixel(x, y, z);
//
////        if (currentDepth != node->depth)
////        {
////            // If pixel height is selected as containing ball radius, there shall always be at least one path from a pore to the pixel, so that watershed height is equal to the pixel containing ball radius,
////            // and watershed shall go through one of these paths at first and set the pixel. Thus, two pores, that share the pixel, shall reach it at equal heights.
////            throw InvalidOperationException("Pixel is reached at different watershed heights from different pores");
////        }
//
//        // Have not reached the watershed, can just rewrite the pixel
//        if (node->localIteration < currentLocalWatershedIteration)
//        {
//            poreThroatIds->SetPixel(x, y, z, pore->id);
//            node->finalId = pore->id;
//            localWatershedIterations->SetPixel(x, y, z, node->localIteration);
//
//            pore->volume++;
//
//            // decrement the volume of the other entity
//            if (currentId <= lastPoreId)
//            {
//                currentPore->volume--;
//            }
//            else
//            {
//                sharedPixelType->volume--;
//            }
//
//            return true;
//        }
//
////        bool isWatershed = localWatershedIteration == currentLocalWatershedIteration;
//        bool isWatershed = node->localIteration >= currentLocalWatershedIteration;
//        if (isWatershed)
//        {
//            vector<PoreThroatIdType> sharedPoreIds;
//            if (currentId <= lastPoreId)
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
//            node->finalId = newSharedPixelType->id;
//
//            // leave the old local iteration in the pixel, so that the pixel contains the minimum local iteration of all the pores that it shares
////            localWatershedIterations->SetPixel(x, y, z, localWatershedIteration); // set the iteration number for the last pore in the shared list
//
//            // increment this pixel's volume
//            newSharedPixelType->volume++;
//
//            // decrement the volume of the other entity
//            if (currentId <= lastPoreId)
//            {
//                currentPore->volume--;
//            }
//            else
//            {
//                sharedPixelType->volume--;
//            }
//
//            return false;
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
//    SharedPixelType* WatershedComputer::GetOrAddSharedPixelType(const vector<PoreThroatIdType>& sharedPoreIds) const
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
//    WatershedComputer::~WatershedComputer()
//    {
//    }
//}
