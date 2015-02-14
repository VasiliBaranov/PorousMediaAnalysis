// Copyright (c) 2013 Vasili Baranau
// Distributed under the MIT software license
// See the accompanying file License.txt or http://opensource.org/licenses/MIT

#include "../Headers/EmptyPoresRemover.h"

#include <string>
#include <cmath>
#include "stdio.h"
#include <map>
#include <stack>

#include "Core/Headers/Path.h"
#include "ImageProcessing/Model/Headers/Config.h"
#include "ImageProcessing/Services/Headers/ActiveAreaComputer.h"
#include "ImageProcessing/Services/Headers/Serializer.h"

using namespace std;
using namespace Core;
using namespace Model;
using namespace Services;

namespace Services
{
    EmptyPoresRemover::EmptyPoresRemover(const ActiveAreaComputer& currentActiveAreaComputer, const Serializer& currentSerializer) :
            BaseDataArrayProcessor(currentActiveAreaComputer, currentSerializer)
    {
        dataArrays.AddActiveDataArrayTypes(DataArrayFlags::PoreThroatIds);
    }

    void EmptyPoresRemover::RemoveEmptyPores(const Config& config) const
    {
        this->config = &config;
        vector<ActiveArea> activeAreas;

        Margin margin(Margin::Pixels, 0);

        vector<Axis::Type> priorityAxes(2);
        priorityAxes[0] = Axis::X;
        priorityAxes[1] = Axis::Y;
        activeAreaComputer.ComputeActiveAreas(config, priorityAxes, margin, dataArrays.GetBytesPerPixel(), &activeAreas);

        dataArrays.Initialize(config, activeAreas);

        newPoreThroatIdsByOldIds.clear();
        poresByIds.clear();
        sharedPixelTypesByIds.clear();
        RemoveEmptyPores(activeAreas);
    }

    void EmptyPoresRemover::RemoveEmptyPores(const vector<ActiveArea>& activeAreas) const
    {
        FillPixelMapping();
        if (newPoreThroatIdsByOldIds.size() == 0)
        {
            return;
        }

        printf("Assigning pore indexes in the entire image...\n");
        for (size_t activeAreaIndex = 0; activeAreaIndex < activeAreas.size(); ++activeAreaIndex)
        {
            printf("Active area " SIZE_T_FORMAT " / " SIZE_T_FORMAT "...\n", activeAreaIndex + 1, activeAreas.size());
            this->activeArea = &activeAreas[activeAreaIndex];

            dataArrays.ChangeActiveArea(activeAreaIndex);

            RemoveEmptyPoresInActiveArea();
        }

        dataArrays.WriteCurrentActiveArea();
        dataArrays.Clear();
    }

    void EmptyPoresRemover::RemoveEmptyPoresInActiveArea() const
    {
        for (int x = activeArea->activeBox.leftCorner[Axis::X]; x < activeArea->activeBox.exclusiveRightCorner[Axis::X]; ++x)
        {
            for (int y = activeArea->activeBox.leftCorner[Axis::Y]; y < activeArea->activeBox.exclusiveRightCorner[Axis::Y]; ++y)
            {
                for (int z = activeArea->activeBox.leftCorner[Axis::Z]; z < activeArea->activeBox.exclusiveRightCorner[Axis::Z]; ++z)
                {
                    PoreThroatIdType pixelValue = dataArrays.poreThroatIds.GetPixel(x, y, z);
                    if (pixelValue == UNKNOWN_PORE_INDEX) // solid
                    {
                        continue;
                    }

                    map<PoreThroatIdType, PoreThroatIdType>::iterator it = newPoreThroatIdsByOldIds.find(pixelValue);
                    if (it == newPoreThroatIdsByOldIds.end()) // pixel remains unchanged
                    {
                        continue;
                    }

                    PoreThroatIdType newValue = (*it).second;
                    dataArrays.poreThroatIds.SetPixel(x, y, z, newValue);
                }
            }
        }
    }

    void EmptyPoresRemover::FillPixelMapping() const
    {
        string poresFilePath = Path::Append(config->baseFolder, PORES_FILE_NAME);
        string sharedPixelTypesFilePath = Path::Append(config->baseFolder, SHARED_PIXEL_TYPES_FILE_NAME);

        // Read pores and shared pixel types
        serializer.ReadPores(poresFilePath, &poresByIds);
        serializer.ReadSharedPixelTypes(sharedPixelTypesFilePath, &sharedPixelTypesByIds);

        RemoveEmptyPoresFromIdMaps();

        RemoveSharedPixelTypesWithFewPores();

        RemoveDuplicateSharedPixelTypes();

        // Write new pore and throat maps
        serializer.WritePores(poresFilePath, poresByIds);
        serializer.WriteSharedPixelTypes(sharedPixelTypesFilePath, sharedPixelTypesByIds);

        for (map<PoreThroatIdType, PoreThroatIdType>::const_iterator it = newPoreThroatIdsByOldIds.begin(); it != newPoreThroatIdsByOldIds.end(); ++it)
        {
            printf("%lu %lu\n", static_cast<long unsigned int>((*it).first), static_cast<long unsigned int>((*it).second));
        }
    }

    void EmptyPoresRemover::RemoveDuplicateSharedPixelTypes() const
    {
        // Build a map from pore lists to shared pixels lists
        typedef PoreThroatIdType PoreIdType;
        typedef PoreThroatIdType ThroatIdType;
        map<vector<PoreIdType>, vector<ThroatIdType> > throatIdsBySharedPoreIds;

        for (map<PoreThroatIdType, SharedPixelType>::iterator it = sharedPixelTypesByIds.begin(); it != sharedPixelTypesByIds.end(); ++it)
        {
            SharedPixelType& sharedPixelType = (*it).second;
            vector<ThroatIdType>& throatIds = throatIdsBySharedPoreIds[sharedPixelType.poreIds];
            throatIds.push_back(sharedPixelType.id);
        }

        // Remove shared pixels that are duplicates, update the pixels map
        for (map<vector<PoreIdType>, vector<ThroatIdType> >::iterator it = throatIdsBySharedPoreIds.begin(); it != throatIdsBySharedPoreIds.end(); ++it)
        {
            vector<ThroatIdType>& throatIds = (*it).second;
            if (throatIds.size() == 1) // it can't actually be zero
            {
                continue;
            }

            if (throatIds.size() == 0)
            {
                throw InvalidOperationException("Number of pores belonging to this shared pixel type is zero after removing pores with zero volume. It shall never happen.");
            }

            ThroatIdType maxThroatId = StlUtilities::FindMaxElement(throatIds);
            for (size_t i = 0; i < throatIds.size(); ++i)
            {
                ThroatIdType currentId = throatIds[i];
                if (currentId != maxThroatId)
                {
                    sharedPixelTypesByIds.erase(currentId);
                    newPoreThroatIdsByOldIds[currentId] = maxThroatId;
                }
            }
        }
    }

    void EmptyPoresRemover::RemoveSharedPixelTypesWithFewPores() const
    {
        vector<PoreThroatIdType> sharedPixelTypeIdsWithNoPores;
        for (map<PoreThroatIdType, SharedPixelType>::iterator it = sharedPixelTypesByIds.begin(); it != sharedPixelTypesByIds.end(); ++it)
        {
            SharedPixelType& sharedPixelType = (*it).second;
            if (sharedPixelType.poreIds.size() < 2)
            {
                sharedPixelTypeIdsWithNoPores.push_back(sharedPixelType.id);
            }
        }

        for (size_t i = 0; i < sharedPixelTypeIdsWithNoPores.size(); ++i)
        {
            PoreThroatIdType id = sharedPixelTypeIdsWithNoPores[i];
            SharedPixelType& sharedPixelType = sharedPixelTypesByIds[id];

            if (sharedPixelType.poreIds.size() == 1)
            {
                newPoreThroatIdsByOldIds[sharedPixelType.id] = sharedPixelType.poreIds[0];
            }

            sharedPixelTypesByIds.erase(id);
        }
    }

    void EmptyPoresRemover::RemoveEmptyPoresFromIdMaps() const
    {
        vector<PoreThroatIdType> emptyPoreIds;
        for (map<PoreThroatIdType, Pore>::iterator it = poresByIds.begin(); it != poresByIds.end(); ++it)
        {
            const Pore& pore = (*it).second;
            if (pore.volume == 0)
            {
                emptyPoreIds.push_back(pore.id);
            }
        }

        printf("Empty pores id: ");
        for (size_t i = 0; i < emptyPoreIds.size(); ++i)
        {
            PoreThroatIdType id = emptyPoreIds[i];
            poresByIds.erase(id);
            printf("%u ", id);
        }
        printf("\n");

        // Remove pores from shared pixel types map
        StlUtilities::Sort(&emptyPoreIds);

        size_t maxSharedPoresCount = 0;
        for (map<PoreThroatIdType, SharedPixelType>::iterator it = sharedPixelTypesByIds.begin(); it != sharedPixelTypesByIds.end(); ++it)
        {
            SharedPixelType& sharedPixelType = (*it).second;
            if (sharedPixelType.poreIds.size() > maxSharedPoresCount)
            {
                maxSharedPoresCount = sharedPixelType.poreIds.size();
            }
            StlUtilities::Sort(&sharedPixelType.poreIds);
        }

        vector<PoreThroatIdType> currentNonEmptyPoreIds(maxSharedPoresCount);
        for (map<PoreThroatIdType, SharedPixelType>::iterator it = sharedPixelTypesByIds.begin(); it != sharedPixelTypesByIds.end(); ++it)
        {
            SharedPixelType& sharedPixelType = (*it).second;
            size_t oldSize = sharedPixelType.poreIds.size();
            vector<PoreThroatIdType>::iterator lastTempIterator = set_difference(sharedPixelType.poreIds.begin(), sharedPixelType.poreIds.end(),
                                                                    emptyPoreIds.begin(), emptyPoreIds.end(),
                                                                    currentNonEmptyPoreIds.begin());

            std::copy(currentNonEmptyPoreIds.begin(), lastTempIterator, sharedPixelType.poreIds.begin());
            sharedPixelType.poreIds.resize(lastTempIterator - currentNonEmptyPoreIds.begin());
            if (sharedPixelType.poreIds.size() < oldSize)
            {
                printf("Shared id %u; old size: " SIZE_T_FORMAT "; new size " SIZE_T_FORMAT "\n", sharedPixelType.id, oldSize, sharedPixelType.poreIds.size());
            }
        }
    }

    EmptyPoresRemover::~EmptyPoresRemover()
    {
    }
}
