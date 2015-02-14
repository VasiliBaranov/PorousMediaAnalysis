// Copyright (c) 2013 Vasili Baranau
// Distributed under the MIT software license
// See the accompanying file License.txt or http://opensource.org/licenses/MIT

#include "../Headers/PoreAndThroatComputer.h"

#include <string>
#include <cmath>
#include "stdio.h"
#include <stack>

#include "Core/Headers/Path.h"
#include "ImageProcessing/Model/Headers/Config.h"
#include "ImageProcessing/Model/Headers/ModelUtilities.h"
#include "ImageProcessing/Services/Headers/ActiveAreaComputer.h"
#include "ImageProcessing/Services/Headers/Serializer.h"

using namespace std;
using namespace Core;
using namespace Model;
using namespace Services;

namespace Services
{

    PoreAndThroatComputer::PoreAndThroatComputer(const ActiveAreaComputer& currentActiveAreaComputer, const Serializer& currentSerializer) :
            BasePoreAndThroatComputer(currentActiveAreaComputer, currentSerializer)
    {
        dataArrays.AddActiveDataArrayTypes(DataArrayFlags::MaximumBallsMask |
                DataArrayFlags::EuclideanDistanceSquares |
                DataArrayFlags::ContainingBallRadiiSquares |
                DataArrayFlags::PoreThroatIds);
    }

    void PoreAndThroatComputer::AssignPoreAndThroatIds(const Config& config) const
    {
        this->config = &config;
        vector<ActiveArea> activeAreas;

        IntermediateStatistics intermediateStatistics = ReadIntermediateStatistics();
        Margin margin(Margin::Pixels, config.maxPoreRadiusToSeedRadiusRatio * sqrt(static_cast<FLOAT_TYPE>(intermediateStatistics.maxEuclideanDistanceSquare)));

        FLOAT_TYPE maximumBallsFraction = ModelUtilities::GetMaximumBallsFraction(config, intermediateStatistics);

        // MaximumBall will be used for sorting
        FLOAT_TYPE averageBytesPerPixel = dataArrays.GetBytesPerPixel() + maximumBallsFraction * sizeof(MaximumBall);
        vector<Axis::Type> priorityAxes;
        activeAreaComputer.ComputeActiveAreas(config, priorityAxes, margin, averageBytesPerPixel, &activeAreas);

        dataArrays.Initialize(config, activeAreas);

        ClearCache();
        AssignPoreIds(activeAreas);
        dataArrays.Clear();
    }

    void PoreAndThroatComputer::AssignPoreIds(const vector<ActiveArea>& activeAreas) const
    {
        printf("Assigning pore indexes in the entire image...\n");
        PoreThroatIdType startingPoreId = UNKNOWN_PORE_INDEX + 127; // add 127 to make even the first pore visible, if indexes are interpreted as CMYK colors (127 is a rather strong cyan)
        for (size_t activeAreaIndex = 0; activeAreaIndex < activeAreas.size(); ++activeAreaIndex)
        {
            printf("Active area " SIZE_T_FORMAT " / " SIZE_T_FORMAT "...\n", activeAreaIndex + 1, activeAreas.size());
            this->activeArea = &activeAreas[activeAreaIndex];

            dataArrays.ChangeActiveArea(activeAreaIndex);

            startingPoreId = AssignPoreIdsInActiveArea(startingPoreId);
        }

        dataArrays.WriteCurrentActiveArea();

        // Write pores to a file
        string poresFilePath = Path::Append(config->baseFolder, PORES_FILE_NAME);
        serializer.WritePores(Path::Append(config->baseFolder, PORES_FILE_NAME), poresByIds);

        string connectingBallsFilePath = Path::Append(config->baseFolder, SHARED_PIXEL_TYPES_FILE_NAME);
        serializer.WriteSharedPixelTypes(Path::Append(config->baseFolder, SHARED_PIXEL_TYPES_FILE_NAME), sharedPixelTypesByIds);
    }

    PoreThroatIdType PoreAndThroatComputer::AssignPoreIdsInActiveArea(PoreThroatIdType startingPoreId) const
    {
        // Sort maximum balls
        printf("Filling and sorting maximum balls...\n");
        vector<MaximumBall> maximumBalls;
        FillDescendingMaximumBalls(&maximumBalls);

        printf("Assigning pore indexes in active area...\n");
        startingPoreId = AssignPoreIdsInActiveArea(maximumBalls, startingPoreId);
        return startingPoreId;
    }

    void PoreAndThroatComputer::FillDescendingMaximumBalls(vector<MaximumBall>* maximumBalls) const
    {
        for (int z = activeArea->activeBox.leftCorner[Core::Axis::Z]; z < activeArea->activeBox.exclusiveRightCorner[Core::Axis::Z]; ++z)
        {
            for (int x = activeArea->activeBox.leftCorner[Core::Axis::X]; x < activeArea->activeBox.exclusiveRightCorner[Core::Axis::X]; ++x)
            {
                for (int y = activeArea->activeBox.leftCorner[Core::Axis::Y]; y < activeArea->activeBox.exclusiveRightCorner[Core::Axis::Y]; ++y)
                {
                    IsMaximumBallMaskType maximumBallMask = dataArrays.maximumBallsMask.GetPixel(x, y, z);

                    // Can't use containingBallRadiiSquares here, as the center of the maximum ball may be covered by a larger maximum ball.
                    EuclideanDistanceSquareType ballRadiusSquare = dataArrays.euclideanDistanceSquares.GetPixel(x, y, z);

                    if (maximumBallMask != OUTER_BALL_MASK)
                    {
                        continue;
                    }

                    MaximumBall maximumBall;
                    maximumBall.center[Axis::X] = x;
                    maximumBall.center[Axis::Y] = y;
                    maximumBall.center[Axis::Z] = z;
                    maximumBall.ballRadiusSquare = ballRadiusSquare;
                    maximumBalls->push_back(maximumBall);
                }
            }
        }

        MaximumBallComparer comparer;
        StlUtilities::Sort(maximumBalls, comparer);
    }

    PoreThroatIdType PoreAndThroatComputer::AssignPoreIdsInActiveArea(const vector<MaximumBall>& maximumBalls, PoreThroatIdType startingPoreId) const
    {
        numberOfAssignedPixels = 0;

        PoreThroatIdType poreId = startingPoreId;
        for (size_t i = 0; i < maximumBalls.size(); ++i)
        {
            const MaximumBall& maximumBall = maximumBalls[i];

            PoreThroatIdType currentPoreId = dataArrays.poreThroatIds.GetPixel(maximumBall.center);
            if (currentPoreId != UNKNOWN_PORE_INDEX)
            {
                continue;
            }

            printf("Assigning pore index %d for maximum ball " SIZE_T_FORMAT " / " SIZE_T_FORMAT "\n", poreId, i + 1, maximumBalls.size());

            Pore& pore = poresByIds[poreId];
            pore.seedPosition = maximumBall.center;
            pore.seedRadiusSquare = maximumBall.ballRadiusSquare;
            pore.id = poreId;
            pore.volume = 0;
            pore.isBoundaryPore = false;

            // Pore indexes are saved as 32 bit file, which is interpreted as CMYK file (4 bytes of colors).
            // If, e.g., poreId == 255 now (strong cyan) and we increment by 1, we get 1 in the magenta channel and 0 in the cyan channel.
            // Thus, we ensure that at least cyan channel will be visible after increment.
            PoreThroatIdType addition = (poreId % 255 == 0) ? 127 : 1;
            poreId += addition;

            AssignPoreIdRecursively(&pore);

            RemoveEmptySharedPixelTypes();

            printf("Number of assigned pixels is " SIZE_T_FORMAT "\n", numberOfAssignedPixels);
        }

        return poreId;
    }

    // I could track empty throats every time when i decrease the volume of the shared pixels during recursion,
    // but each pore may be comprised of several millions of pixels, while there always no more than several thousands of shared pixel types,
    // so doing it here seems to be a better solution.
    void PoreAndThroatComputer::RemoveEmptySharedPixelTypes() const
    {
        vector<PoreThroatIdType> emptySharedPixelTypeIds;
        for (map<PoreThroatIdType, SharedPixelType>::iterator it = sharedPixelTypesByIds.begin(); it != sharedPixelTypesByIds.end(); ++it)
        {
            const SharedPixelType& sharedPixelType = (*it).second;

            if (sharedPixelType.volume == 0)
            {
                emptySharedPixelTypeIds.push_back(sharedPixelType.id);
            }
        }

        for (size_t i = 0; i < emptySharedPixelTypeIds.size(); ++i)
        {
            PoreThroatIdType id = emptySharedPixelTypeIds[i];
            const SharedPixelType& sharedPixelType = sharedPixelTypesByIds[id];

            sharedPixelTypeIdsBySharedPores.erase(sharedPixelType.poreIds);
            sharedPixelTypesByIds.erase(id);
        }
    }

    // Depth-first search
    void PoreAndThroatComputer::AssignPoreIdRecursively(Pore* pore) const
    {
        // This could be a recursive algorithm, rewritten through iteration with an external stack, because for real monoliths i get stack overflow.
        stack<NodeToVisit> nodesToVisit;
        NodeToVisit firstParams = pore->seedPosition; // the first pore ball is by definition not shared, so we won't need parent at all
        nodesToVisit.push(firstParams);

        while (!nodesToVisit.empty())
        {
            NodeToVisit node = nodesToVisit.top(); // TODO: may be improve, use NodeToVisit& node = nodesToVisit.top(), call pop later, when node is not needed anymore
            nodesToVisit.pop();

            bool shouldContiueRecursion = UpdatePixelAndSharedBalls(node, pore);
            if (!shouldContiueRecursion)
            {
                continue;
            }

            if (numberOfAssignedPixels % 10000 == 0)
            {
                printf("Number of assigned pixels is " SIZE_T_FORMAT "\n", numberOfAssignedPixels);
            }

            AddNeighborsToStack(node, pore, &nodesToVisit);
        }
    }

    void PoreAndThroatComputer::AddNeighborsToStack(const NodeToVisit& node, Pore* pore, stack<DiscreteSpatialVector>* nodesToVisit) const
    {
        int x = node[Axis::X];
        int y = node[Axis::Y];
        int z = node[Axis::Z];

        EuclideanDistanceSquareType ballRadiusSquare = dataArrays.containingBallRadiiSquares.GetPixel(node);

        // for the points that are in the cube for this pixel
        for (int neighborX = x - 1; neighborX <= x + 1; ++neighborX)
        {
            for (int neighborY = y - 1; neighborY <= y + 1; ++neighborY)
            {
                for (int neighborZ = z - 1; neighborZ <= z + 1; ++neighborZ)
                {
                    if (neighborX == x && neighborY == y && neighborZ == z)
                    {
                        continue;
                    }

                    DiscreteSpatialVector neighborVector = {{neighborX, neighborY, neighborZ}};

                    if ((config->stencilType == StencilType::D3Q7) &&
                            ModelUtilities::IsDiagonalStencilVector(node, neighborVector))
                    {
                        continue;
                    }

                    if (ModelUtilities::IsOuterPixel(neighborVector, *config))
                    {
                        pore->isBoundaryPore = true;
                        continue;
                    }

                    if (!activeArea->boxWithMargins.Contains(neighborVector))
                    {
//                        printf("WARNING: Neighbor is outside active area. Pore is not processed fully. Increase the 'Max ratio of pore radius to seed radius' parameter in the config.\n");
                        continue;
                    }

                    EuclideanDistanceSquareType neighborBallRadiusSquare = dataArrays.containingBallRadiiSquares.GetPixel(neighborVector);
                    bool neighborVoid = neighborBallRadiusSquare > 0;

                    if (neighborVoid && neighborBallRadiusSquare <= ballRadiusSquare)
                    {
                        nodesToVisit->push(neighborVector);
                    }
                }
            }
        }
    }

    bool PoreAndThroatComputer::UpdatePixelAndSharedBalls(const NodeToVisit& node, Pore* pore) const
    {
        return BasePoreAndThroatComputer::UpdatePixelAndSharedBalls(node, true, &dataArrays.poreThroatIds, pore);
    }

    PoreAndThroatComputer::~PoreAndThroatComputer()
    {
    }
}
