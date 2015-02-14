// Copyright (c) 2013 Vasili Baranau
// Distributed under the MIT software license
// See the accompanying file License.txt or http://opensource.org/licenses/MIT

#ifndef ImageProcessing_Services_Headers_OnePixelBoundaryThroatComputer_h
#define ImageProcessing_Services_Headers_OnePixelBoundaryThroatComputer_h

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
    class OnePixelBoundaryThroatComputer
    {
    // Classes
    private:
        struct NodeToVisit;
        struct PoreWithRadius;

    private:
        // Services
        const ActiveAreaComputer& activeAreaComputer;
        const Serializer& serializer;

        // Cache variables
        mutable Model::Config const * config;
        mutable Model::ActiveArea const * activeArea;
        mutable Model::DataArray<Model::EuclideanDistanceSquareType>* containingBallRadiiSquares;
        mutable Model::DataArray<Model::PoreThroatIdType>* oldPoreThroatIds;
        mutable Model::DataArray<Model::PoreThroatIdType>* poreThroatIds;

        mutable Model::PoreThroatIdType firstNewSharedId;
        mutable Model::PoreThroatIdType lastPoreId;
        mutable std::map<Model::PoreThroatIdType, PoreWithRadius> poresByIds;
        mutable std::map<Model::PoreThroatIdType, Model::SharedPixelType> oldSharedPixelTypesByIds;
        mutable std::map<Model::PoreThroatIdType, Model::SharedPixelType> sharedPixelTypesByIds;
        mutable std::map<std::vector<Model::PoreThroatIdType>, Model::PoreThroatIdType> sharedPixelTypeIdsBySharedPores;
        mutable size_t numberOfAssignedPixels;

    public:
        OnePixelBoundaryThroatComputer(const ActiveAreaComputer& currentActiveAreaComputer, const Serializer& currentSerializer);

        void AssignPoreAndThroatIds(const Model::Config& config) const;

        virtual ~OnePixelBoundaryThroatComputer();

    private:
        void FillPoresByIds() const;

        void FillOldSharedPixelTypesByIds() const;

        void AssignPoreIds(const std::vector<Model::ActiveArea>& activeAreas) const;

        void AssignPoreIdsInActiveArea() const;

        void AssignPoreIdRecursively(PoreWithRadius* pore) const;

        void RemoveEmptySharedPixelTypes() const;

        bool UpdatePixelAndSharedBalls(const NodeToVisit& node, Model::Pore* pore) const;

        Model::SharedPixelType* GetOrAddSharedPixelType(const std::vector<Model::PoreThroatIdType>& sharedPoreIds) const;

        DISALLOW_COPY_AND_ASSIGN(OnePixelBoundaryThroatComputer);

    // Classes
    private:
        struct NodeToVisit
        {
            int x;
            int y;
            int z;

            NodeToVisit(int currentX, int currentY, int currentZ) :
                x(currentX), y(currentY), z(currentZ)
            {

            }
        };

        struct PoreWithRadius : Model::Pore
        {
            Model::EuclideanDistanceSquareType seedRadius;
        };

        class PoreWithRadiusIdComparer
        {
        private:
            std::map<Model::PoreThroatIdType, PoreWithRadius>& poresByIds;

        public:
            PoreWithRadiusIdComparer(std::map<Model::PoreThroatIdType, PoreWithRadius>& currentPoresByIds) : poresByIds(currentPoresByIds)
            {
            };

            bool operator()(Model::PoreThroatIdType i, Model::PoreThroatIdType j)
            {
                PoreWithRadius& firstPore = poresByIds[i];
                PoreWithRadius& secondPore = poresByIds[j];
                return firstPore.seedRadius > secondPore.seedRadius; // descending order
            };
        };
    };
}

#endif /* ImageProcessing_Services_Headers_OnePixelBoundaryThroatComputer_h */

