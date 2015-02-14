// Copyright (c) 2013 Vasili Baranau
// Distributed under the MIT software license
// See the accompanying file License.txt or http://opensource.org/licenses/MIT

#ifndef ImageProcessing_Model_Headers_DataArraysGroup_h
#define ImageProcessing_Model_Headers_DataArraysGroup_h

#include <map>
#include "ImageProcessing/Model/Headers/Types.h"
#include "ImageProcessing/Model/Headers/DataArray.h"

namespace Model
{
    class DataArraysGroup
    {
    public:
        DataArray<IsSolidMaskType> isSolidMask;
        DataArray<EuclideanDistanceSquareType> euclideanDistanceSquares;
        DataArray<IsMaximumBallMaskType> maximumBallsMask;

        DataArray<EuclideanDistanceSquareType> containingBallRadiiSquares;
        DataArray<PixelCoordinateType> containingBallCoordinatesX;
        DataArray<PixelCoordinateType> containingBallCoordinatesY;
        DataArray<PixelCoordinateType> containingBallCoordinatesZ;

        DataArray<PoreThroatIdType> poreThroatIds;
        DataArray<PoreThroatIdType> poreThroatIdsWatershed;

        DataArray<ShortestPathDistanceType> shortestPathDistances;
        DataArray<CavityIdType> cavityIds;

    private:
        DataArrayFlags::Type activeDataArrayTypes;
        bool initializeCalled;

        std::map<DataArrayFlags::Type, std::string> folderNamesByDataArrayTypes;
        std::map<DataArrayFlags::Type, IDataArray*> dataArraysByDataArrayTypes;

    public:
        explicit DataArraysGroup(DataArrayFlags::Type activeDataArrayTypes = DataArrayFlags::Empty);

        DataArrayFlags::Type GetActiveDataArrayTypes() const;

        void SetActiveDataArrayTypes(DataArrayFlags::Type value);

        void AddActiveDataArrayTypes(DataArrayFlags::Type value);

        void Initialize(const Config& config, const std::vector<ActiveArea>& activeAreas);

        // Saves the current active area to disk, loads the new active area from disk. If there are no images for the new active area, initializes the missing values with zeros
        void ChangeActiveArea(int activeAreaIndex);

        // Writes current active area to disk
        void WriteCurrentActiveArea() const;

        int GetBytesPerPixel() const;

        void Clear();

    private:
        DISALLOW_COPY_AND_ASSIGN(DataArraysGroup);
    };
}

#endif /* ImageProcessing_Model_Headers_DataArraysGroup_h */

