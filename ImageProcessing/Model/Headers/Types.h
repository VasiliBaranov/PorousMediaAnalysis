// Copyright (c) 2013 Vasili Baranau
// Distributed under the MIT software license
// See the accompanying file License.txt or http://opensource.org/licenses/MIT

#ifndef ImageProcessing_Model_Headers_Types_h
#define ImageProcessing_Model_Headers_Types_h

#include <vector>
#include "Core/Headers/Types.h"
#include "Core/Headers/Exceptions.h"

// The basic project structure is from the Domain-Driven Design: Model (merely data transfer objects) and Services (see Domain-Driven Design Quickly).
// Several model types are included in this file (Types.h) for simplicity.
namespace Model
{
    typedef char IsSolidMaskType;
    typedef unsigned int EuclideanDistanceSquareType;
    typedef char IsMaximumBallMaskType;
    typedef unsigned int PixelCoordinateType;
    typedef float EuclideanDistanceType;
    typedef unsigned int PoreThroatIdType;
    typedef unsigned int WatershedIterationType;

    typedef float ShortestPathDistanceType;
    typedef unsigned int CavityIdType;

    struct ExecutionMode
    {
        // Defines different execution modes for program operation.
        enum Type
        {
            Unknown = 0,
            All = 1,
            EuclideanDistanceTransform = 2,
            InnerBallsRemoval = 3,
            PoreAndThroatAssignment = 4,
            ShortestPathDistancesComputation = 5
        };
    };

    struct StencilType
    {
        enum Type
        {
            Unknown = 0,
            D3Q7 = 1,
            D3Q27 = 2
        };
    };

    struct DataArrayFlags
    {
        enum Type
        {
            Empty = 0,
            IsSolidMask = 1,
            EuclideanDistanceSquares = 2,
            MaximumBallsMask = 4,
            ContainingBallRadiiSquares = 8,
            ContainingBallCoordinatesX = 16,
            ContainingBallCoordinatesY = 32,
            ContainingBallCoordinatesZ = 64,
            PoreThroatIds = 128,
            PoreThroatIdsWatershed = 256,
            ShortestPathDistances = 512,
            CavityIds = 1024
        };
    };

    // See http://stackoverflow.com/questions/1448396/how-to-use-enums-as-flags-in-c
    // TODO: create macros for this
    inline DataArrayFlags::Type operator | (DataArrayFlags::Type a, DataArrayFlags::Type b) { return static_cast<DataArrayFlags::Type>(static_cast<int>(a) | static_cast<int>(b)); }
    inline DataArrayFlags::Type operator & (DataArrayFlags::Type a, DataArrayFlags::Type b) { return static_cast<DataArrayFlags::Type>(static_cast<int>(a) & static_cast<int>(b)); }

    struct Margin
    {
        enum Type
        {
            Pixels,
            Percentage
        };

        Margin::Type type;
        Core::FLOAT_TYPE value;

        Margin()
        {

        }

        Margin(Margin::Type currentType, Core::FLOAT_TYPE currentValue) : type(currentType), value(currentValue)
        {

        }
    };

    template<int dimensionality>
    struct GenericBox
    {
        boost::array<int, dimensionality> leftCorner;
        boost::array<int, dimensionality> exclusiveRightCorner;
        boost::array<int, dimensionality> boxSize;

        bool operator==(const GenericBox<dimensionality>& other) const
        {
            return leftCorner == other.leftCorner &&
                    exclusiveRightCorner == other.exclusiveRightCorner &&
                    boxSize == other.boxSize;
        }

        bool Contains(const boost::array<int, dimensionality>& point) const
        {
            for (int i = 0; i < dimensionality; ++i)
            {
                if ((point[i] < leftCorner[i]) || (point[i] >= exclusiveRightCorner[i]))
                {
                    return false;
                }
            }

            return true;
        }

        GenericBox()
        {
        }

        template<int otherDimensionality>
        explicit GenericBox(const GenericBox<otherDimensionality> other)
        {
            int minDim = std::min(dimensionality, otherDimensionality);
            for (int i = 0; i < minDim; ++i)
            {
                leftCorner[i] = other.leftCorner[i];
                exclusiveRightCorner[i] = other.exclusiveRightCorner[i];
                boxSize[i] = other.boxSize[i];
            }
        }
    };

    typedef GenericBox<DIMENSIONS> Box;

    typedef GenericBox<2> Rectangle;

    struct ActiveArea
    {
        Box activeBox;
        Box boxWithMargins;

        bool operator==(const ActiveArea& other) const
        {
            return activeBox == other.activeBox &&
                    boxWithMargins == other.boxWithMargins;
        }
    };

    struct Pore
    {
        PoreThroatIdType id;

        // The position of a maximum ball, from which all other pixels of the pore can be reached.
        Core::DiscreteSpatialVector seedPosition;

        EuclideanDistanceSquareType seedRadiusSquare;

        size_t volume;

        bool isBoundaryPore;
    };

    // "Type" here means "group", "class"
    struct SharedPixelType
    {
        PoreThroatIdType id;

        // Pores that are connected by the throat
        std::vector<PoreThroatIdType> poreIds;

        size_t volume;
    };

    struct MaximumBall
    {
        Core::DiscreteSpatialVector center;
        Model::EuclideanDistanceSquareType ballRadiusSquare;
    };

    class MaximumBallComparer
    {
    public:
        bool operator()(const Model::MaximumBall& x, const Model::MaximumBall& y)
        {
            return x.ballRadiusSquare > y.ballRadiusSquare;
        };
    };

    struct IntermediateStatistics
    {
        Model::EuclideanDistanceSquareType maxEuclideanDistanceSquare;
        size_t maximumBallsCount;
    };

    struct CavityDescription
    {
        CavityIdType id;
        Core::DiscreteSpatialVector seed;
        ShortestPathDistanceType euclideanDistanceToMonolithCenter;
        size_t voidPixelsCount;
        int reachedWallsCount;
        bool isReachingAllWalls;
    };
}

#endif /* ImageProcessing_Model_Headers_Types_h */

