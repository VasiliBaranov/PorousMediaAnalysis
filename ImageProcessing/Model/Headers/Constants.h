// Copyright (c) 2013 Vasili Baranau
// Distributed under the MIT software license
// See the accompanying file License.txt or http://opensource.org/licenses/MIT

#ifndef ImageProcessing_Constants_h
#define ImageProcessing_Constants_h

#include <string>
#include "Core/Headers/Constants.h"
#include "Types.h"

namespace Model
{
    // Files
    const std::string CONFIG_FILE_NAME = "config.txt";

    const std::string INTERMEDIATE_STATISTICS_FILE_NAME = "intermediateStatistics.txt";
    const std::string EUCLIDEAN_DISTANCE_SQUARE_COUNTS_FILE_NAME = "euclideanDistanceSquareCounts.txt";
    const std::string PORES_FILE_NAME = "pores.txt";
    const std::string SHARED_PIXEL_TYPES_FILE_NAME = "sharedPixelTypes.txt";

    const std::string WATERSHED_PORES_FILE_NAME = "poresWatershed.txt";
    const std::string WATERSHED_SHARED_PIXEL_TYPES_FILE_NAME = "sharedPixelTypesWatershed.txt";

    const std::string CAVITY_DESCRIPTIONS_FILE_NAME = "cavityDescriptions.txt";

    // Folders
    const std::string INITIAL_IMAGES_FOLDER_NAME = "Images";
    const std::string INITIAL_IMAGES_BEFORE_RESAMPLING_FOLDER_NAME = "Original";
    const std::string EUCLIDEAN_DISTANCE_SQUARES_FOLDER_NAME = "EuclideanDistanceSquares";
    const std::string CONTAINING_BALL_RADII_SQUARES_FOLDER_NAME = "ContainingBallRadiiSquares";
    const std::string CONTAINING_BALL_COORDINATES_X_FOLDER_NAME = "ContainingBallCoordinatesX";
    const std::string CONTAINING_BALL_COORDINATES_Y_FOLDER_NAME = "ContainingBallCoordinatesY";
    const std::string CONTAINING_BALL_COORDINATES_Z_FOLDER_NAME = "ContainingBallCoordinatesZ";
    const std::string IS_MAXIMUM_BALL_MASK_FOLDER_NAME = "IsMaximumBall";
    const std::string PORE_THROAT_IDS_FOLDER_NAME = "PoreThroatIds";
    const std::string WATERSHED_PORE_THROAT_IDS_FOLDER_NAME = "PoreThroatIdsWatershed";
    const std::string SHORTEST_PATH_DISTANCES_FOLDER_NAME = "ShortestPathDistances";
    const std::string CAVITY_IDS_FOLDER_NAME = "CavityIds";

    // Default values
    const IsMaximumBallMaskType INNER_BALL_MASK = 127;
    const IsMaximumBallMaskType OUTER_BALL_MASK= 255;
    const IsMaximumBallMaskType SOLID_BALL_MASK = 0;
    const IsMaximumBallMaskType UNKNOWN_BALL_MASK = 3;

    const PixelCoordinateType UNKNOWN_CONTAINING_BALL_COORDINATE = std::numeric_limits<PixelCoordinateType>::max();

    const PoreThroatIdType UNKNOWN_PORE_INDEX = 0;

    const ShortestPathDistanceType UNKNOWN_SHORTEST_PATH_DISTANCE = std::numeric_limits<ShortestPathDistanceType>::max();
    const ShortestPathDistanceType SOLID_SHORTEST_PATH_DISTANCE = -1;
    const CavityIdType UNKNOWN_CAVITY_ID = std::numeric_limits<CavityIdType>::max();
    const CavityIdType SOLID_CAVITY_ID = 0;
}

#endif /* ImageProcessing_Constants_h */

