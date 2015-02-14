// Copyright (c) 2013 Vasili Baranau
// Distributed under the MIT software license
// See the accompanying file License.txt or http://opensource.org/licenses/MIT

#ifndef ImageProcessing_Services_Headers_EmptyPoresRemover_h
#define ImageProcessing_Services_Headers_EmptyPoresRemover_h

#include <map>
#include "ImageProcessing/Services/Headers/BaseDataArrayProcessor.h"

namespace Services
{
    class EmptyPoresRemover : public BaseDataArrayProcessor
    {
    // Classes
    private:
        struct NodeToVisit;

    private:
        mutable std::map<Model::PoreThroatIdType, Model::PoreThroatIdType> newPoreThroatIdsByOldIds;
        mutable std::map<Model::PoreThroatIdType, Model::Pore> poresByIds;
        mutable std::map<Model::PoreThroatIdType, Model::SharedPixelType> sharedPixelTypesByIds;

    public:
        EmptyPoresRemover(const ActiveAreaComputer& currentActiveAreaComputer, const Serializer& currentSerializer);

        void RemoveEmptyPores(const Model::Config& config) const;

        virtual ~EmptyPoresRemover();

    private:
        void RemoveEmptyPores(const std::vector<Model::ActiveArea>& activeAreas) const;

        void RemoveEmptyPoresInActiveArea() const;

        void FillPixelMapping() const;

        void RemoveEmptyPoresFromIdMaps() const;

        void RemoveSharedPixelTypesWithFewPores() const;

        void RemoveDuplicateSharedPixelTypes() const;

        DISALLOW_COPY_AND_ASSIGN(EmptyPoresRemover);

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
    };
}

#endif /* ImageProcessing_Services_Headers_EmptyPoresRemover_h */

