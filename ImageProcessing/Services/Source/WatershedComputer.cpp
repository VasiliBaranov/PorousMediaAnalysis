// Copyright (c) 2013 Vasili Baranau
// Distributed under the MIT software license
// See the accompanying file License.txt or http://opensource.org/licenses/MIT

#include "../Headers/WatershedComputer.h"

#include "stdio.h"
#include <string>
#include <cmath>

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
    WatershedComputer::WatershedComputer(const ActiveAreaComputer& currentActiveAreaComputer, const Serializer& currentSerializer) :
        BasePoreAndThroatComputer(currentActiveAreaComputer, currentSerializer)
    {
        dataArrays.AddActiveDataArrayTypes(DataArrayFlags::ContainingBallRadiiSquares | DataArrayFlags::PoreThroatIdsWatershed);
    }

    void WatershedComputer::AssignPoreAndThroatIds(const Config& config) const
    {
        this->config = &config;

        vector<ActiveArea> activeAreas;
        IntermediateStatistics intermediateStatistics = ReadIntermediateStatistics();
        FLOAT_TYPE marginInPixels = config.maxPoreRadiusToSeedRadiusRatio * sqrt(static_cast<FLOAT_TYPE>(intermediateStatistics.maxEuclideanDistanceSquare));
        Margin margin(Margin::Pixels, marginInPixels);

        vector<Axis::Type> priorityAxes;
        activeAreaComputer.ComputeActiveAreas(config, priorityAxes, margin, dataArrays.GetBytesPerPixel(), &activeAreas);

        dataArrays.Initialize(config, activeAreas);

        ClearCache();
        serializer.ReadPores(Path::Append(config.baseFolder, PORES_FILE_NAME), &poresByIds);

        AssignPoreIds(activeAreas);
        dataArrays.Clear();
    }

    void WatershedComputer::AssignPoreIds(const vector<ActiveArea>& activeAreas) const
    {
        printf("Assigning pore indexes in the entire image...\n");
        for (size_t activeAreaIndex = 0; activeAreaIndex < activeAreas.size(); ++activeAreaIndex)
        {
            printf("Active area " SIZE_T_FORMAT " / " SIZE_T_FORMAT "...\n", activeAreaIndex + 1, activeAreas.size());
            this->activeArea = &activeAreas[activeAreaIndex];

            dataArrays.ChangeActiveArea(activeAreaIndex);

            AssignPoreIdsInActiveArea();
        }

        dataArrays.WriteCurrentActiveArea();

        // Write pores to a file
        string poresFilePath = Path::Append(config->baseFolder, WATERSHED_PORES_FILE_NAME);
        serializer.WritePores(poresFilePath, poresByIds);

        string connectingBallsFilePath = Path::Append(config->baseFolder, WATERSHED_SHARED_PIXEL_TYPES_FILE_NAME);
        serializer.WriteSharedPixelTypes(connectingBallsFilePath, sharedPixelTypesByIds);
    }

    // True watershed through containing ball radii, all pores at once.
    // Watershed is a variant of breadth-first search, when we do breadth-first search through the nodes with the highest value (which we call depth) at first
    // (starting from all the "watershed seed nodes" simultaneously).
    // Then we decrease the value, and repeat the breadth first search for the current value. And so on.
    // Watersheds from different seeds at a given node value will meet somewhere in the middle of areas with the same depth
    // (more precisely, at nodes, where "local iterations" of propagation from two or more seeds at a given depth (starting from higher depth) become equal.
    void WatershedComputer::AssignPoreIdsInActiveArea() const
    {
        printf("Assigning pore indexes in active area...\n");
        numberOfAssignedPixels = 0;

        vector<queue<NodeToVisit> > nodesToVisitByDepth; // usually currentDepth is no more that ~1000
        FillInitialWatershedQueue(&nodesToVisitByDepth);
        int maxDepth = nodesToVisitByDepth.size() - 1;

        for (int depth = maxDepth; depth >= 0; --depth) // can't use unsigned depth, as operation "--depth" will produce max_int, when depth == 0, and the cycle will continue (with access violations)
        {
            queue<NodeToVisit>& nodesToVisit = nodesToVisitByDepth[depth];

            while (!nodesToVisit.empty())
            {
                // TODO: improve, use NodeToVisit& node = nodesToVisit.front(), call pop later, when node is not needed anymore
                NodeToVisit node = nodesToVisit.front();
                nodesToVisit.pop();

                UpdateAllNeighborsAndAddToQueue(node, &nodesToVisitByDepth);
            }
        }
    }

    void WatershedComputer::UpdateAllNeighborsAndAddToQueue(const NodeToVisit& node,
                    vector< queue<NodeToVisit> >* nodesToVisitByDepth) const
    {
        int x = node.position[Axis::X];
        int y = node.position[Axis::Y];
        int z = node.position[Axis::Z];

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
                    UpdateNeighborAndAddToQueue(node, neighborVector, nodesToVisitByDepth);
                }
            }
        }
    }

    void WatershedComputer::UpdateNeighborAndAddToQueue(const NodeToVisit& parentNode,
            const DiscreteSpatialVector& neighborVector,
            vector< queue<NodeToVisit> >* nodesToVisitByDepth) const
    {
        if ((config->stencilType == StencilType::D3Q7) &&
                 ModelUtilities::IsDiagonalStencilVector(parentNode.position, neighborVector))
         {
             return;
         }

         if (ModelUtilities::IsOuterPixel(neighborVector, *config))
         {
             parentNode.pore->isBoundaryPore = true;
             return;
         }

         if (!activeArea->boxWithMargins.Contains(neighborVector))
         {
//             printf("WARNING: Neighbor is outside active area. Pore is not processed fully. Increase the 'Max ratio of pore radius to seed radius' parameter in the config.\n");
             return;
         }

         EuclideanDistanceSquareType neighborBallRadiusSquare = dataArrays.containingBallRadiiSquares.GetPixel(neighborVector);
         bool neighborVoid = neighborBallRadiusSquare > 0;
         if (!neighborVoid || neighborBallRadiusSquare > parentNode.depth)
         {
             return;
         }

         NodeToVisit neighbor(neighborVector);
         neighbor.depth = neighborBallRadiusSquare;
         neighbor.pore = parentNode.pore;

         bool shouldContiueRecursion = UpdatePixelAndSharedBalls(&neighbor);

         if (shouldContiueRecursion)
         {
             vector< queue<NodeToVisit> >& nodesToVisitByDepthRef = *nodesToVisitByDepth;
             queue<NodeToVisit>& neighborNodesToVisit = nodesToVisitByDepthRef[neighborBallRadiusSquare];
             neighborNodesToVisit.push(neighbor);

             if (numberOfAssignedPixels % 10000 == 0)
             {
                 printf("Number of assigned pixels is " SIZE_T_FORMAT "\n", numberOfAssignedPixels);
             }
         }
    }

    bool WatershedComputer::UpdatePixelAndSharedBalls(NodeToVisit* node) const
    {
        return BasePoreAndThroatComputer::UpdatePixelAndSharedBalls(node->position, false, &dataArrays.poreThroatIdsWatershed, node->pore);
    }

    void WatershedComputer::FillInitialWatershedQueue(vector< queue<NodeToVisit> >* nodesToVisitByDepth) const
    {
        IntermediateStatistics intermediateStatistics = ReadIntermediateStatistics();
        nodesToVisitByDepth->resize(intermediateStatistics.maxEuclideanDistanceSquare + 1);

        vector< queue<NodeToVisit> >& nodesToVisitByDepthRef = *nodesToVisitByDepth;
        for (map<PoreThroatIdType, Pore>::iterator it = poresByIds.begin(); it != poresByIds.end(); ++it)
        {
            Pore& pore = (*it).second;

            if (!activeArea->activeBox.Contains(pore.seedPosition))
            {
                continue;
            }

            pore.isBoundaryPore = false; // reset this value
            pore.volume = 0;

            NodeToVisit firstParams(pore.seedPosition); // the first pore ball is by definition not shared, so we won't need parent at all
            firstParams.depth = pore.seedRadiusSquare;
            firstParams.pore = &pore;
            queue<NodeToVisit>& nodesToVisit = nodesToVisitByDepthRef[pore.seedRadiusSquare];
            nodesToVisit.push(firstParams);

            bool shouldContiueRecursion = UpdatePixelAndSharedBalls(&firstParams);
            // Actually, shouldContiueRecursion shall never be false, as all the pore seeds from the previous steps shall never be rewritten
            // during watershed segmentation, as far as these seeds correspond to containing ball radii plateaus, which are local maxima.
            // E.g., even if we have several active areas and some of the watersheds propagated into neighboring empty areas,
            // they will not cover these maxima. It will lead to strange pores, though.
            // There is another problem with propagation: if the pore propagates too far away (into another active area),
            // and neighboring pores in another active area will not be able to enter it, it will remain empty
            if (!shouldContiueRecursion)
            {
                continue;
            }
        }
    }

    WatershedComputer::~WatershedComputer()
    {
    }
}
