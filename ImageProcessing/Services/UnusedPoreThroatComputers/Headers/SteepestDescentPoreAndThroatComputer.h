// Copyright (c) 2013 Vasili Baranau
// Distributed under the MIT software license
// See the accompanying file License.txt or http://opensource.org/licenses/MIT

#ifndef ImageProcessing_Services_Headers_SteepestDescentPoreAndThroatComputer_h
#define ImageProcessing_Services_Headers_SteepestDescentPoreAndThroatComputer_h

#include <map>
#include <queue>
#include "Core/Headers/Macros.h"
#include "ImageProcessing/Model/Headers/Types.h"

namespace Services { class ActiveAreaComputer; }
namespace Services { class Serializer; }
namespace Model { class Config; }
namespace Model { struct ActiveArea; }
namespace Model { template<class TData> class DataArray; }

namespace Services
{
    class SteepestDescentPoreAndThroatComputer
    {
    // Classes
    private:
        struct Pixel;
        struct Plateau;

    // Constants
    public:
        static const Model::PoreThroatIdType UNKNOWN_INDEX;

    private:
        // Services
        const ActiveAreaComputer& activeAreaComputer;
        const Serializer& serializer;

        mutable int pixelsToMaskCenter;
        mutable bool*** isProcessedMask;

        // Cache variables
        mutable Model::Config const * config;
        mutable Model::ActiveArea const * activeArea;
        mutable Model::DataArray<Model::IsMaximumBallMaskType>* maximumBallsMask;
        mutable Model::DataArray<Model::EuclideanDistanceSquareType>* containingBallRadiiSquares;
        mutable Model::DataArray<Model::PoreThroatIdType>* poreThroatIds;

        mutable Model::PoreThroatIdType firstSharedId;
        mutable std::map<Model::PoreThroatIdType, Model::Pore> poresByIds;
        mutable std::map<Model::PoreThroatIdType, Model::SharedPixelType> sharedPixelTypesByIds;
        mutable std::map<std::vector<Model::PoreThroatIdType>, Model::PoreThroatIdType> sharedPixelTypeIdsBySharedPores;
        mutable size_t numberOfAssignedPixels;

    public:
        SteepestDescentPoreAndThroatComputer(const ActiveAreaComputer& currentActiveAreaComputer, const Serializer& currentSerializer);

        void AssignPoreAndThroatIds(const Model::Config& config) const;

        virtual ~SteepestDescentPoreAndThroatComputer();

    private:
        // AssignPoreIds
        void AssignPoreIds(const std::vector<Model::ActiveArea>& activeAreas) const;

        Model::PoreThroatIdType AssignPoreIdsInActiveArea(Model::PoreThroatIdType startingPoreId) const;

        Model::PoreThroatIdType AssignPoreIdsInActiveArea(const std::vector<Model::MaximumBall>& maximumBalls, Model::PoreThroatIdType startingPoreId) const;

        void AssignPoreIdRecursively(Model::Pore* pore) const;

        void FillPlateauAndMarkPixelsProcessed(const Model::Pore& pore, Plateau* plateau) const;

        void SetPoreInPlateauAndNeighbors(const Plateau& plateau, Model::Pore* pore) const;

        void AddPlateauNeighborsToQueue(const Plateau& plateau, const Model::Pore& pore, std::vector< std::queue<Pixel> >* nodesToVisitByDepth) const;

        bool HasSteepestDescentNeighborOutsidePore(const Plateau& plateau, const Model::Pore& pore) const;

        bool PixelBelongsToPore(const Core::DiscreteSpatialVector& position, const Model::Pore& pore) const;

        void RemoveEmptySharedPixelTypes() const;

        bool UpdatePixelAndSharedBalls(const Core::DiscreteSpatialVector& position, Model::Pore* pore) const;

        void FillDescendingMaximumBalls(std::vector<Model::MaximumBall>* maximumBalls) const;

        Model::SharedPixelType* GetOrAddSharedPixelType(const std::vector<Model::PoreThroatIdType>& sharedPoreIds) const;

        DISALLOW_COPY_AND_ASSIGN(SteepestDescentPoreAndThroatComputer);

    // Classes
    private:
        struct Plateau
        {
            Core::DiscreteSpatialVector seed;
            Model::EuclideanDistanceSquareType ballRadiusSquare;
            Model::EuclideanDistanceSquareType parentBallRadiusSquare;
            std::vector<Core::DiscreteSpatialVector> plateauPixels;
            std::vector<Pixel> neighbors;
        };

        struct Pixel
        {
            Core::DiscreteSpatialVector position;
            Model::EuclideanDistanceSquareType ballRadiusSquare;
            Model::EuclideanDistanceSquareType parentBallRadiusSquare;

            bool operator<(const Pixel& other) const
            {
                return position < other.position;
            }

            bool operator==(const Pixel& other) const
            {
                return position == other.position;
            }
        };
    };
}

#endif /* ImageProcessing_Services_Headers_SteepestDescentPoreAndThroatComputer_h */

