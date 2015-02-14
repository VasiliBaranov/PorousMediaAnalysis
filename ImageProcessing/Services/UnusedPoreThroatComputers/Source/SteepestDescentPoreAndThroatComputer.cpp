//// Copyright (c) 2013 Vasili Baranau
//// Distributed under the MIT software license
//// See the accompanying file License.txt or http://opensource.org/licenses/MIT
//
//#include "../Headers/SteepestDescentPoreAndThroatComputer.h"
//
//#include <string>
//#include <cmath>
//#include "stdio.h"
//#include <map>
//#include <stack>
//#include <queue>
//
//#include "Core/Headers/Path.h"
//#include "Core/Headers/MemoryUtility.h"
//#include "ImageProcessing/Model/Headers/Config.h"
//#include "ImageProcessing/Model/Headers/DataArray.h"
//#include "ImageProcessing/Services/Headers/ActiveAreaComputer.h"
//#include "ImageProcessing/Services/Headers/Serializer.h"
//#include "ImageProcessing/Services/Headers/InnerBallsRemover.h"
//#include "ImageProcessing/Services/Headers/ContainingBallsAssigner.h"
//
//using namespace std;
//using namespace Core;
//using namespace Model;
//using namespace Services;
//
//namespace Services
//{
//    const PoreThroatIdType SteepestDescentPoreAndThroatComputer::UNKNOWN_INDEX = 0;
//
//    SteepestDescentPoreAndThroatComputer::SteepestDescentPoreAndThroatComputer(const ActiveAreaComputer& currentActiveAreaComputer, const Serializer& currentSerializer) :
//            activeAreaComputer(currentActiveAreaComputer), serializer(currentSerializer)
//    {
//        isProcessedMask = NULL;
//    }
//
//    void SteepestDescentPoreAndThroatComputer::AssignPoreAndThroatIds(const Config& config) const
//    {
//        this->config = &config;
//        vector<ActiveArea> activeAreas;
//
//        IntermediateStatistics intermediateStatistics;
//        string intermediateStatisticsPath = Path::Append(config.baseFolder, INTERMEDIATE_STATISTICS_FILE_NAME);
//        serializer.ReadIntermediateStatistics(intermediateStatisticsPath, &intermediateStatistics);
//        FLOAT_TYPE marginSize = config.maxPoreRadiusToSeedRadiusRatio * sqrt(intermediateStatistics.maxEuclideanDistanceSquare);
//        Margin margin(Margin::Pixels, marginSize);
//
//        boost::array<size_t, 3> imageSize;
//        StlUtilities::Copy(config.imageSize, &imageSize);
//        size_t pixelsCount = VectorUtilities::GetProduct(imageSize);
//        FLOAT_TYPE maximumBallsFraction = static_cast<FLOAT_TYPE>(intermediateStatistics.maximumBallsCount) / pixelsCount;
//
//        // MaximumBall will be used for sorting
//        FLOAT_TYPE averageBytesPerPixel = sizeof(IsMaximumBallMaskType) + sizeof(EuclideanDistanceSquareType) + // sizeof(EuclideanDistanceSquareType) +
//                sizeof(PoreThroatIdType) + maximumBallsFraction * sizeof(MaximumBall);
//
//        vector<Axis::Type> priorityAxes;
//        activeAreaComputer.ComputeActiveAreas(config, priorityAxes, margin, averageBytesPerPixel, &activeAreas);
//
//        string maximumBallsMaskPath = Path::Append(config.baseFolder, IS_MAXIMUM_BALL_MASK_FOLDER_NAME);
//        DataArray<IsMaximumBallMaskType> maximumBallsMaskLocal(serializer, config, maximumBallsMaskPath, activeAreas);
//        maximumBallsMask = &maximumBallsMaskLocal;
//
//        string containingBallRadiiSquaresPath = Path::Append(config.baseFolder, CONTAINING_BALL_RADII_SQUARES_FOLDER_NAME);
//        DataArray<EuclideanDistanceSquareType> containingBallRadiiSquaresLocal(serializer, config, containingBallRadiiSquaresPath, activeAreas);
//        containingBallRadiiSquares = &containingBallRadiiSquaresLocal;
//
//        string poreThroatIdsPath = Path::Append(config.baseFolder, STEEPEST_DESCENT_PIXEL_PORE_THROAT_IDS_FOLDER_NAME);
//        DataArray<PoreThroatIdType> poreThroatIdsLocal(serializer, config, poreThroatIdsPath, activeAreas);
//        poreThroatIdsLocal.defaultValue = UNKNOWN_INDEX;
//        poreThroatIds = &poreThroatIdsLocal;
//
//        if (isProcessedMask != NULL)
//        {
//            MemoryUtility::Free3DArray(isProcessedMask);
//        }
//        pixelsToMaskCenter = ceil(marginSize);
//        int localIsProcessedSize = 2 * pixelsToMaskCenter + 1;
//        isProcessedMask = MemoryUtility::Allocate3DArray<bool>(localIsProcessedSize, localIsProcessedSize, localIsProcessedSize);
//
//        firstSharedId = std::numeric_limits<PoreThroatIdType>::max();
//        poresByIds.clear();
//        sharedPixelTypesByIds.clear();
//        sharedPixelTypeIdsBySharedPores.clear();
//        AssignPoreIds(activeAreas);
//    }
//
//
//    SteepestDescentPoreAndThroatComputer::~SteepestDescentPoreAndThroatComputer()
//    {
//        if (isProcessedMask != NULL)
//        {
//            MemoryUtility::Free3DArray(isProcessedMask);
//        }
//    }
//
//    void SteepestDescentPoreAndThroatComputer::AssignPoreIds(const vector<ActiveArea>& activeAreas) const
//    {
//        printf("Assigning pore indexes in the entire image...\n");
//        PoreThroatIdType startingPoreId = UNKNOWN_INDEX + 127; // add 127 to make even the first pore visible, if indexes are interpreted as CMYK colors (127 is a rather strong cyan)
//        for (size_t activeAreaIndex = 0; activeAreaIndex < activeAreas.size(); ++activeAreaIndex)
//        {
//            printf("Active area %d / %d...\n", activeAreaIndex + 1, activeAreas.size());
//            this->activeArea = &activeAreas[activeAreaIndex];
//
//            maximumBallsMask->ChangeActiveArea(activeAreaIndex);
//            containingBallRadiiSquares->ChangeActiveArea(activeAreaIndex);
//            poreThroatIds->ChangeActiveArea(activeAreaIndex);
//
//            startingPoreId = AssignPoreIdsInActiveArea(startingPoreId);
//        }
//
//        poreThroatIds->WriteCurrentActiveArea();
//
//        // Write pores to a file
//        string poresFilePath = Path::Append(config->baseFolder, STEEPEST_DESCENT_PORES_FILE_NAME);
//        serializer.WritePores(poresFilePath, poresByIds);
//
//        string connectingBallsFilePath = Path::Append(config->baseFolder, STEEPEST_DESCENT_SHARED_PIXEL_TYPES_FILE_NAME);
//        serializer.WriteSharedPixelTypes(connectingBallsFilePath, sharedPixelTypesByIds);
//    }
//
//    PoreThroatIdType SteepestDescentPoreAndThroatComputer::AssignPoreIdsInActiveArea(PoreThroatIdType startingPoreId) const
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
//    void SteepestDescentPoreAndThroatComputer::FillDescendingMaximumBalls(vector<MaximumBall>* maximumBalls) const
//    {
//        EuclideanDistanceSquareType minPoreSeedRadiusSquare = static_cast<EuclideanDistanceSquareType>(config->minPoreSeedRadiusInPixels) * config->minPoreSeedRadiusInPixels;
//        for (int z = activeArea->activeBox.leftCorner[Core::Axis::Z]; z < activeArea->activeBox.exclusiveRightCorner[Core::Axis::Z]; ++z)
//        {
//            for (int x = activeArea->activeBox.leftCorner[Core::Axis::X]; x < activeArea->activeBox.exclusiveRightCorner[Core::Axis::X]; ++x)
//            {
//                for (int y = activeArea->activeBox.leftCorner[Core::Axis::Y]; y < activeArea->activeBox.exclusiveRightCorner[Core::Axis::Y]; ++y)
//                {
//                    IsMaximumBallMaskType maximumBallMask = maximumBallsMask->GetPixel(x, y, z);
//                    EuclideanDistanceSquareType ballRadiusSquare = containingBallRadiiSquares->GetPixel(x, y, z);
//
//                    if ((maximumBallMask != InnerBallsRemover::OUTER_BALL_MASK) || (ballRadiusSquare < minPoreSeedRadiusSquare))
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
//    PoreThroatIdType SteepestDescentPoreAndThroatComputer::AssignPoreIdsInActiveArea(const vector<MaximumBall>& maximumBalls, PoreThroatIdType startingPoreId) const
//    {
//        numberOfAssignedPixels = 0;
//
//        PoreThroatIdType poreId = startingPoreId;
//        for (size_t i = 0; i < maximumBalls.size(); ++i)
//        {
//            const MaximumBall& maximumBall = maximumBalls[i];
//
//            PoreThroatIdType currentPoreId = poreThroatIds->GetPixel(maximumBall.x, maximumBall.y, maximumBall.z);
//            if (currentPoreId != UNKNOWN_INDEX)
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
//    void SteepestDescentPoreAndThroatComputer::RemoveEmptySharedPixelTypes() const
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
//    void SteepestDescentPoreAndThroatComputer::AssignPoreIdRecursively(Pore* pore) const
//    {
//        size_t elementsCount = (2 * pixelsToMaskCenter + 1) * (2 * pixelsToMaskCenter + 1) * (2 * pixelsToMaskCenter + 1);
//        std::fill(isProcessedMask[0][0], isProcessedMask[0][0] + elementsCount, false);
//
//        EuclideanDistanceSquareType containingBallRadiusSquare = containingBallRadiiSquares->GetPixel(pore->seedPosition);
//
//        vector< queue<Pixel> > nodesToVisitByDepth(containingBallRadiusSquare + 1);
//
//        Plateau firstPlateau;
//        firstPlateau.ballRadiusSquare = containingBallRadiusSquare;
//        firstPlateau.parentBallRadiusSquare = numeric_limits<EuclideanDistanceSquareType>::max();
//        firstPlateau.seed = pore->seedPosition;
//
//        FillPlateauAndMarkPixelsProcessed(*pore, &firstPlateau);
//        AddPlateauNeighborsToQueue(firstPlateau, *pore, &nodesToVisitByDepth);
//        SetPoreInPlateauAndNeighbors(firstPlateau, pore);
//
//        for (int depth = containingBallRadiusSquare; depth >= 0; --depth) // can't use unsigned depth, as operation "--depth" will produce max_int, when depth == 0, and the cycle will continue (with access violations)
//        {
//            queue<Pixel>& nodesToVisit = nodesToVisitByDepth[depth];
//
//            while (!nodesToVisit.empty())
//            {
//                // TODO: improve, use NodeToVisit& node = nodesToVisit.front(), call pop later, when node is not needed anymore
//                Pixel node = nodesToVisit.front();
//                nodesToVisit.pop();
//
//                int localNodeX = node.position[Axis::X] - pore->seedPosition[Axis::X] + pixelsToMaskCenter;
//                int localNodeY = node.position[Axis::Y] - pore->seedPosition[Axis::Y] + pixelsToMaskCenter;
//                int localNodeZ = node.position[Axis::Z] - pore->seedPosition[Axis::Z] + pixelsToMaskCenter;
//
//                bool processed = isProcessedMask[localNodeX][localNodeY][localNodeZ];
//                if (processed)
//                {
//                    continue;
//                }
//
//                // We use queues and watershed algorithm, so even if "node" is added multiple times to the queue (from pixels with different depths),
//                // we reach it for the first time, when it has the highest possible parentBallRadiusSquare (for this pore).
//                // Thus, node.parentBallRadiusSquare is the right one.
//                // We will mark "node" as processed later, and will never process it again.
//                Plateau plateau;
//                plateau.seed = node.position;
//                plateau.parentBallRadiusSquare = node.parentBallRadiusSquare;
//                plateau.ballRadiusSquare = node.ballRadiusSquare;
//
//                // Has steepest descent neighbors
//
//                // If true, continue
//
//                // else, redo propagation, add neighbors directly to nodesToVisitByDepth. Do not update pixels outside plateau
//
//                FillPlateauAndMarkPixelsProcessed(*pore, &plateau);
//
//                bool hasSteepestDescentNeighborOutsidePore = HasSteepestDescentNeighborOutsidePore(plateau, *pore);
//                if (hasSteepestDescentNeighborOutsidePore)
//                {
//                    continue;
//                }
//
//                AddPlateauNeighborsToQueue(plateau, *pore, &nodesToVisitByDepth);
//                SetPoreInPlateauAndNeighbors(plateau, pore);
//            }
//        }
//
//        // Do propagation once again, ensure that there are no uncovered pure pixels
//    }
//
//    bool SteepestDescentPoreAndThroatComputer::HasSteepestDescentNeighborOutsidePore(const Plateau& plateau, const Pore& pore) const
//    {
//        for (size_t neighborIndex = 0; neighborIndex < plateau.neighbors.size(); ++neighborIndex)
//        {
//            const Pixel& neighbor = plateau.neighbors[neighborIndex];
//            if (neighbor.ballRadiusSquare > plateau.parentBallRadiusSquare)
//            {
//                bool neighborBelongsToPore = PixelBelongsToPore(neighbor.position, pore);
//                if (!neighborBelongsToPore)
//                {
//                    return true;
//                }
//            }
//        }
//
//        return false;
//    }
//
//    bool SteepestDescentPoreAndThroatComputer::PixelBelongsToPore(const DiscreteSpatialVector& position, const Pore& pore) const
//    {
//        PoreThroatIdType pixelId = poreThroatIds->GetPixel(position);
//        if (pixelId == pore.id)
//        {
//            return true;
//        }
//        else if (pixelId >= firstSharedId)
//        {
//            SharedPixelType& sharedPixelType = sharedPixelTypesByIds[pixelId];
//            if (StlUtilities::Contains(sharedPixelType.poreIds, pore.id))
//            {
//                return true;
//            }
//        }
//
//        return false;
//    }
//
//    void SteepestDescentPoreAndThroatComputer::FillPlateauAndMarkPixelsProcessed(const Pore& pore, Plateau* plateau) const
//    {
//        int localNodeX = plateau->seed[Axis::X] - pore.seedPosition[Axis::X] + pixelsToMaskCenter;
//        int localNodeY = plateau->seed[Axis::Y] - pore.seedPosition[Axis::Y] + pixelsToMaskCenter;
//        int localNodeZ = plateau->seed[Axis::Z] - pore.seedPosition[Axis::Z] + pixelsToMaskCenter;
//        isProcessedMask[localNodeX][localNodeY][localNodeZ] = true;
//
//        plateau->plateauPixels.push_back(plateau->seed);
//
//        queue<DiscreteSpatialVector> nodesToVisit;
//        nodesToVisit.push(plateau->seed);
//
//        while (!nodesToVisit.empty())
//        {
//            DiscreteSpatialVector node = nodesToVisit.front();
//            nodesToVisit.pop();
//
//            int x = node[Axis::X];
//            int y = node[Axis::Y];
//            int z = node[Axis::Z];
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
//                        // D3Q7 lattice. Ensure that no more than one coordinate is changed
//                        if ((neighborX != x && neighborY != y) ||
//                                (neighborX != x && neighborZ != z) ||
//                                (neighborY != y && neighborZ != z))
//                        {
//                            continue;
//                        }
//
//                        EuclideanDistanceSquareType neighborRadiusSquare = containingBallRadiiSquares->GetPixel(neighborX, neighborY, neighborZ);
//                        if (neighborRadiusSquare == 0)
//                        {
//                            continue;
//                        }
//
//                        if (neighborRadiusSquare == plateau->ballRadiusSquare)
//                        {
//                            int localNodeX = neighborX - pore.seedPosition[Axis::X] + pixelsToMaskCenter;
//                            int localNodeY = neighborY - pore.seedPosition[Axis::Y] + pixelsToMaskCenter;
//                            int localNodeZ = neighborZ - pore.seedPosition[Axis::Z] + pixelsToMaskCenter;
//
//                            bool processed = isProcessedMask[localNodeX][localNodeY][localNodeZ];
//                            if (processed)
//                            {
//                                continue;
//                            }
//
//                            isProcessedMask[localNodeX][localNodeY][localNodeZ] = true;
//
//                            size_t oldSize = plateau->plateauPixels.size();
//                            plateau->plateauPixels.resize(oldSize + 1);
//
//                            DiscreteSpatialVector& newPixel = plateau->plateauPixels[oldSize];
//                            newPixel[Axis::X] = neighborX;
//                            newPixel[Axis::Y] = neighborY;
//                            newPixel[Axis::Z] = neighborZ;
//
//                            nodesToVisit.push(newPixel);
//                        }
//                        else
//                        {
//                            size_t oldSize = plateau->neighbors.size();
//                            plateau->neighbors.resize(oldSize + 1);
//
//                            Pixel& neighbor = plateau->neighbors[oldSize];
//                            neighbor.ballRadiusSquare = neighborRadiusSquare;
//                            neighbor.parentBallRadiusSquare = plateau->ballRadiusSquare;
//                            neighbor.position[Axis::X] = neighborX;
//                            neighbor.position[Axis::Y] = neighborY;
//                            neighbor.position[Axis::Z] = neighborZ;
//                        }
//                    }
//                }
//            }
//        }
//
//        StlUtilities::SortAndResizeToUnique(&plateau->neighbors);
//    }
//
//    void SteepestDescentPoreAndThroatComputer::SetPoreInPlateauAndNeighbors(const Plateau& plateau, Pore* pore) const
//    {
//        for (size_t i = 0; i < plateau.plateauPixels.size(); ++i)
//        {
//            const DiscreteSpatialVector& position = plateau.plateauPixels[i];
//            UpdatePixelAndSharedBalls(position, pore);
//        }
//
//        for (size_t i = 0; i < plateau.neighbors.size(); ++i)
//        {
//            const Pixel& neighbor = plateau.neighbors[i];
//            UpdatePixelAndSharedBalls(neighbor.position, pore);
//        }
//    }
//
//    void SteepestDescentPoreAndThroatComputer::AddPlateauNeighborsToQueue(const Plateau& plateau, const Pore& pore, vector<queue<Pixel> >* nodesToVisitByDepth) const
//    {
//        vector<queue<Pixel> >& nodesToVisitByDepthRef = *nodesToVisitByDepth;
//        for (size_t i = 0; i < plateau.neighbors.size(); ++i)
//        {
//            const Pixel& neighbor = plateau.neighbors[i];
//
//            if (neighbor.ballRadiusSquare < plateau.ballRadiusSquare)
//            {
//                bool neighborBelongsToPore = PixelBelongsToPore(neighbor.position, pore);
//                if (!neighborBelongsToPore)
//                {
//                    queue<Pixel>& nodesToVisit = nodesToVisitByDepthRef[neighbor.ballRadiusSquare];
//                    nodesToVisit.push(neighbor);
//                }
//            }
//        }
//    }
//
//    // Returns "shall continue recursion"
//    bool SteepestDescentPoreAndThroatComputer::UpdatePixelAndSharedBalls(const DiscreteSpatialVector& position, Pore* pore) const
//    {
//        PoreThroatIdType currentId = poreThroatIds->GetPixel(position);
//
//        // ball is marked as the current pore -> do nothing, shall stop recursion
//        if (currentId == pore->id)
//        {
//            return false;
//        }
//
//        // ball is marked as unknown -> update it, shall continue recursion
//        else if (currentId == UNKNOWN_INDEX)
//        {
//            poreThroatIds->SetPixel(position, pore->id);
//            pore->volume++;
//            numberOfAssignedPixels++;
//            return true;
//        }
//
//        // ball is marked as another pore -> add a throat candidate (set connected pore - the current pore and the other one), add throat seed, set pixel, shall continue recursion
//        else if (currentId < firstSharedId)
//        {
//            vector<PoreThroatIdType> sharedPoreIds(2);
//            sharedPoreIds[0] = pore->id;
//            sharedPoreIds[1] = currentId;
//
//            // get or create shared pixel type
//            SharedPixelType* sharedPixelType = GetOrAddSharedPixelType(sharedPoreIds);
//
//            // set the pixel
//            poreThroatIds->SetPixel(position, sharedPixelType->id);
//
//            // increment this pixel's volume
//            sharedPixelType->volume++;
//
//            // decrement the volume of the other pore
//            Pore& currentPore = poresByIds[currentId];
//            currentPore.volume--;
//
//            // continue recursion
//            return true;
//        }
//
//        // ball is marked as shared -> add a candidate (if necessary), add pore to a candidate (if necessary), shall stop recursion
//        else // (currentId >= firstSharedId)
//        {
//            SharedPixelType& sharedPixelType = sharedPixelTypesByIds[currentId];
//            if (StlUtilities::Contains(sharedPixelType.poreIds, pore->id))
//            {
//                return false;
//            }
//            else
//            {
//                vector<PoreThroatIdType> sharedPoreIds = sharedPixelType.poreIds;
//                sharedPoreIds.push_back(pore->id);
//                // get or create shared pixel type for updated pores list
//                SharedPixelType* newSharedPixelType = GetOrAddSharedPixelType(sharedPoreIds);
//
//                // set pixel to this type
//                poreThroatIds->SetPixel(position, newSharedPixelType->id);
//
//                // decrement volume of the old shared type
//                sharedPixelType.volume--;
//
//                // increment volume of the current type
//                newSharedPixelType->volume++;
//
//                // continue recursion
//                return true;
//            }
//        }
//    }
//
//    SharedPixelType* SteepestDescentPoreAndThroatComputer::GetOrAddSharedPixelType(const vector<PoreThroatIdType>& sharedPoreIds) const
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
//}
