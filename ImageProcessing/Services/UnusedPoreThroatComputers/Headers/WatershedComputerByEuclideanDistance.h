// Copyright (c) 2013 Vasili Baranau
// Distributed under the MIT software license
// See the accompanying file License.txt or http://opensource.org/licenses/MIT

#ifndef ImageProcessing_Services_Headers_WatershedComputerByEuclideanDistance_h
#define ImageProcessing_Services_Headers_WatershedComputerByEuclideanDistance_h

#include <map>
#include "Core/Headers/Macros.h"
#include "ImageProcessing/Model/Headers/Types.h"

namespace Services { class ActiveAreaComputer; }
namespace Services { class Serializer; }
namespace Model { class Config; }
namespace Model { struct ActiveArea; }
namespace Model { template<class TData> class DataArray; }

namespace Services
{
    class WatershedComputerByEuclideanDistance
    {
    // Classes
    private:
        struct Node;
        struct NeighborNode;

    // Constants
    public:
        static const Model::WatershedIterationType UNKNOWN_ITERATION;

    private:
        // Services
        const ActiveAreaComputer& activeAreaComputer;
        const Serializer& serializer;

        // Cache variables
        mutable Model::Config const * config;
        mutable Model::ActiveArea const * activeArea;
        mutable Model::DataArray<Model::EuclideanDistanceSquareType>* euclideanDistanceSquares;
        mutable Model::DataArray<Model::IsMaximumBallMaskType>* maximumBallsMask;
        mutable Model::DataArray<Model::PoreThroatIdType>* poreThroatIds;
        mutable Model::DataArray<Model::WatershedIterationType>* localWatershedIterations;

        mutable Model::PoreThroatIdType firstSharedId;
        mutable std::map<Model::PoreThroatIdType, Model::Pore> poresByIds;
        mutable std::map<Model::PoreThroatIdType, Model::SharedPixelType> sharedPixelTypesByIds;
        mutable std::map<std::vector<Model::PoreThroatIdType>, Model::PoreThroatIdType> sharedPixelTypeIdsBySharedPores;
        mutable size_t numberOfAssignedPixels;
        mutable Model::EuclideanDistanceType discreteMargin;

    public:
        WatershedComputerByEuclideanDistance(const ActiveAreaComputer& currentActiveAreaComputer, const Serializer& currentSerializer);

        void AssignPoreAndThroatIds(const Model::Config& config) const;

        virtual ~WatershedComputerByEuclideanDistance();

    private:
        void AssignPoreIds(const std::vector<Model::ActiveArea>& activeAreas) const;

        Model::PoreThroatIdType AssignPoreIdsInActiveArea(Model::PoreThroatIdType startingPoreId) const;

        void FillDescendingMaximumBalls(std::vector<Model::MaximumBall>* maximumBalls) const;

        Model::PoreThroatIdType AssignPoreIdsInActiveArea(const std::vector<Model::MaximumBall>& maximumBalls, Model::PoreThroatIdType startingPoreId) const;

        void AssignPoreIdRecursively(Model::Pore* pore) const;

        void RemoveEmptySharedPixelTypes() const;

        bool UpdatePixelAndSharedBalls(const NeighborNode& node, Model::Pore* pore) const;

        Model::SharedPixelType* GetOrAddSharedPixelType(const std::vector<Model::PoreThroatIdType>& sharedPoreIds) const;

        DISALLOW_COPY_AND_ASSIGN(WatershedComputerByEuclideanDistance);

    // Classes
    private:
        struct Node
        {
            int x;
            int y;
            int z;

            Node()
            {

            }

            Node(int currentX, int currentY, int currentZ) :
                x(currentX), y(currentY), z(currentZ)
            {

            }
        };

        struct NeighborNode : Node
        {
            Model::EuclideanDistanceSquareType depth;
            Model::EuclideanDistanceSquareType euclideanDistanceSquare;
            Model::EuclideanDistanceSquareType parentEuclideanDistanceSquare;
            Model::WatershedIterationType localIteration;
            Model::WatershedIterationType globalIteration;
            Model::PoreThroatIdType initialId;
            Model::PoreThroatIdType finalId;

            // To allow default copy constructor
            NeighborNode()
            {

            }

            NeighborNode(int currentX, int currentY, int currentZ) :
                Node(currentX, currentY, currentZ)
            {

            }
        };
    };
}

#endif /* ImageProcessing_Services_Headers_WatershedComputerByEuclideanDistance_h */

