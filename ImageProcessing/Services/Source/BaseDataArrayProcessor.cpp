// Copyright (c) 2013 Vasili Baranau
// Distributed under the MIT software license
// See the accompanying file License.txt or http://opensource.org/licenses/MIT

#include "../Headers/BaseDataArrayProcessor.h"
#include "Core/Headers/OpenMpManager.h"
#include "ImageProcessing/Services/Headers/EuclideanDistanceDerivativesComputer.h"
#include "ImageProcessing/Services/Headers/Serializer.h"

using namespace std;
using namespace Core;
using namespace Model;
using namespace Services;

namespace Services
{
    BaseDataArrayProcessor::BaseDataArrayProcessor(const ActiveAreaComputer& currentActiveAreaComputer, const Serializer& currentSerializer) :
            activeAreaComputer(currentActiveAreaComputer),
            serializer(currentSerializer)
    {
    }

    void BaseDataArrayProcessor::InitializeEuclideanDistanceDerivatives(EuclideanDistanceSquareType maxNeededEuclideanDistanceSquare) const
    {
        EuclideanDistanceDerivativesComputer euclideanDistanceDerivativesComputer;
        euclideanDistanceDerivativesComputer.ComputeEuclideanDistanceDerivatives(maxNeededEuclideanDistanceSquare,
                &euclideanDistancesByEuclideanDistanceSquares, &ballLowerRadiiSquaresByEuclideanDistanceSquares, &ballLowerRadiiByEuclideanDistanceSquares);
    }

    IntermediateStatistics BaseDataArrayProcessor::ReadIntermediateStatistics() const
    {
        IntermediateStatistics intermediateStatistics;
        string intermediateStatisticsPath = Path::Append(config->baseFolder, INTERMEDIATE_STATISTICS_FILE_NAME);
        serializer.ReadIntermediateStatistics(intermediateStatisticsPath, &intermediateStatistics);

        return intermediateStatistics;
    }

    void BaseDataArrayProcessor::WriteIntermediateStatistics(const IntermediateStatistics& intermediateStatistics) const
    {
        string intermediateStatisticsPath = Path::Append(config->baseFolder, INTERMEDIATE_STATISTICS_FILE_NAME);
        serializer.WriteIntermediateStatistics(intermediateStatisticsPath, intermediateStatistics);
    }

    void BaseDataArrayProcessor::ResetPercentagePrinting(Core::Axis::Type axisForIteration) const
    {
        this->axisForIteration = axisForIteration;
        layersCompletedByMaster = 0;
        previouslyPrintedPercentage = std::numeric_limits<FLOAT_TYPE>::min();
    }

    void BaseDataArrayProcessor::PrintPercentageCompleted() const
    {
        if (OpenMpManager::IsMaster())
        {
            // Assume that all the layers are distributed uniformly and are executed approximately uniformly.
            // May update layersCompleted with "omp atomic" by all threads, but it adds some locking overhead.
            FLOAT_TYPE percentageCompleted = 100.0 * OpenMpManager::GetCurrentNumberOfThreads() * layersCompletedByMaster / config->imageSize[axisForIteration];

            if (percentageCompleted >= previouslyPrintedPercentage + 1.0)
            {
                printf("%.2f%% completed\n", percentageCompleted);
                previouslyPrintedPercentage = percentageCompleted;
            }

            layersCompletedByMaster++;
        }
    }

    BaseDataArrayProcessor::~BaseDataArrayProcessor()
    {
    }
}
