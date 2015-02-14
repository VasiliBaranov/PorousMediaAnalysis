//// Copyright (c) 2013 Vasili Baranau
//// Distributed under the MIT software license
//// See the accompanying file License.txt or http://opensource.org/licenses/MIT
//
//#include "../Headers/OnePixelBoundaryThroatComputer.h"
//
//#include <string>
//#include <cmath>
//#include "stdio.h"
//#include <map>
//#include <stack>
//
//#include "Core/Headers/Path.h"
//#include "ImageProcessing/Model/Headers/Config.h"
//#include "ImageProcessing/Model/Headers/DataArray.h"
//#include "ImageProcessing/Services/Headers/ActiveAreaComputer.h"
//#include "ImageProcessing/Services/Headers/Serializer.h"
//#include "ImageProcessing/Services/Headers/InnerBallsRemover.h"
//#include "ImageProcessing/Services/Headers/PoreAndThroatComputer.h"
//
//using namespace std;
//using namespace Core;
//using namespace Model;
//using namespace Services;
//
//namespace Services
//{
//    OnePixelBoundaryThroatComputer::OnePixelBoundaryThroatComputer(const ActiveAreaComputer& currentActiveAreaComputer, const Serializer& currentSerializer) :
//            activeAreaComputer(currentActiveAreaComputer), serializer(currentSerializer)
//    {
//    }
//
//    void OnePixelBoundaryThroatComputer::AssignPoreAndThroatIds(const Config& config) const
//    {
//        this->config = &config;
//
//        FillPoresByIds();
//        FillOldSharedPixelTypesByIds();
//
//        firstNewSharedId = std::numeric_limits<PoreThroatIdType>::max();
//        sharedPixelTypesByIds.clear();
//        sharedPixelTypeIdsBySharedPores.clear();
//
//        vector<ActiveArea> activeAreas;
//        IntermediateStatistics intermediateStatistics;
//        string intermediateStatisticsPath = Path::Append(config.baseFolder, INTERMEDIATE_STATISTICS_FILE_NAME);
//        serializer.ReadIntermediateStatistics(intermediateStatisticsPath, &intermediateStatistics);
//        Margin margin(Margin::Pixels, config.maxPoreRadiusToSeedRadiusRatio * sqrt(intermediateStatistics.maxEuclideanDistanceSquare));
//
//        FLOAT_TYPE averageBytesPerPixel = sizeof(EuclideanDistanceSquareType) + sizeof(PoreThroatIdType) + sizeof(PoreThroatIdType);
//
//        vector<Axis::Type> priorityAxes;
//        activeAreaComputer.ComputeActiveAreas(config, priorityAxes, margin, averageBytesPerPixel, &activeAreas);
//
//        string containingBallRadiiSquaresPath = Path::Append(config.baseFolder, CONTAINING_BALL_RADII_SQUARES_FOLDER_NAME);
//        DataArray<EuclideanDistanceSquareType> containingBallRadiiSquaresLocal(serializer, config, containingBallRadiiSquaresPath, activeAreas);
//        containingBallRadiiSquares = &containingBallRadiiSquaresLocal;
//
//        string poreThroatIdsPath = Path::Append(config.baseFolder, PORE_THROAT_IDS_FOLDER_NAME);
//        DataArray<PoreThroatIdType> oldPoreThroatIdsLocal(serializer, config, poreThroatIdsPath, activeAreas);
//        oldPoreThroatIdsLocal.defaultValue = PoreAndThroatComputer::UNKNOWN_INDEX;
//        oldPoreThroatIds = &oldPoreThroatIdsLocal;
//
//        string poreThroatIdsOnePixelPath = Path::Append(config.baseFolder, ONE_PIXEL_PORE_THROAT_IDS_FOLDER_NAME);
//        DataArray<PoreThroatIdType> poreThroatIdsLocal(serializer, config, poreThroatIdsOnePixelPath, activeAreas);
//        poreThroatIdsLocal.defaultValue = PoreAndThroatComputer::UNKNOWN_INDEX;
//        poreThroatIds = &poreThroatIdsLocal;
//
//        AssignPoreIds(activeAreas);
//    }
//
//    void OnePixelBoundaryThroatComputer::FillOldSharedPixelTypesByIds() const
//    {
//        string sharedPixelTypesFilePath = Path::Append(config->baseFolder, SHARED_PIXEL_TYPES_FILE_NAME);
//        serializer.ReadSharedPixelTypes(sharedPixelTypesFilePath, &oldSharedPixelTypesByIds);
//
//        // Sort pores in each pixel by their radii
//        PoreWithRadiusIdComparer comparer(poresByIds);
//        for (map<PoreThroatIdType, SharedPixelType>::iterator it = oldSharedPixelTypesByIds.begin(); it != oldSharedPixelTypesByIds.end(); ++it)
//        {
//            SharedPixelType& sharedPixelType = (*it).second;
//            StlUtilities::Sort(&sharedPixelType.poreIds, comparer);
//        }
//    }
//
//    void OnePixelBoundaryThroatComputer::FillPoresByIds() const
//    {
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
//    void OnePixelBoundaryThroatComputer::AssignPoreIds(const vector<ActiveArea>& activeAreas) const
//    {
//        printf("Assigning pore indexes in the entire image...\n");
//        for (size_t activeAreaIndex = 0; activeAreaIndex < activeAreas.size(); ++activeAreaIndex)
//        {
//            printf("Active area %d / %d...\n", activeAreaIndex + 1, activeAreas.size());
//            this->activeArea = &activeAreas[activeAreaIndex];
//
//            containingBallRadiiSquares->ChangeActiveArea(activeAreaIndex);
//            oldPoreThroatIds->ChangeActiveArea(activeAreaIndex);
//            poreThroatIds->ChangeActiveArea(activeAreaIndex);
//
//            AssignPoreIdsInActiveArea();
//        }
//
//        poreThroatIds->WriteCurrentActiveArea();
//
//        // Write pores to a file
//        string poresFilePath = Path::Append(config->baseFolder, ONE_PIXEL_PORES_FILE_NAME);
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
//        serializer.WritePores(poresFilePath, simplePoresByIds);
//
//        string connectingBallsFilePath = Path::Append(config->baseFolder, ONE_PIXEL_SHARED_PIXEL_TYPES_FILE_NAME);
//        serializer.WriteSharedPixelTypes(connectingBallsFilePath, sharedPixelTypesByIds);
//    }
//
//    void OnePixelBoundaryThroatComputer::AssignPoreIdsInActiveArea() const
//    {
//        printf("Assigning pore indexes in active area...\n");
//        numberOfAssignedPixels = 0;
//
//        for (map<PoreThroatIdType, PoreWithRadius>::iterator it = poresByIds.begin(); it != poresByIds.end(); ++it)
//        {
//            // NOTE: if propagation happens from smallest pores
////        for (map<PoreThroatIdType, PoreWithRadius>::reverse_iterator it = poresByIds.rbegin(); it != poresByIds.rend(); ++it)
////        {
//
//            PoreWithRadius& pore = (*it).second;
//            if (activeArea->activeBox.Contains(pore.seedPosition))
//            {
//                AssignPoreIdRecursively(&pore);
//
//                RemoveEmptySharedPixelTypes();
//
//                printf("Number of assigned pixels is %d\n", numberOfAssignedPixels);
//            }
//        }
//    }
//
//    // I could track empty throats every time when i decrease the volume of the shared pixels during recursion,
//    // but each pore may be comprised of several millions of pixels, while there always no more than several thousands of shared pixel types,
//    // so doing it here seems to be a better solution.
//    void OnePixelBoundaryThroatComputer::RemoveEmptySharedPixelTypes() const
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
//    void OnePixelBoundaryThroatComputer::AssignPoreIdRecursively(PoreWithRadius* pore) const
//    {
//        // This could be a recursive algorithm, rewritten through iteration with an external stack, because for real monoliths i get stack overflow.
//        stack<NodeToVisit> nodesToVisit;
//        NodeToVisit firstParams(pore->seedPosition[Axis::X], pore->seedPosition[Axis::Y], pore->seedPosition[Axis::Z]); // the first pore ball is by definition not shared, so we won't need parent at all
//        nodesToVisit.push(firstParams);
//
//        while (!nodesToVisit.empty())
//        {
//            NodeToVisit node = nodesToVisit.top();
//            nodesToVisit.pop();
//            bool shouldContiueRecursion = UpdatePixelAndSharedBalls(node, pore);
//            if (!shouldContiueRecursion)
//            {
//                continue;
//            }
//
//            if (numberOfAssignedPixels % 10000 == 0)
//            {
//                printf("Number of assigned pixels is %d\n", numberOfAssignedPixels);
//            }
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
//            EuclideanDistanceSquareType ballRadiusSquare = containingBallRadiiSquares->GetPixel(x, y, z);
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
//                        EuclideanDistanceSquareType neighborBallRadiusSquare = containingBallRadiiSquares->GetPixel(neighborX, neighborY, neighborZ);
//                        bool neighborVoid = neighborBallRadiusSquare > 0;
//
//                        if (neighborVoid && neighborBallRadiusSquare <= ballRadiusSquare)
//                        {
//                            NodeToVisit callParams(neighborX, neighborY, neighborZ);
//                            nodesToVisit.push(callParams);
//                        }
//                    }
//                }
//            }
//        }
//    }
//
//    // Returns "shall continue recursion"
//    bool OnePixelBoundaryThroatComputer::UpdatePixelAndSharedBalls(const NodeToVisit& node, Pore* pore) const
//    {
//        int x = node.x;
//        int y = node.y;
//        int z = node.z;
//        PoreThroatIdType currentId = poreThroatIds->GetPixel(x, y, z);
//
//        // Pixel contains the current pore -> do nothing, shall stop recursion
//        if (currentId == pore->id)
//        {
//            return false;
//        }
//
//        SharedPixelType* sharedPixelType = NULL;
//        if (currentId > lastPoreId)
//        {
//            sharedPixelType = &sharedPixelTypesByIds[currentId];
//            if (StlUtilities::Contains(sharedPixelType->poreIds, pore->id))
//            {
//                return false;
//            }
//        }
//
//        // Read the initial pore/throat id
//        PoreThroatIdType currentOldId = oldPoreThroatIds->GetPixel(x, y, z);
//
//        // Determine if the current pore is the largest one in the old pixel
//        bool isLargestPore = false;
//        if (currentOldId <= lastPoreId)
//        {
//            if (currentOldId == pore->id)
//            {
//                isLargestPore = true;
//            }
//            else
//            {
//                throw InvalidOperationException("The pixel is reachable with the pore, contains pore id, but it is not the current pore id. It shall be a shared pixel instead.");
//            }
//        }
//        else
//        {
//            SharedPixelType& oldSharedPixelType = oldSharedPixelTypesByIds[currentOldId];
//            isLargestPore = oldSharedPixelType.poreIds[0] == pore->id;
//        }
//
//        // If the current pore is the first in the initial list: add it to the new list, update volumes, continue
//        // If it is not the first: add it to the new list, update volumes, stop
//        // The current pore can't be absent in the initial list
//
//        // Why we shall not rewrite the current pixel, if the current pore is the first: as there may be smaller pores from other active areas,
//        // which stopped their propagation at this pixel, knowing that here they will encounter a larger pore.
//
//        // Here we extract "add it to the new list, update volumes" common part
//        if (currentId == PoreAndThroatComputer::UNKNOWN_INDEX) // unknown index
//        {
//            poreThroatIds->SetPixel(x, y, z, pore->id);
//            pore->volume++;
//        }
//        else if (currentId > lastPoreId) // shared pixel
//        {
//            vector<PoreThroatIdType> sharedPoreIds = sharedPixelType->poreIds;
//            sharedPoreIds.push_back(pore->id);
//            // get or create shared pixel type for updated pores list
//            SharedPixelType* newSharedPixelType = GetOrAddSharedPixelType(sharedPoreIds);
//
//            // set pixel to this type
//            poreThroatIds->SetPixel(x, y, z, newSharedPixelType->id);
//
//            // decrement volume of the old shared type
//            sharedPixelType->volume--;
//
//            // increment volume of the current type
//            newSharedPixelType->volume++;
//        }
//        else // another pore
//        {
//            vector<PoreThroatIdType> sharedPoreIds(2);
//            sharedPoreIds[0] = pore->id;
//            sharedPoreIds[1] = currentId;
//
//            // get or create shared pixel type
//            SharedPixelType* sharedPixelType = GetOrAddSharedPixelType(sharedPoreIds);
//
//            // set the pixel
//            poreThroatIds->SetPixel(x, y, z, sharedPixelType->id);
//
//            // increment this pixel's volume
//            sharedPixelType->volume++;
//
//            // decrement the volume of the other pore
//            Pore& currentPore = poresByIds[currentId];
//            currentPore.volume--;
//        }
//        numberOfAssignedPixels++;
//
//        bool shouldContinueRecursion = isLargestPore;
//        return shouldContinueRecursion;
//
//        // NOTE: if we are trying to propagate from smallest pores as much as possible
////        return (currentId == PoreAndThroatComputer::UNKNOWN_INDEX);
//    }
//
//    SharedPixelType* OnePixelBoundaryThroatComputer::GetOrAddSharedPixelType(const vector<PoreThroatIdType>& sharedPoreIds) const
//    {
//        map<vector<PoreThroatIdType>, PoreThroatIdType>::iterator it = sharedPixelTypeIdsBySharedPores.find(sharedPoreIds);
//        if (it != sharedPixelTypeIdsBySharedPores.end())
//        {
//            PoreThroatIdType id = (*it).second;
//            return &sharedPixelTypesByIds[id];
//        }
//        else
//        {
//            firstNewSharedId--;
//            SharedPixelType& sharedPixelType = sharedPixelTypesByIds[firstNewSharedId];
//
//            sharedPixelType.id = firstNewSharedId;
//            sharedPixelType.volume = 0;
//            sharedPixelType.poreIds = sharedPoreIds;
//
//            sharedPixelTypeIdsBySharedPores[sharedPoreIds] = firstNewSharedId;
//
//            return &sharedPixelType;
//        }
//    }
//
//    OnePixelBoundaryThroatComputer::~OnePixelBoundaryThroatComputer()
//    {
//    }
//}
