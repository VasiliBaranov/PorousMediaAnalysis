// Copyright (c) 2013 Vasili Baranau
// Distributed under the MIT software license
// See the accompanying file License.txt or http://opensource.org/licenses/MIT

#ifndef ImageProcessing_Services_Headers_BaseDataArrayProcessor_h
#define ImageProcessing_Services_Headers_BaseDataArrayProcessor_h

#include <map>
#include "Core/Headers/Macros.h"
#include "ImageProcessing/Model/Headers/Types.h"
#include "ImageProcessing/Model/Headers/DataArraysGroup.h"

namespace Services { class ActiveAreaComputer; }
namespace Services { class Serializer; }
namespace Model { class Config; }

namespace Services
{
    class BaseDataArrayProcessor
    {
    protected:
        // Services
        const ActiveAreaComputer& activeAreaComputer;
        const Serializer& serializer;

        // Cache variables
        mutable Model::Config const * config;
        mutable Model::ActiveArea const * activeArea;

        mutable Model::DataArraysGroup dataArrays;

        mutable std::vector<Model::EuclideanDistanceType> euclideanDistancesByEuclideanDistanceSquares;
        mutable std::vector<Model::EuclideanDistanceSquareType> ballLowerRadiiSquaresByEuclideanDistanceSquares;
        mutable std::vector<Model::EuclideanDistanceType> ballLowerRadiiByEuclideanDistanceSquares;

    private:
        // Utility variables
        mutable Core::Axis::Type axisForIteration;
        mutable int layersCompletedByMaster;
        mutable Core::FLOAT_TYPE previouslyPrintedPercentage;

    protected:
        BaseDataArrayProcessor(const ActiveAreaComputer& currentActiveAreaComputer, const Serializer& currentSerializer);

        virtual ~BaseDataArrayProcessor();

    protected:
        void InitializeEuclideanDistanceDerivatives(Model::EuclideanDistanceSquareType maxNeededEuclideanDistanceSquare) const;

        Model::IntermediateStatistics ReadIntermediateStatistics() const;

        void WriteIntermediateStatistics(const Model::IntermediateStatistics& intermediateStatistics) const;

        void ResetPercentagePrinting(Core::Axis::Type axisForIteration) const;

        void PrintPercentageCompleted() const;

    private:
        DISALLOW_COPY_AND_ASSIGN(BaseDataArrayProcessor);
    };
}

#endif /* ImageProcessing_Services_Headers_BaseDataArrayProcessor_h */

