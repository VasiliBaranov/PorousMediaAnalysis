// Copyright (c) 2013 Vasili Baranau
// Distributed under the MIT software license
// See the accompanying file License.txt or http://opensource.org/licenses/MIT

#ifndef ImageProcessing_Services_Headers_ShortestPathComputer_h
#define ImageProcessing_Services_Headers_ShortestPathComputer_h

#include <vector>
#include "ImageProcessing/Services/Headers/BaseDataArrayProcessor.h"

namespace Core { template<class TContainer, class TCompare> class OrderedPriorityQueue; }

namespace Services
{
    class ShortestPathComputer : public BaseDataArrayProcessor
    {
    private:
        class Node;
        class NodeComparer;
        struct BoxPlane;

        // Cache variables
        mutable std::vector<Model::ActiveArea> activeAreas;
        mutable std::vector<Model::ShortestPathDistanceType> pixelDifferenceSqrts;

        mutable std::vector<BoxPlane> reachedBoundaryPlanes;
        mutable Model::CavityIdType currentCavityId;

        mutable size_t voidPixelsCount;
        mutable size_t processedPixelsCount;

    public:
        ShortestPathComputer(const ActiveAreaComputer& currentActiveAreaComputer, const Serializer& currentSerializer);

        void ComputeShortestPathDistances(const Model::Config& config) const;

        virtual ~ShortestPathComputer();

    private:
        void FillActiveAreas() const;

        void ComputeShortestPathDistances(std::vector<Model::CavityDescription>* cavities) const;

        void ComputeShortestPathDistances(const Core::DiscreteSpatialVector& startingPosition, std::vector<Node>* unvisitedNodes, Core::OrderedPriorityQueue<std::vector<Node>, NodeComparer>* unvisitedNodesQueue) const;

        void InitializeDataArraysAndUnvisitedNodes(std::vector<Node>* unvisitedNodes) const;

        void InitializeDataArraysAndUnvisitedNodesAndQueue(std::vector<Node>* unvisitedNodes, Core::OrderedPriorityQueue<std::vector<Node>, NodeComparer>* unvisitedNodesQueue) const;

        void UpdateDistancesToNeighbors(Node& closestNode, Model::ShortestPathDistanceType distanceToCurrentPixel, Core::OrderedPriorityQueue<std::vector<Node>, NodeComparer>* unvisitedNodesQueue) const;

        Model::ShortestPathDistanceType GetDistanceToNeighbor(const Core::DiscreteSpatialVector& nodePosition, const Core::DiscreteSpatialVector& neighborPosition) const;

        bool SelectStartingPosition(Core::DiscreteSpatialVector* startingPosition, int* boxHalfSize) const;

        void FillMonolithCenter(Core::DiscreteSpatialVector* center) const;

        int GetMaxBoxHalfSize() const;

        void AddCavityDescription(const Core::DiscreteSpatialVector& startingPosition, size_t previousProcessedPixelsCount, size_t processedPixelsCount, std::vector<Model::CavityDescription>* cavities) const;

        void FillImageSizeInclusive(Core::DiscreteSpatialVector* imageSizeInclusive) const;

        bool FindVoidPixelOnBoxBoundaries(const Core::DiscreteSpatialVector& boxCenter, int boxHalfSize, Core::DiscreteSpatialVector* position) const;

        void UpdateReachedBoundaryPlanes(const Core::DiscreteSpatialVector& position) const;

        size_t GetLinearIndex(const Core::DiscreteSpatialVector& position) const;

        int GetWallsCount() const;

        void FillAllBoundaryPlanes(std::vector<BoxPlane>* boundaryPlanes) const;

        DISALLOW_COPY_AND_ASSIGN(ShortestPathComputer);

    private:
        class Node
        {
        public:
            Core::DiscreteSpatialVector position;
            bool processedOrSolid;
        };

        class NodeComparer
        {
        public:
            Model::DataArray<Model::ShortestPathDistanceType>* shortestPathDistances;

        public:
            bool operator()(const Node& i, const Node& j)
            {
                if (i.processedOrSolid)
                {
                    return false;
                }

                if (j.processedOrSolid)
                {
                    return true;
                }

                return shortestPathDistances->GetPixel(i.position) < shortestPathDistances->GetPixel(j.position);
            }
        };

        struct BoxPlane
        {
            Core::Axis::Type normalAxis;
            int coordinateOnNormalAxis;

            bool operator==(const BoxPlane& other) const
            {
                return this->normalAxis == other.normalAxis &&
                    this->coordinateOnNormalAxis == other.coordinateOnNormalAxis;
            }
        };
    };
}

#endif /* ImageProcessing_Services_Headers_ShortestPathComputer_h */

