// Copyright (c) 2013 Vasili Baranau
// Distributed under the MIT software license
// See the accompanying file License.txt or http://opensource.org/licenses/MIT

#include "../Headers/ImageProcessingManager.h"
#include "stdio.h"

#include "Core/Headers/OpenMpManager.h"
#include "ImageProcessing/Services/Headers/Serializer.h"
#include "ImageProcessing/Services/Headers/ImageResampler.h"
#include "ImageProcessing/Services/Headers/EuclideanDistanceComputer.h"
#include "ImageProcessing/Services/Headers/ContainingBallsAssigner.h"
#include "ImageProcessing/Services/Headers/PoreAndThroatComputer.h"
#include "ImageProcessing/Services/Headers/EmptyPoresRemover.h"
#include "ImageProcessing/Services/Headers/WatershedComputer.h"
#include "ImageProcessing/Services/Headers/ShortestPathComputer.h"

using namespace std;
using namespace Services;
using namespace Model;
using namespace Core;

namespace Execution
{
    ImageProcessingManager::ImageProcessingManager(Serializer* serializer,
            ImageResampler* imageResampler,
            EuclideanDistanceComputer* euclideanDistanceComputer,
            ContainingBallsAssigner* containingBallsAssigner,
            PoreAndThroatComputer* poreAndThroatComputer,
            EmptyPoresRemover* emptyPoresRemover,
            WatershedComputer* watershedComputer,
            ShortestPathComputer* shortestPathComputer)
    {
        this->serializer = serializer;
        this->imageResampler = imageResampler;
        this->euclideanDistanceComputer = euclideanDistanceComputer;
        this->containingBallsAssigner = containingBallsAssigner;
        this->poreAndThroatComputer = poreAndThroatComputer;
        this->emptyPoresRemover = emptyPoresRemover;
        this->watershedComputer = watershedComputer;
        this->shortestPathComputer = shortestPathComputer;
    }

    void ImageProcessingManager::SetUserConfig(const Model::Config& userConfig)
    {
        config.Reset();

        // Find the actual config file, read it
        serializer->ReadConfig(userConfig.baseFolder, &config);

        // Find the initial images folder, determine the size of the 3D image
        serializer->FillImageSize(&config);

        // All the parameters, that were specified through console, have higher priority
        config.MergeWith(userConfig);

        OpenMpManager::DisableDynamicThreadNumber();
        OpenMpManager::SetMaxPossibleNumberOfThreads(config.numberOfThreads);
        printf("Number of threads in parallel sections from config: %d, expected in OpenMP: %d, actual: %d\n", 
            config.numberOfThreads, OpenMpManager::GetMaxPossibleNumberOfThreads(), OpenMpManager::GetNumberOfThreadsInParallelSection());
    }

    void ImageProcessingManager::DoAll()
    {
        printf("Doing all..\n");

        CalculateEuclideanDistanceTransform();

        RemoveInnerBalls();

        AssignPoresAndThroats();
    }

    void ImageProcessingManager::CalculateEuclideanDistanceTransform()
    {
        printf("Resampling initial images...\n");
        imageResampler->ResampleImages(config);

        printf("Calculating Euclidean distance transform...\n");
        euclideanDistanceComputer->ComputeEuclideanDistances(config);
    }

    void ImageProcessingManager::RemoveInnerBalls()
    {
        printf("Setting containing balls...\n");
        containingBallsAssigner->SetContainingBalls(config);
    }

    void ImageProcessingManager::AssignPoresAndThroats()
    {
        printf("Assigning pores and throats...\n");
        poreAndThroatComputer->AssignPoreAndThroatIds(config);

        printf("Removing pores with zero volume...\n");
        emptyPoresRemover->RemoveEmptyPores(config);

        printf("Assigning pores with watershed ...\n");
        watershedComputer->AssignPoreAndThroatIds(config);
    }

    void ImageProcessingManager::CalculateShortestPathDistances()
    {
        printf("Resampling initial images...\n");
        imageResampler->ResampleImages(config);

        printf("Calculating shortest path distances...\n");
        shortestPathComputer->ComputeShortestPathDistances(config);
    }

    ImageProcessingManager::~ImageProcessingManager()
    {

    }
}

