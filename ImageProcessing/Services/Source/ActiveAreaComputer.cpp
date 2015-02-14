// Copyright (c) 2013 Vasili Baranau
// Distributed under the MIT software license
// See the accompanying file License.txt or http://opensource.org/licenses/MIT

#include <limits>
#include <cstdio>
#include "../Headers/ActiveAreaComputer.h"
#include "ImageProcessing/Model/Headers/Config.h"
#include "ImageProcessing/Model/Headers/ModelUtilities.h"
#include "Core/Headers/VectorUtilities.h"
#include "Core/Headers/StlUtilities.h"

using namespace std;
using namespace Core;
using namespace Model;

namespace Services
{
    ActiveAreaComputer::ActiveAreaComputer()
    {

    }

    void ActiveAreaComputer::ComputeActiveAreas(const Config& config, const vector<Axis::Type>& priorityAxes, Margin margin, FLOAT_TYPE averageBytesPerPixel, vector<ActiveArea>* activeAreas) const
    {
        if (margin.type == Margin::Percentage)
        {
            throw InvalidOperationException("Percentage margins are not currently supported");
        }

        FLOAT_TYPE optimalMemoryInMegabytes = ModelUtilities::GetPixelsCount(config) / 1024.0 / 1024.0 * averageBytesPerPixel;
        printf("Current available memory in Mb is %d, optimal is %f\n", config.availableMemoryInMegabytes, optimalMemoryInMegabytes);

        vector<vector<Axis::Type> > possiblePriorityAxes;
        FillPossiblePriorityAxes(priorityAxes, &possiblePriorityAxes);

        // For each of the possible priority axes, if they conform to the specified axes,
        // compute active areas. Determine the set with min number of reads.

        // Actually, this loop is needed to easily consider the case when there can be just one cell in a certain non-priority direction.
        // It is hard to handle all the possibilities explicitly in the code, so it's a quick and dirty solution.
        // If you would like to handle them explicitly and remove this loop, you shall update the code after cell counts are computed, and they are <= 2 in certain directions.
        int imageReadsCount = numeric_limits<int>::max();
        size_t activeAreasCount = numeric_limits<size_t>::max();
        vector<ActiveArea>& activeAreasRef = *activeAreas;
        DiscreteSpatialVector cellCounts;
        bool haveValidActiveAreas = false;
        for (size_t i = 0; i < possiblePriorityAxes.size(); ++i)
        {
            vector<Axis::Type>& currentPriorityAxes = possiblePriorityAxes[i];
            vector<ActiveArea> currentActiveAreas;
            DiscreteSpatialVector currentCellCounts;

            bool success = ComputeActiveAreasForExactPriorityAxes(config, currentPriorityAxes, margin, averageBytesPerPixel, &currentActiveAreas, &currentCellCounts);
            if (success)
            {
                haveValidActiveAreas = true;
            }
            else
            {
                continue;
            }

            int currentImageReadsCount = GetImageReadsCount(currentActiveAreas);
            bool axesSetIsBetter = (currentImageReadsCount < imageReadsCount) ||
                    ((currentImageReadsCount == imageReadsCount) && (currentActiveAreas.size() < activeAreasCount));
            if (axesSetIsBetter)
            {
                activeAreasRef.resize(currentActiveAreas.size());
                StlUtilities::Copy(currentActiveAreas, activeAreas);
                imageReadsCount = currentImageReadsCount;
                activeAreasCount = currentActiveAreas.size();
                cellCounts = currentCellCounts;

                if (imageReadsCount == 1)
                {
                    break;
                }
            }
        }

        if (!haveValidActiveAreas)
        {
            // TODO: push min required memory up to here
            throw InvalidOperationException("Memory requirements for active areas are too strict. There is not enough memory even for 1-pixel active area with correct margins and priority axes.");
        }

        PrintStatistics(config, averageBytesPerPixel, activeAreasRef, cellCounts, imageReadsCount);

        // Check if conform to memory requirements and all the areas consume equal amount of space
        CheckMemoryRequirements(config, averageBytesPerPixel, activeAreasRef);
    }

    void ActiveAreaComputer::PrintStatistics(const Config& config, FLOAT_TYPE averageBytesPerPixel, const vector<ActiveArea>& activeAreas,
            DiscreteSpatialVector& cellCounts, int imageReadsCount) const
    {
        vector<vector<int> > activeAreaSizesByDimension(3);
        for (size_t i = 0; i < activeAreas.size(); ++i)
        {
            const ActiveArea& activeArea = activeAreas[i];
            for (int dim = 0; dim < 3; ++dim)
            {
                activeAreaSizesByDimension[dim].push_back(activeArea.boxWithMargins.boxSize[dim]);
            }
        }

        SpatialVector averageBoxSize;
        for (int dim = 0; dim < 3; ++dim)
        {
            averageBoxSize[dim] = VectorUtilities::GetMean(activeAreaSizesByDimension[dim]);
        }

        printf("Number of active areas by each dimension is %d %d %d\n", cellCounts[Axis::X], cellCounts[Axis::Y], cellCounts[Axis::Z]);
        printf("Total number of image reads is %d\n", imageReadsCount);
    }

    void ActiveAreaComputer::CheckMemoryRequirements(const Config& config, FLOAT_TYPE averageBytesPerPixel, const vector<ActiveArea>& activeAreas) const
    {
        vector<FLOAT_TYPE> megabytesPerActiveArea(activeAreas.size());

        for (size_t i = 0; i < activeAreas.size(); ++i)
        {
            DiscreteSpatialVector activeAreaBoxSize = activeAreas[i].boxWithMargins.boxSize;
            SpatialVector activeAreaBoxSizeFloat;
            VectorUtilities::Convert(activeAreaBoxSize, &activeAreaBoxSizeFloat);
            megabytesPerActiveArea[i] = VectorUtilities::GetProduct(activeAreaBoxSize) * averageBytesPerPixel / 1024.0 / 1024.0;
        }

        FLOAT_TYPE megabytesInActiveAreaMean = VectorUtilities::GetMean(megabytesPerActiveArea);
        FLOAT_TYPE megabytesInActiveAreaStd = VectorUtilities::GetStd(megabytesPerActiveArea);
        FLOAT_TYPE maxMegabytesInActiveArea = StlUtilities::FindMaxElement(megabytesPerActiveArea);
        FLOAT_TYPE minMegabytesInActiveArea = StlUtilities::FindMinElement(megabytesPerActiveArea);

        const FLOAT_TYPE maxDifferenceInMegabytes = 50;
        bool maxMemoryIsHigh = maxMegabytesInActiveArea > config.availableMemoryInMegabytes + maxDifferenceInMegabytes;
        bool minMemoryIsLow = minMegabytesInActiveArea < config.availableMemoryInMegabytes - maxDifferenceInMegabytes;
        bool minMemoryIsLowAndWrong = minMemoryIsLow && (activeAreas.size() > 1); // If there is just one active area, no need to complain that require less memory

        if (maxMemoryIsHigh || minMemoryIsLowAndWrong)
        {
            if (maxMemoryIsHigh && minMemoryIsLowAndWrong)
            {
                printf("WARNING: Max and min memory per active area differ too much from the expected value\n");
            }
            else if (maxMemoryIsHigh)
            {
                printf("WARNING: Max memory per active area differs too much from the expected value\n");
            }
            else
            {
                printf("WARNING: Min memory per active area differs too much from the expected value\n");
            }

            printf("Max memory per active area specified is %dMb. Actual memory per active area: mean %fMb, std %fMb, max %fMb, min %fMb\n",
                    config.availableMemoryInMegabytes, megabytesInActiveAreaMean, megabytesInActiveAreaStd, maxMegabytesInActiveArea, minMegabytesInActiveArea);
        }
    }

    int ActiveAreaComputer::GetImageReadsCount(const vector<ActiveArea>& activeAreas) const
    {
        int imageReadsCount = 0;
        for (size_t i = 0; i < activeAreas.size(); ++i)
        {
            imageReadsCount += activeAreas[i].boxWithMargins.boxSize[Axis::Z];
        }

        return imageReadsCount;
    }

    void ActiveAreaComputer::FillPossiblePriorityAxes(const vector<Axis::Type>& priorityAxes, vector<vector<Axis::Type> >* possiblePriorityAxes) const
    {
        possiblePriorityAxes->clear();

        vector<Axis::Type> correctPriorityAxes(priorityAxes);
        FillCorrectPrirityAxes(priorityAxes, &correctPriorityAxes);

        boost::array<int, 3> axisMasks = {{1, 2, 4}}; // int values for bit masks {{100, 010, 001}}

        // Iterate through all combinations of axes. I.e., through all combinations of bit masks above.
        // Do not include i = 7 = 1 + 2 + 4 = 111, as three priority axes are equivalent to no priority axes.
        for (int axesCombination = 0; axesCombination < 8; ++axesCombination)
        {
            vector<Axis::Type> currentAxes;
            for (int axisIndex = 0; axisIndex < 3; ++axisIndex)
            {
                bool axisIsPriority = (axesCombination & axisMasks[axisIndex]) != 0;
                if (axisIsPriority)
                {
                    Axis::Type axis = static_cast<Axis::Type>(axisIndex);
                    currentAxes.push_back(axis);
                }
            }

            bool isCorrect = true;
            for (size_t i = 0; i < correctPriorityAxes.size(); ++i)
            {
                if (!StlUtilities::Contains(currentAxes, correctPriorityAxes[i]))
                {
                    isCorrect = false;
                    break;
                }
            }

            if (isCorrect)
            {
                possiblePriorityAxes->push_back(currentAxes);
            }
        }
    }

    void ActiveAreaComputer::FillCellCounts(const Config& config, const vector<Axis::Type>& priorityAxes, Margin margin,
            SpatialVector internalActiveAreaSize, DiscreteSpatialVector* cellCounts) const
    {
        SpatialVector cellCountsFloat;
        SpatialVector imageSizeFloat;
        StlUtilities::Copy(config.imageSize, &imageSizeFloat);
        VectorUtilities::SubtractValue(imageSizeFloat, 2.0 * margin.value, &imageSizeFloat);
        VectorUtilities::Divide(imageSizeFloat, internalActiveAreaSize, &cellCountsFloat);

        DiscreteSpatialVector& cellCountsRef = *cellCounts;
        VectorUtilities::Ceil(cellCountsFloat, cellCounts);
//        VectorUtilities::Round(cellCountsFloat, cellCounts);

        for (size_t i = 0; i < priorityAxes.size(); ++i)
        {
            Axis::Type priorityAxis = priorityAxes[i];
            cellCountsRef[priorityAxis] = 1;
        }
    }

    bool ActiveAreaComputer::ComputeActiveAreasForExactPriorityAxes(const Config& config, const vector<Axis::Type>& priorityAxes, Margin margin,
            FLOAT_TYPE averageBytesPerPixel, vector<ActiveArea>* activeAreas, DiscreteSpatialVector* cellCounts) const
    {
        SpatialVector internalActiveAreaSizeFloat;
        FillInternalActiveAreaSize(config, priorityAxes, margin, averageBytesPerPixel, &internalActiveAreaSizeFloat);

        FLOAT_TYPE minInternalCellSize = StlUtilities::FindMinElement(internalActiveAreaSizeFloat);
        if (minInternalCellSize < 1.0)
        {
            return false;
        }

        // Check memory of the active area. If priority axes were too strict,
        // the optimization procedure will still return some result, but the memory consumed will be too large.
        // If this check is not done, the best answer for free Z is always with X and Y priority axes.
        FLOAT_TYPE activeAreaPixelsFloat = 1.0;
        for (int i = 0; i < 3; ++i)
        {
            if (StlUtilities::Contains(priorityAxes, (Axis::Type)i))
            {
                activeAreaPixelsFloat *= config.imageSize[i];
            }
            else
            {
                activeAreaPixelsFloat *= (internalActiveAreaSizeFloat[i] + 2.0 * margin.value);
            }
        }

        FLOAT_TYPE activeAreaMemoryFloat = activeAreaPixelsFloat * averageBytesPerPixel / 1024.0 / 1024.0;
        if (activeAreaMemoryFloat > config.availableMemoryInMegabytes * 1.01)
        {
            return false;
        }

        bool success = ComputeActiveAreasForExactPriorityAxes(config, priorityAxes, margin, internalActiveAreaSizeFloat, activeAreas, cellCounts);
        return success;
    }

    bool ActiveAreaComputer::ComputeActiveAreasForExactPriorityAxes(const Config& config, const vector<Axis::Type>& priorityAxes, Margin margin,
            const SpatialVector& internalActiveAreaSizeFloat, vector<ActiveArea>* activeAreas, DiscreteSpatialVector* cellCounts) const
    {
        DiscreteSpatialVector& cellCountsRef = *cellCounts;
        FillCellCounts(config, priorityAxes, margin, internalActiveAreaSizeFloat, cellCounts);
        if (StlUtilities::FindMinElement(cellCountsRef) <= 0)
        {
            return false;
        }

        SpatialVector internalActiveAreaSizeFromCellsCountFloat;
        SpatialVector imageSizeFloat;
        StlUtilities::Copy(config.imageSize, &imageSizeFloat);
        VectorUtilities::SubtractValue(imageSizeFloat, 2.0 * margin.value, &imageSizeFloat);
        VectorUtilities::Divide(imageSizeFloat, cellCountsRef, &internalActiveAreaSizeFromCellsCountFloat);
        DiscreteSpatialVector internalActiveAreaSize;
        VectorUtilities::Floor(internalActiveAreaSizeFromCellsCountFloat, &internalActiveAreaSize);
//        VectorUtilities::Round(internalActiveAreaSizeFromCellsCountFloat, &internalActiveAreaSize);

        // Split full image into active areas, the last ones being maybe slightly different from others

        // Fill active areas list, adding margins, where necessary
        vector<ActiveArea>& activeAreasRef = *activeAreas;
        activeAreasRef.resize(VectorUtilities::GetProduct(cellCountsRef));

        // TODO: make dimension-agnostic, like CellListNeighborProvider
        int activeAreaIndex = 0;
        for (int i = 0; i < cellCountsRef[Axis::X]; ++i)
        {
            for (int j = 0; j < cellCountsRef[Axis::Y]; ++j)
            {
                for (int k = 0; k < cellCountsRef[Axis::Z]; ++k)
                {
                    ActiveArea& activeArea = activeAreasRef[activeAreaIndex];

                    DiscreteSpatialVector leftCornerIndexes = {{i, j, k}};
                    FillActiveBox(config, margin, internalActiveAreaSize, cellCountsRef, leftCornerIndexes, &activeArea.activeBox);
                    FillBoxWithMargins(config, activeArea.activeBox, margin, &activeArea.boxWithMargins);

                    activeAreaIndex++;
                }
            }
        }

        return true;
    }

    void ActiveAreaComputer::FillActiveBox(const Config& config, Margin margin, const DiscreteSpatialVector& internalActiveAreaSize, const DiscreteSpatialVector& cellCounts,
            const DiscreteSpatialVector& leftCornerIndexes, Model::Box* activeBox) const
    {
        Box& activeBoxRef = *activeBox;
        // TODO: may vectorize
        for (int dim = 0; dim < DIMENSIONS; ++dim)
        {
            int leftCornerIndex = leftCornerIndexes[dim];
            activeBoxRef.leftCorner[dim] = leftCornerIndex * internalActiveAreaSize[dim];
            if (leftCornerIndex > 0)
            {
                activeBoxRef.leftCorner[dim] += margin.value; // because boundary boxes are larger by margin (to consume as much memory as internal boxes)
            }

            if (leftCornerIndex < cellCounts[dim] - 1)
            {
                activeBoxRef.exclusiveRightCorner[dim] = activeBoxRef.leftCorner[dim] + internalActiveAreaSize[dim];
                if (leftCornerIndex == 0)
                {
                    activeBoxRef.exclusiveRightCorner[dim] += margin.value;
                }
            }
            else
            {
                activeBoxRef.exclusiveRightCorner[dim] = config.imageSize[dim];
            }

            activeBoxRef.boxSize[dim] = activeBoxRef.exclusiveRightCorner[dim] - activeBoxRef.leftCorner[dim];
        }
    }

    void ActiveAreaComputer::FillBoxWithMargins(const Config& config, const Box& activeBox, Margin margin, Box* boxWithMargins) const
    {
        Box& boxWithMarginsRef = *boxWithMargins;
        VectorUtilities::SubtractValue(activeBox.leftCorner, margin.value, &boxWithMarginsRef.leftCorner);

        DiscreteSpatialVector zeroPoint;
        VectorUtilities::InitializeWith(&zeroPoint, 0);
        VectorUtilities::Max(boxWithMarginsRef.leftCorner, zeroPoint, &boxWithMarginsRef.leftCorner);

        VectorUtilities::AddValue(activeBox.exclusiveRightCorner, margin.value, &boxWithMarginsRef.exclusiveRightCorner);
        VectorUtilities::Min(boxWithMarginsRef.exclusiveRightCorner, config.imageSize, &boxWithMarginsRef.exclusiveRightCorner);

        VectorUtilities::Subtract(boxWithMarginsRef.exclusiveRightCorner, boxWithMarginsRef.leftCorner, &boxWithMarginsRef.boxSize);
    }

    FLOAT_TYPE ActiveAreaComputer::GetPixelsCountForNonPriorityAxes(const Config& config, FLOAT_TYPE averageBytesPerPixel, const vector<Axis::Type>& priorityAxes) const
    {
        FLOAT_TYPE maxMemorySizeInBytes = static_cast<FLOAT_TYPE>(config.availableMemoryInMegabytes) * 1024 * 1024; // cast to float to avoid overflow
        FLOAT_TYPE pixelsCountForCell = maxMemorySizeInBytes / averageBytesPerPixel;
        FLOAT_TYPE pixelsCountForNonPriorityAxes = pixelsCountForCell;
        for (size_t i = 0; i < priorityAxes.size(); ++i)
        {
            Axis::Type priorityAxis = priorityAxes[i];
            pixelsCountForNonPriorityAxes /= config.imageSize[priorityAxis];
        }

        return pixelsCountForNonPriorityAxes;
    }

    void ActiveAreaComputer::FillInternalActiveAreaSize(const Model::Config& config, const std::vector<Core::Axis::Type>& priorityAxes, Margin margin,
            FLOAT_TYPE averageBytesPerPixel, SpatialVector* internalActiveAreaSize) const
    {
        if (margin.value == 0)
        {
            FillInternalActiveAreaSizeForZeroMargin(config, priorityAxes, averageBytesPerPixel, internalActiveAreaSize);
            return;
        }

        SpatialVector& internalActiveAreaSizeRef = *internalActiveAreaSize;

        vector<Axis::Type> nonPriorityAxes;
        FillNonPriorityAxes(priorityAxes, &nonPriorityAxes);
        FLOAT_TYPE pixelsCountForNonPriorityAxes = GetPixelsCountForNonPriorityAxes(config, averageBytesPerPixel, priorityAxes);

        FLOAT_TYPE marginPixelsCount = std::pow(2.0 * margin.value, static_cast<double>(nonPriorityAxes.size())); // priority axes have no margins
        FLOAT_TYPE pixelsCountPerMarginPixel = pixelsCountForNonPriorityAxes / marginPixelsCount;

        FLOAT_TYPE lagrangeMultiplier = 1.0;
        bool zIsPriorityAxis = StlUtilities::Contains(priorityAxes, Axis::Z);
        if (!zIsPriorityAxis)
        {
            if (nonPriorityAxes.size() == 3)
            {
                lagrangeMultiplier = 1.0 + (1.0 + sqrt(4.0 * pixelsCountPerMarginPixel + 1.0)) / (2.0 * pixelsCountPerMarginPixel);
            }
            else if (nonPriorityAxes.size() == 2)
            {
                lagrangeMultiplier = 1.0 + 1.0 / pixelsCountPerMarginPixel;
            }
            else if (nonPriorityAxes.size() == 1)
            {
                lagrangeMultiplier = 1.0 / pixelsCountPerMarginPixel;
            }
            else
            {
                // can't happen
            }
        }
        else
        {
            if (nonPriorityAxes.size() == 2)
            {
                lagrangeMultiplier = sqrt(pixelsCountPerMarginPixel) / (sqrt(pixelsCountPerMarginPixel) - 1.0);
            }
            else if (nonPriorityAxes.size() == 1)
            {
                lagrangeMultiplier = pixelsCountPerMarginPixel / (pixelsCountPerMarginPixel - 1.0);
            }
            else
            {
                // can't happen
            }
        }

        StlUtilities::Copy(config.imageSize, internalActiveAreaSize);

        for (size_t i = 0; i < nonPriorityAxes.size(); ++i)
        {
            Axis::Type nonPriorityAxis = nonPriorityAxes[i];
            internalActiveAreaSizeRef[nonPriorityAxis] = 2.0 * margin.value / (lagrangeMultiplier - 1);
        }

        if (!zIsPriorityAxis)
        {
            internalActiveAreaSizeRef[Axis::Z] = 2.0 * margin.value / lagrangeMultiplier;
        }
    }

    void ActiveAreaComputer::FillInternalActiveAreaSizeForZeroMargin(const Config& config, const vector<Axis::Type>& priorityAxes,
                    FLOAT_TYPE averageBytesPerPixel, SpatialVector* internalActiveAreaSize) const
    {
        bool zIsPriorityAxis = StlUtilities::Contains(priorityAxes, Axis::Z);
        int nonPriorityAxesSize = 3 - priorityAxes.size();
        FLOAT_TYPE pixelsCountForNonPriorityAxes = GetPixelsCountForNonPriorityAxes(config, averageBytesPerPixel, priorityAxes);

        SpatialVector& internalActiveAreaSizeRef = *internalActiveAreaSize;
        StlUtilities::Copy(config.imageSize, internalActiveAreaSize);

        if (!zIsPriorityAxis)
        {
            internalActiveAreaSizeRef[Axis::Z] = 1;
            if (nonPriorityAxesSize == 3)
            {
                FLOAT_TYPE axisSize = sqrt(pixelsCountForNonPriorityAxes);
                internalActiveAreaSizeRef[Axis::X] = axisSize;
                internalActiveAreaSizeRef[Axis::Y] = axisSize;
            }
            else if (nonPriorityAxesSize == 2)
            {
                Axis::Type leftNonPriorityAxis = StlUtilities::Contains(priorityAxes, Axis::X) ? Axis::Y : Axis::X;
                internalActiveAreaSizeRef[leftNonPriorityAxis] = pixelsCountForNonPriorityAxes;
            }
            // if nonPriorityAxesSize == 1, shall only set size by z, which is already doen
            // nonPriorityAxesSize == 0 can not be, as z is not a priority axis
        }
        else
        {
            if (nonPriorityAxesSize == 2)
            {
                FLOAT_TYPE axisSize = sqrt(pixelsCountForNonPriorityAxes);
                internalActiveAreaSizeRef[Axis::X] = axisSize;
                internalActiveAreaSizeRef[Axis::Y] = axisSize;
            }
            else if (nonPriorityAxesSize == 1)
            {
                Axis::Type leftNonPriorityAxis = StlUtilities::Contains(priorityAxes, Axis::X) ? Axis::Y : Axis::X;
                internalActiveAreaSizeRef[leftNonPriorityAxis] = pixelsCountForNonPriorityAxes;
            }
        }
    }

    void ActiveAreaComputer::FillNonPriorityAxes(const vector<Axis::Type>& priorityAxes, vector<Axis::Type>* nonPriorityAxes) const
    {
        nonPriorityAxes->clear();
        for (size_t i = 0; i < static_cast<size_t>(DIMENSIONS); ++i)
        {
            if (!StlUtilities::Contains(priorityAxes, (Axis::Type)i))
            {
                nonPriorityAxes->push_back((Axis::Type)i);
            }
        }
    }

    void ActiveAreaComputer::FillCorrectPrirityAxes(const vector<Axis::Type>& priorityAxes, vector<Axis::Type>* correctPriorityAxes) const
    {
        // If there are three priority axes, make it empty
        StlUtilities::Copy(priorityAxes, correctPriorityAxes);
        if (priorityAxes.size() == static_cast<size_t>(DIMENSIONS))
        {
            correctPriorityAxes->empty();
        }
    }

    ActiveAreaComputer::~ActiveAreaComputer()
    {
    }
}

