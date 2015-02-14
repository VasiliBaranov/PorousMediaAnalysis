// Copyright (c) 2013 Vasili Baranau
// Distributed under the MIT software license
// See the accompanying file License.txt or http://opensource.org/licenses/MIT

#ifndef ImageProcessing_Services_Headers_ActiveAreaComputer_h
#define ImageProcessing_Services_Headers_ActiveAreaComputer_h

#include <vector>
#include "Core/Headers/Types.h"
#include "Core/Headers/Macros.h"
#include "ImageProcessing/Model/Headers/Types.h"

namespace Model { class Config; }

namespace Services
{
    class ActiveAreaComputer
    {
    public:
        ActiveAreaComputer();

        void ComputeActiveAreas(const Model::Config& config, const std::vector<Core::Axis::Type>& priorityAxes, Model::Margin margin,
                Core::FLOAT_TYPE averageBytesPerPixel, std::vector<Model::ActiveArea>* activeAreas) const;

        virtual ~ActiveAreaComputer();

    private:
        void FillPossiblePriorityAxes(const std::vector<Core::Axis::Type>& priorityAxes, std::vector<std::vector<Core::Axis::Type> >* possiblePriorityAxes) const;

        bool ComputeActiveAreasForExactPriorityAxes(const Model::Config& config, const std::vector<Core::Axis::Type>& priorityAxes, Model::Margin margin,
                Core::FLOAT_TYPE averageBytesPerPixel, std::vector<Model::ActiveArea>* activeAreas, Core::DiscreteSpatialVector* cellCounts) const;

        bool ComputeActiveAreasForExactPriorityAxes(const Model::Config& config, const std::vector<Core::Axis::Type>& priorityAxes, Model::Margin margin,
                const Core::SpatialVector& internalActiveAreaSize, std::vector<Model::ActiveArea>* activeAreas, Core::DiscreteSpatialVector* cellCounts) const;

        int GetImageReadsCount(const std::vector<Model::ActiveArea>& activeAreas) const;

        void CheckMemoryRequirements(const Model::Config& config, Core::FLOAT_TYPE averageBytesPerPixel, const std::vector<Model::ActiveArea>& activeAreas) const;

        void PrintStatistics(const Model::Config& config, Core::FLOAT_TYPE averageBytesPerPixel, const std::vector<Model::ActiveArea>& activeAreas,
                            Core::DiscreteSpatialVector& cellCounts, int imageReadsCount) const;

        void FillCellCounts(const Model::Config& config, const std::vector<Core::Axis::Type>& priorityAxes, Model::Margin margin,
                Core::SpatialVector internalActiveAreaSize, Core::DiscreteSpatialVector* cellCounts) const;

        void FillCorrectPrirityAxes(const std::vector<Core::Axis::Type>& priorityAxes, std::vector<Core::Axis::Type>* correctPriorityAxes) const;

        void FillNonPriorityAxes(const std::vector<Core::Axis::Type>& priorityAxes, std::vector<Core::Axis::Type>* nonPriorityAxes) const;

        Core::FLOAT_TYPE GetPixelsCountForNonPriorityAxes(const Model::Config& config, Core::FLOAT_TYPE averageBytesPerPixel, const std::vector<Core::Axis::Type>& priorityAxes) const;

        void FillInternalActiveAreaSize(const Model::Config& config, const std::vector<Core::Axis::Type>& priorityAxes, Model::Margin margin,
                Core::FLOAT_TYPE averageBytesPerPixel, Core::SpatialVector* internalActiveAreaSize) const;

        void FillInternalActiveAreaSizeForZeroMargin(const Model::Config& config, const std::vector<Core::Axis::Type>& priorityAxes,
                        Core::FLOAT_TYPE averageBytesPerPixel, Core::SpatialVector* internalActiveAreaSize) const;

        void FillActiveBox(const Model::Config& config, Model::Margin margin, const Core::DiscreteSpatialVector& internalActiveAreaSize, const Core::DiscreteSpatialVector& cellCounts,
                const Core::DiscreteSpatialVector& leftCornerIndexes, Model::Box* activeBox) const;

        void FillBoxWithMargins(const Model::Config& config, const Model::Box& activeBox, Model::Margin margin, Model::Box* boxWithMargins) const;

        DISALLOW_COPY_AND_ASSIGN(ActiveAreaComputer);
    };
}

#endif /* ImageProcessing_Services_Headers_ActiveAreaComputer_h */

