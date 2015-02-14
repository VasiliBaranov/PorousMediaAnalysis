// Copyright (c) 2013 Vasili Baranau
// Distributed under the MIT software license
// See the accompanying file License.txt or http://opensource.org/licenses/MIT

#include "../Headers/BasePoreAndThroatComputer.h"

#include <string>
#include <cmath>
#include "stdio.h"

#include "Core/Headers/Path.h"
#include "ImageProcessing/Model/Headers/Config.h"
#include "ImageProcessing/Model/Headers/DataArray.h"
#include "ImageProcessing/Services/Headers/ActiveAreaComputer.h"

using namespace std;
using namespace Core;
using namespace Model;
using namespace Services;

namespace Services
{
    BasePoreAndThroatComputer::BasePoreAndThroatComputer(const ActiveAreaComputer& currentActiveAreaComputer, const Serializer& currentSerializer) :
        BaseDataArrayProcessor(currentActiveAreaComputer, currentSerializer)
    {
    }

    void BasePoreAndThroatComputer::ClearCache() const
    {
        firstSharedId = std::numeric_limits<PoreThroatIdType>::max();
        poresByIds.clear();
        sharedPixelTypesByIds.clear();
        sharedPixelTypeIdsBySharedPores.clear();
    }

    // Returns "shall continue recursion"
    bool BasePoreAndThroatComputer::UpdatePixelAndSharedBalls(const DiscreteSpatialVector& node, bool continueAfterSharedPixels, DataArray<PoreThroatIdType>* poreThroatIds, Pore* pore) const
    {
        PoreThroatIdType currentId = poreThroatIds->GetPixel(node);

        // ball is marked as the current pore -> do nothing, shall stop recursion
        if (currentId == pore->id)
        {
            return false;
        }

        // ball is marked as unknown -> update it, shall continue recursion
        else if (currentId == UNKNOWN_PORE_INDEX)
        {
            poreThroatIds->SetPixel(node, pore->id);
            pore->volume++;
            numberOfAssignedPixels++;
            return true;
        }

        // ball is marked as another pore -> add a throat candidate (set connected pore - the current pore and the other one), add throat seed, set pixel, shall continue recursion
        else if (currentId < firstSharedId)
        {
            vector<PoreThroatIdType> sharedPoreIds(2);
            sharedPoreIds[0] = pore->id;
            sharedPoreIds[1] = currentId;

            // get or create shared pixel type
            SharedPixelType* sharedPixelType = GetOrAddSharedPixelType(sharedPoreIds);

            // set the pixel
            poreThroatIds->SetPixel(node, sharedPixelType->id);

            // increment this pixel's volume
            sharedPixelType->volume++;

            // decrement the volume of the other pore
            Pore& currentPore = poresByIds[currentId];
            currentPore.volume--;

            // continue recursion
            bool shouldContinue = continueAfterSharedPixels ? true : false;
            return shouldContinue;
        }

        // ball is marked as shared -> add a candidate (if necessary), add pore to a candidate (if necessary), shall stop recursion
        else // (currentId >= firstSharedId)
        {
            SharedPixelType& sharedPixelType = sharedPixelTypesByIds[currentId];
            if (StlUtilities::Contains(sharedPixelType.poreIds, pore->id))
            {
                return false;
            }
            else
            {
                vector<PoreThroatIdType> sharedPoreIds = sharedPixelType.poreIds;
                sharedPoreIds.push_back(pore->id);
                // get or create shared pixel type for updated pores list
                SharedPixelType* newSharedPixelType = GetOrAddSharedPixelType(sharedPoreIds);

                // set pixel to this type
                poreThroatIds->SetPixel(node, newSharedPixelType->id);

                // decrement volume of the old shared type
                sharedPixelType.volume--;

                // increment volume of the current type
                newSharedPixelType->volume++;

                // continue recursion
                bool shouldContinue = continueAfterSharedPixels ? true : false;
                return shouldContinue;
            }
        }
    }

    SharedPixelType* BasePoreAndThroatComputer::GetOrAddSharedPixelType(const vector<PoreThroatIdType>& sharedPoreIds) const
    {
        map<vector<PoreThroatIdType>, PoreThroatIdType>::iterator it = sharedPixelTypeIdsBySharedPores.find(sharedPoreIds);
        if (it != sharedPixelTypeIdsBySharedPores.end())
        {
            PoreThroatIdType id = (*it).second;
            return &sharedPixelTypesByIds[id];
        }
        else
        {
            firstSharedId--;
            SharedPixelType& sharedPixelType = sharedPixelTypesByIds[firstSharedId];

            sharedPixelType.id = firstSharedId;
            sharedPixelType.volume = 0;
            sharedPixelType.poreIds = sharedPoreIds;

            sharedPixelTypeIdsBySharedPores[sharedPoreIds] = firstSharedId;

            return &sharedPixelType;
        }
    }

    BasePoreAndThroatComputer::~BasePoreAndThroatComputer()
    {
    }
}
