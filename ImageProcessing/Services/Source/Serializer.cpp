// Copyright (c) 2013 Vasili Baranau
// Distributed under the MIT software license
// See the accompanying file License.txt or http://opensource.org/licenses/MIT

#include "../Headers/Serializer.h"

#include <cmath>
#include <fstream>

#include "Core/Headers/Path.h"
#include "Core/Headers/ScopedFile.h"
#include "Core/Headers/VectorUtilities.h"
#include "Core/Headers/StlUtilities.h"
#include "Core/Headers/IEndiannessProvider.h"
#include "ImageProcessing/Model/Headers/Config.h"
#include "ImageProcessing/Model/Headers/Types.h"
#include "ImageProcessing/Model/Headers/Constants.h"
#include "ImageProcessing/Model/Headers/Image.h"

using namespace std;
using namespace Core;
using namespace Model;

namespace Services
{
    Serializer::Serializer(IEndiannessProvider* endiannessProvider)
    {
        this->endiannessProvider = endiannessProvider;
    }

    void Serializer::ReadConfig(string baseFolder, Config* config) const
    {
        config->baseFolder = baseFolder;
        ScopedFile<ExceptionErrorHandler> file(Path::Append(baseFolder, CONFIG_FILE_NAME), FileOpenMode::Read);
        boost::array<FLOAT_TYPE, 3> resolution; // need a separate 3D vector, as the system may be two-dimensional.

        int useD3Q7Stencil;

        // For scanning and printing doubles, see http://stackoverflow.com/questions/210590/why-does-scanf-need-lf-for-doubles-when-printf-is-okay-with-just-f
        fscanf(file,
                "Resolution of initial images: " FLOAT_FORMAT " " FLOAT_FORMAT " " FLOAT_FORMAT "\n" \
                "Available memory in megabytes: %d\n" \
                "Max ratio of pore radius to seed radius: " FLOAT_FORMAT "\n" \
                "Use D3Q7 stencil: %d\n" \
                "Number of threads: %d\n",
                &resolution[Axis::X],
                &resolution[Axis::Y],
                &resolution[Axis::Z],
                &config->availableMemoryInMegabytes,
                &config->maxPoreRadiusToSeedRadiusRatio,
                &useD3Q7Stencil,
                &config->numberOfThreads);

        config->stencilType = (useD3Q7Stencil != 0) ? StencilType::D3Q7 : StencilType::D3Q27;

        for (int i = 0; i < DIMENSIONS; ++i)
        {
            config->normalizedResolutionOfInitialImages[i] = resolution[i];
        }

        size_t minResolutionIndex = StlUtilities::FindMinElementPosition(config->normalizedResolutionOfInitialImages);
        Core::FLOAT_TYPE minResolution = config->normalizedResolutionOfInitialImages[minResolutionIndex];
        Core::VectorUtilities::DivideByValue(config->normalizedResolutionOfInitialImages, minResolution, &config->normalizedResolutionOfInitialImages);
    }

    void Serializer::ReadSharedPixelTypes(string sharedPixelTypesPath, std::map<Model::PoreThroatIdType, Model::SharedPixelType>* sharedPixelTypesByIds) const
    {
        std::map<Model::PoreThroatIdType, Model::SharedPixelType>& sharedPixelTypesByIdsRef = *sharedPixelTypesByIds;
        sharedPixelTypesByIds->clear();

        fstream sharedPixelTypesStream(sharedPixelTypesPath.c_str(), ios_base::in);
        sharedPixelTypesStream.ignore(numeric_limits<streamsize>::max(), '\n'); // ignore header

        while (sharedPixelTypesStream.good())
        {
            SharedPixelType sharedPixelType;
            sharedPixelType.id = 0;

            sharedPixelTypesStream >> sharedPixelType.id;
            sharedPixelTypesStream.ignore(2); // ignore the "; " sequence

            sharedPixelTypesStream >> sharedPixelType.volume;
            sharedPixelTypesStream.ignore(2); // ignore the "; " sequence

            // Read pore indexes. There should be at least one pore index for a throat.
            char nextCharacter;
            do
            {
                PoreThroatIdType poreId;
                sharedPixelTypesStream >> poreId;
                sharedPixelType.poreIds.push_back(poreId);

                nextCharacter = sharedPixelTypesStream.peek();
            } while (nextCharacter != '\n' && sharedPixelTypesStream.good());

            sharedPixelTypesByIdsRef[sharedPixelType.id] = sharedPixelType;

            // ignore the "\n" and peek the next character. It is needed to set stream flag to bad at the end of the last line
            sharedPixelTypesStream.ignore(1);
            sharedPixelTypesStream.peek();
        }
    }

    void Serializer::ReadPores(string poresPath, map<PoreThroatIdType, Pore>* poresByIds) const
    {
        map<PoreThroatIdType, Pore>& poresByIdsRef = *poresByIds;
        ScopedFile<ExceptionErrorHandler> file(poresPath, FileOpenMode::Read);
        fscanf(file, "PoreId SeedX SeedY SeedZ SeedRadiusSquare IsBoundaryPore Volume\n");

        while (true)
        {
            Pore pore;
            int isBoundaryPore;
            int result = fscanf(file, "%u %d %d %d %d %d " SIZE_T_FORMAT "\n",
                    &pore.id,
                    &pore.seedPosition[Axis::X], &pore.seedPosition[Axis::Y], &pore.seedPosition[Axis::Z],
                    &pore.seedRadiusSquare, &isBoundaryPore, &pore.volume);

            pore.isBoundaryPore = (isBoundaryPore != 0);

            if (result != 7)
            {
                break;
            }

            poresByIdsRef[pore.id] = pore;
        }
    }

    void Serializer::ReadIntermediateStatistics(string intermediateStatisticsPath, IntermediateStatistics* intermediateStatistics) const
    {
        ScopedFile<ExceptionErrorHandler> file(intermediateStatisticsPath, FileOpenMode::Read);
        fscanf(file, "Max Euclidean distance square: %u\n" \
                "Maximum balls count: " SIZE_T_FORMAT "\n",
                &intermediateStatistics->maxEuclideanDistanceSquare,
                &intermediateStatistics->maximumBallsCount);
    }

    void Serializer::WriteIntermediateStatistics(string intermediateStatisticsPath, const IntermediateStatistics& intermediateStatistics) const
    {
        ScopedFile<ExceptionErrorHandler> file(intermediateStatisticsPath, FileOpenMode::Write);
        fprintf(file, "Max Euclidean distance square: %u\n" \
                "Maximum balls count: " SIZE_T_FORMAT "\n",
                intermediateStatistics.maxEuclideanDistanceSquare,
                intermediateStatistics.maximumBallsCount);
    }

    void Serializer::WriteEuclideanDistanceSquareCounts(std::string euclideanDistanceSquaresCountsPath, const std::vector<size_t>& euclideanDistanceSquaresCounts) const
    {
        ScopedFile<ExceptionErrorHandler> file(euclideanDistanceSquaresCountsPath, FileOpenMode::Write);
        fprintf(file, "EuclideanDistanceSquaresCount (line index is EuclideanDistanceSquare, the next line has index one)\n");

        // Start from one, as the first element is always zero (there are no pores with zero distance to solid)
        for (size_t i = 1; i < euclideanDistanceSquaresCounts.size(); ++i)
        {
            size_t euclideanDistanceSquaresCount = euclideanDistanceSquaresCounts[i];
            fprintf(file, SIZE_T_FORMAT "\n", euclideanDistanceSquaresCount);
        }
    }

    void Serializer::WriteCavityDescriptions(string cavityDescriptionsPath, const vector<CavityDescription>& cavityDescriptions) const
    {
        //        CavityIdType id;
        //Core::DiscreteSpatialVector seed;
        //ShortestPathDistanceType euclideanDistanceToMonolithCenter;
        //size_t voidPixelsCount;
        //size_t reachedWallsCount;
        //bool isReachingAllWalls;

        ScopedFile<ExceptionErrorHandler> file(cavityDescriptionsPath, FileOpenMode::Write);
        fprintf(file, "CavityId SeedX SeedY SeedZ DistanceToCenter VoidPixelsCount ReachedWallsCount IsReachingAllWalls\n");

        for (vector<CavityDescription>::const_iterator it = cavityDescriptions.begin(); it != cavityDescriptions.end(); ++it)
        {
            const CavityDescription& cavityDescription = (*it);
            int isBoundaryPore = cavityDescription.isReachingAllWalls ? 1 : 0;
            fprintf(file, "%u %d %d %d %f " SIZE_T_FORMAT " %d %d\n",
                    cavityDescription.id,
                    cavityDescription.seed[Axis::X], cavityDescription.seed[Axis::Y], cavityDescription.seed[Axis::Z],
                    cavityDescription.euclideanDistanceToMonolithCenter,
                    cavityDescription.voidPixelsCount,
                    cavityDescription.reachedWallsCount,
                    isBoundaryPore);
        }
    }

    void Serializer::WritePores(std::string poreCentersPath, const map<PoreThroatIdType, Pore>& poresByIds) const
    {
        ScopedFile<ExceptionErrorHandler> file(poreCentersPath, FileOpenMode::Write);
        fprintf(file, "PoreId SeedX SeedY SeedZ SeedRadiusSquare IsBoundaryPore Volume\n");

        for (map<PoreThroatIdType, Pore>::const_iterator it = poresByIds.begin(); it != poresByIds.end(); ++it)
        {
            const Pore& pore = (*it).second;
            int isBoundaryPore = pore.isBoundaryPore ? 1 : 0;
            fprintf(file, "%u %d %d %d %d %d " SIZE_T_FORMAT "\n",
                    pore.id,
                    pore.seedPosition[Axis::X], pore.seedPosition[Axis::Y], pore.seedPosition[Axis::Z],
                    pore.seedRadiusSquare, isBoundaryPore, pore.volume);
        }
    }

    void Serializer::WriteSharedPixelTypes(string sharedPixelTypePath, const map<PoreThroatIdType, SharedPixelType>& sharedPixelTypesByIds) const
    {
        ScopedFile<ExceptionErrorHandler> file(sharedPixelTypePath, FileOpenMode::Write);
        fprintf(file, "SharedPixelTypeId; Volume; PoreId1 PoreId2 ... \n");

        for (map<PoreThroatIdType, SharedPixelType>::const_iterator it = sharedPixelTypesByIds.begin(); it != sharedPixelTypesByIds.end(); ++it)
        {
            const SharedPixelType& sharedPixelType = (*it).second;
            fprintf(file, "%u; " SIZE_T_FORMAT ";", sharedPixelType.id, sharedPixelType.volume);

            // print pore indexes
            for (size_t i = 0; i < sharedPixelType.poreIds.size(); ++i)
            {
                PoreThroatIdType poreIndex = sharedPixelType.poreIds[i];
                fprintf(file, " %u", poreIndex);
            }
            fprintf(file, "\n");
        }
    }

    void Serializer::FillImageSize(Config* config) const
    {
        string imagesFolder = Path::Append(config->baseFolder, INITIAL_IMAGES_FOLDER_NAME);
        vector<string> imagePaths;
        FillImagePaths(imagesFolder, &imagePaths);

        if (imagePaths.size() == 0)
        {
            string errorMessage = "Initial images folder, \""+ INITIAL_IMAGES_FOLDER_NAME + "\", which shall be near the " + CONFIG_FILE_NAME + " file, is empty";
            throw InvalidOperationException(errorMessage);
        }

        Image<IsSolidMaskType> currentImage;
        currentImage.Load(imagePaths[0]);

        config->imageSize[0] = currentImage.GetSizeX();
        config->imageSize[1] = currentImage.GetSizeY();
        config->imageSize[2] = imagePaths.size();

        string imagesFolderBeforeResampling = Path::Append(config->baseFolder, INITIAL_IMAGES_BEFORE_RESAMPLING_FOLDER_NAME);
        bool imagesAlreadyResampled = Path::Exists(imagesFolderBeforeResampling);
        if (imagesAlreadyResampled)
        {
            return;
        }

        SpatialVector imageSizeAfterExtrapolation;
        VectorUtilities::Multiply(config->imageSize, config->normalizedResolutionOfInitialImages, &imageSizeAfterExtrapolation);
        VectorUtilities::Round(imageSizeAfterExtrapolation, &config->imageSize);
    }

    void Serializer::FillImagePaths(string folder, vector<string>* imagePaths)
    {
        // Get image file paths
        vector<string> fileNames;

        Path::FillFileNames(folder, &fileNames);

        vector<string> imageNames;
        vector<string> imageNamesWithoutExtensions;
        for (size_t i = 0; i < fileNames.size(); ++i)
        {
            string extension = Path::GetExtension(fileNames[i]);
            if (extension == ".tiff" || extension == ".tif")
            {
                imageNames.push_back(fileNames[i]);
                imageNamesWithoutExtensions.push_back(Path::GetFileNameWithoutExtension(fileNames[i]));
            }
        }

        vector<int> permutation;
        StlUtilities::SortPermutation(imageNamesWithoutExtensions, &permutation);

        imagePaths->clear();
        for (size_t i = 0; i < imageNames.size(); ++i)
        {
            imagePaths->push_back(Path::Append(folder, imageNames[permutation[i]]));
        }
    }

    boost::array<FLOAT_TYPE, 3> Serializer::MakeSpatialVectorThreeDimensional(const Core::SpatialVector& vector) const
    {
        boost::array<FLOAT_TYPE, 3> systemSize;
        for (int i = 0; i < DIMENSIONS; ++i)
        {
            systemSize[i] = vector[i];
        }
        return systemSize;
    }

    Serializer::~Serializer()
    {
    }
}

