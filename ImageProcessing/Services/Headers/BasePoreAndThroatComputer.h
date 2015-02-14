// Copyright (c) 2013 Vasili Baranau
// Distributed under the MIT software license
// See the accompanying file License.txt or http://opensource.org/licenses/MIT

#ifndef ImageProcessing_Services_Headers_BasePoreAndThroatComputer_h
#define ImageProcessing_Services_Headers_BasePoreAndThroatComputer_h

#include <map>
#include "ImageProcessing/Services/Headers/BaseDataArrayProcessor.h"

namespace Model { template<class T> class DataArray; }

namespace Services
{
    class BasePoreAndThroatComputer : public BaseDataArrayProcessor
    {
    protected:
        mutable Model::PoreThroatIdType firstSharedId;
        mutable std::map<Model::PoreThroatIdType, Model::Pore> poresByIds;
        mutable std::map<Model::PoreThroatIdType, Model::SharedPixelType> sharedPixelTypesByIds;
        mutable std::map<std::vector<Model::PoreThroatIdType>, Model::PoreThroatIdType> sharedPixelTypeIdsBySharedPores;
        mutable size_t numberOfAssignedPixels;

    public:
        BasePoreAndThroatComputer(const ActiveAreaComputer& currentActiveAreaComputer, const Serializer& currentSerializer);

        virtual ~BasePoreAndThroatComputer();

    protected:
        void ClearCache() const;

        bool UpdatePixelAndSharedBalls(const Core::DiscreteSpatialVector& node, bool continueAfterSharedPixels,
                Model::DataArray<Model::PoreThroatIdType>* poreThroatIds, Model::Pore* pore) const;

        Model::SharedPixelType* GetOrAddSharedPixelType(const std::vector<Model::PoreThroatIdType>& sharedPoreIds) const;

    private:
        DISALLOW_COPY_AND_ASSIGN(BasePoreAndThroatComputer);
    };
}

#endif /* ImageProcessing_Services_Headers_BasePoreAndThroatComputer_h */

