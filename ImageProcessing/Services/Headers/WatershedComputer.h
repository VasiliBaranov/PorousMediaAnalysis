// Copyright (c) 2013 Vasili Baranau
// Distributed under the MIT software license
// See the accompanying file License.txt or http://opensource.org/licenses/MIT

#ifndef ImageProcessing_Services_Headers_WatershedComputer_h
#define ImageProcessing_Services_Headers_WatershedComputer_h

#include <queue>
#include "ImageProcessing/Services/Headers/BasePoreAndThroatComputer.h"

namespace Services
{
    class WatershedComputer : public BasePoreAndThroatComputer
    {
    // Classes
    private:
        struct NodeToVisit;

    public:
        WatershedComputer(const ActiveAreaComputer& currentActiveAreaComputer, const Serializer& currentSerializer);

        void AssignPoreAndThroatIds(const Model::Config& config) const;

        virtual ~WatershedComputer();

    private:
        void AssignPoreIds(const std::vector<Model::ActiveArea>& activeAreas) const;

        void AssignPoreIdsInActiveArea() const;

        void FillInitialWatershedQueue(std::vector< std::queue<NodeToVisit> >* nodesToVisitByDepth) const;

        void UpdateAllNeighborsAndAddToQueue(const NodeToVisit& node, std::vector< std::queue<NodeToVisit> >* nodesToVisitByDepth) const;

        void UpdateNeighborAndAddToQueue(const NodeToVisit& parentNode,
                const Core::DiscreteSpatialVector& neighbor,
                std::vector< std::queue<NodeToVisit> >* nodesToVisitByDepth) const;

        bool UpdatePixelAndSharedBalls(NodeToVisit* node) const;

        DISALLOW_COPY_AND_ASSIGN(WatershedComputer);

    // Classes
    private:
        struct NodeToVisit
        {
            Core::DiscreteSpatialVector position;
            Model::EuclideanDistanceSquareType depth;
            Model::Pore* pore;

            // To allow default copy constructor
            NodeToVisit()
            {

            }

            NodeToVisit(const Core::DiscreteSpatialVector& currentPosition) :
                position(currentPosition)
            {

            }

            NodeToVisit(int x, int y, int z)
            {
                position[Core::Axis::X] = x;
                position[Core::Axis::Y] = y;
                position[Core::Axis::Z] = z;
            }
        };
    };
}

#endif /* ImageProcessing_Services_Headers_WatershedComputer_h */

