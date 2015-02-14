// Copyright (c) 2013 Vasili Baranau
// Distributed under the MIT software license
// See the accompanying file License.txt or http://opensource.org/licenses/MIT

#include "../Headers/DataArraysGroup.h"

using namespace std;
using namespace Core;
using namespace Model;
using namespace Services;

namespace Model
{
    DataArraysGroup::DataArraysGroup(DataArrayFlags::Type activeDataArrayTypes)
    {
        this->activeDataArrayTypes = activeDataArrayTypes;
        initializeCalled = false;

        isSolidMask.defaultValue = 0;
        euclideanDistanceSquares.defaultValue = 0;
        maximumBallsMask.defaultValue = UNKNOWN_BALL_MASK;
        containingBallRadiiSquares.defaultValue = 0;
        containingBallCoordinatesX.defaultValue = UNKNOWN_CONTAINING_BALL_COORDINATE;
        containingBallCoordinatesY.defaultValue = UNKNOWN_CONTAINING_BALL_COORDINATE;
        containingBallCoordinatesZ.defaultValue = UNKNOWN_CONTAINING_BALL_COORDINATE;
        poreThroatIds.defaultValue = UNKNOWN_PORE_INDEX;
        poreThroatIdsWatershed.defaultValue = UNKNOWN_PORE_INDEX;
        shortestPathDistances.defaultValue = UNKNOWN_SHORTEST_PATH_DISTANCE;
        cavityIds.defaultValue = UNKNOWN_CAVITY_ID;

        folderNamesByDataArrayTypes[DataArrayFlags::IsSolidMask] = INITIAL_IMAGES_FOLDER_NAME;
        folderNamesByDataArrayTypes[DataArrayFlags::EuclideanDistanceSquares] = EUCLIDEAN_DISTANCE_SQUARES_FOLDER_NAME;
        folderNamesByDataArrayTypes[DataArrayFlags::MaximumBallsMask] = IS_MAXIMUM_BALL_MASK_FOLDER_NAME;
        folderNamesByDataArrayTypes[DataArrayFlags::ContainingBallRadiiSquares] = CONTAINING_BALL_RADII_SQUARES_FOLDER_NAME;
        folderNamesByDataArrayTypes[DataArrayFlags::ContainingBallCoordinatesX] = CONTAINING_BALL_COORDINATES_X_FOLDER_NAME;
        folderNamesByDataArrayTypes[DataArrayFlags::ContainingBallCoordinatesY] = CONTAINING_BALL_COORDINATES_Y_FOLDER_NAME;
        folderNamesByDataArrayTypes[DataArrayFlags::ContainingBallCoordinatesZ] = CONTAINING_BALL_COORDINATES_Z_FOLDER_NAME;
        folderNamesByDataArrayTypes[DataArrayFlags::PoreThroatIds] = PORE_THROAT_IDS_FOLDER_NAME;
        folderNamesByDataArrayTypes[DataArrayFlags::PoreThroatIdsWatershed] = WATERSHED_PORE_THROAT_IDS_FOLDER_NAME;
        folderNamesByDataArrayTypes[DataArrayFlags::ShortestPathDistances] = SHORTEST_PATH_DISTANCES_FOLDER_NAME;
        folderNamesByDataArrayTypes[DataArrayFlags::CavityIds] = CAVITY_IDS_FOLDER_NAME;

        dataArraysByDataArrayTypes[DataArrayFlags::IsSolidMask] = &isSolidMask;
        dataArraysByDataArrayTypes[DataArrayFlags::EuclideanDistanceSquares] = &euclideanDistanceSquares;
        dataArraysByDataArrayTypes[DataArrayFlags::MaximumBallsMask] = &maximumBallsMask;
        dataArraysByDataArrayTypes[DataArrayFlags::ContainingBallRadiiSquares] = &containingBallRadiiSquares;
        dataArraysByDataArrayTypes[DataArrayFlags::ContainingBallCoordinatesX] = &containingBallCoordinatesX;
        dataArraysByDataArrayTypes[DataArrayFlags::ContainingBallCoordinatesY] = &containingBallCoordinatesY;
        dataArraysByDataArrayTypes[DataArrayFlags::ContainingBallCoordinatesZ] = &containingBallCoordinatesZ;
        dataArraysByDataArrayTypes[DataArrayFlags::PoreThroatIds] = &poreThroatIds;
        dataArraysByDataArrayTypes[DataArrayFlags::PoreThroatIdsWatershed] = &poreThroatIdsWatershed;
        dataArraysByDataArrayTypes[DataArrayFlags::ShortestPathDistances] = &shortestPathDistances;
        dataArraysByDataArrayTypes[DataArrayFlags::CavityIds] = &cavityIds;
    }

    DataArrayFlags::Type DataArraysGroup::GetActiveDataArrayTypes() const
    {
        return activeDataArrayTypes;
    }

    void DataArraysGroup::SetActiveDataArrayTypes(DataArrayFlags::Type value)
    {
        if (initializeCalled)
        {
            throw InvalidOperationException("DataArraysGroup::Initialize has already been called");
        }

        activeDataArrayTypes = value;
    }

    void DataArraysGroup::AddActiveDataArrayTypes(DataArrayFlags::Type value)
    {
        if (initializeCalled)
        {
            throw InvalidOperationException("DataArraysGroup::Initialize has already been called");
        }

        activeDataArrayTypes = activeDataArrayTypes | value;
    }

    void DataArraysGroup::Initialize(const Config& config, const std::vector<ActiveArea>& activeAreas)
    {
        if (initializeCalled)
        {
            throw InvalidOperationException("DataArraysGroup::Initialize called twice");
        }

        for (map<DataArrayFlags::Type, IDataArray*>::iterator i = dataArraysByDataArrayTypes.begin(); i != dataArraysByDataArrayTypes.end(); ++i)
        {
            DataArrayFlags::Type dataArrayType = (*i).first;
            IDataArray* dataArray = (*i).second;

            if ((dataArrayType & activeDataArrayTypes) != 0)
            {
                string folderName = folderNamesByDataArrayTypes[dataArrayType];
                string folderPath = Path::Append(config.baseFolder, folderName);
                dataArray->Initialize(config, folderPath, activeAreas);
            }
        }
        initializeCalled = true;
    }

    void DataArraysGroup::Clear()
    {
        for (map<DataArrayFlags::Type, IDataArray*>::iterator i = dataArraysByDataArrayTypes.begin(); i != dataArraysByDataArrayTypes.end(); ++i)
        {
            DataArrayFlags::Type dataArrayType = (*i).first;
            IDataArray* dataArray = (*i).second;

            if ((dataArrayType & activeDataArrayTypes) != 0)
            {
                dataArray->Clear();
            }
        }
        initializeCalled = false;
    }

    void DataArraysGroup::ChangeActiveArea(int activeAreaIndex)
    {
        if (!initializeCalled)
        {
            throw InvalidOperationException("DataArraysGroup::Initialize has not yet been called");
        }

        for (map<DataArrayFlags::Type, IDataArray*>::iterator i = dataArraysByDataArrayTypes.begin(); i != dataArraysByDataArrayTypes.end(); ++i)
        {
            DataArrayFlags::Type dataArrayType = (*i).first;
            IDataArray* dataArray = (*i).second;

            if ((dataArrayType & activeDataArrayTypes) != 0)
            {
                dataArray->ChangeActiveArea(activeAreaIndex);
            }
        }
    }

    void DataArraysGroup::WriteCurrentActiveArea() const
    {
        if (!initializeCalled)
        {
            throw InvalidOperationException("DataArraysGroup::Initialize has not yet been called");
        }

        for (map<DataArrayFlags::Type, IDataArray*>::const_iterator i = dataArraysByDataArrayTypes.begin(); i != dataArraysByDataArrayTypes.end(); ++i)
        {
            DataArrayFlags::Type dataArrayType = (*i).first;
            const IDataArray* dataArray = (*i).second;

            if ((dataArrayType & activeDataArrayTypes) != 0)
            {
                dataArray->WriteCurrentActiveArea();
            }
        }
    }

    int DataArraysGroup::GetBytesPerPixel() const
    {
        int bytesPerPixel = 0;
        for (map<DataArrayFlags::Type, IDataArray*>::const_iterator i = dataArraysByDataArrayTypes.begin(); i != dataArraysByDataArrayTypes.end(); ++i)
        {
            DataArrayFlags::Type dataArrayType = (*i).first;
            const IDataArray* dataArray = (*i).second;

            if ((dataArrayType & activeDataArrayTypes) != 0)
            {
                bytesPerPixel += dataArray->GetBytesPerPixel();
            }
        }

        return bytesPerPixel;
    }
}
