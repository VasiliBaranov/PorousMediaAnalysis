// Copyright (c) 2013 Vasili Baranau
// Distributed under the MIT software license
// See the accompanying file License.txt or http://opensource.org/licenses/MIT

#include "../Headers/ImageResampler.h"

#include <string>
#include <cmath>
#include "stdio.h"

#include "Core/Headers/Path.h"
#include "ImageProcessing/Services/Headers/ActiveAreaComputer.h"
#include "ImageProcessing/Model/Headers/Config.h"
#include "ImageProcessing/Model/Headers/DataArray.h"
#include "ImageProcessing/Model/Headers/Image.h"

using namespace std;
using namespace Core;
using namespace Model;
using namespace Services;

namespace Services
{
    ImageResampler::ImageResampler()
    {
    }

    void ImageResampler::ResampleImages(const Config& config) const
    {
        this->config = &config;

        string imagesFolderBeforeResampling = Path::Append(config.baseFolder, INITIAL_IMAGES_BEFORE_RESAMPLING_FOLDER_NAME);
        bool imagesAlreadyResampled = Path::Exists(imagesFolderBeforeResampling);
        if (imagesAlreadyResampled)
        {
            return;
        }

        DiscreteSpatialVector actualDimensions;
        FillActualDimensions(&actualDimensions);

        // If all dimensions are correct, return
        if (actualDimensions == config.imageSize)
        {
            return;
        }

        string initialImagesPath = Path::Append(config.baseFolder, INITIAL_IMAGES_FOLDER_NAME);
        string initialImagesBeforeResamplingPath = Path::Append(config.baseFolder, INITIAL_IMAGES_BEFORE_RESAMPLING_FOLDER_NAME);
        Path::Rename(initialImagesPath, initialImagesBeforeResamplingPath);
        Path::CreateFolder(initialImagesPath);

        vector<string> imagePathsBeforeResamplingWithDuplicates;
        vector<string> imagePathsAfterResamplingWithPostfixes;
        FillImageMapping(&imagePathsBeforeResamplingWithDuplicates, &imagePathsAfterResamplingWithPostfixes);

        // Copy to another folder, resample images by z
        ResampleImagesByZ(imagePathsBeforeResamplingWithDuplicates, imagePathsAfterResamplingWithPostfixes);

        // If necessary, resample the images by x and y
        if (actualDimensions[Axis::X] == config.imageSize[Axis::X] && actualDimensions[Axis::Y] == config.imageSize[Axis::Y])
        {
            return;
        }

        ResampleImagesByXY(actualDimensions, imagePathsBeforeResamplingWithDuplicates, imagePathsAfterResamplingWithPostfixes);
    }

    void ImageResampler::FillActualDimensions(DiscreteSpatialVector* actualDimensions) const
    {
        DiscreteSpatialVector& actualDimensionsRef = *actualDimensions;

        string initialImagesPath = Path::Append(config->baseFolder, INITIAL_IMAGES_FOLDER_NAME);
        vector<string> initialImagePaths;
        Serializer::FillImagePaths(initialImagesPath, &initialImagePaths);

        actualDimensionsRef[Axis::Z] = initialImagePaths.size();

        Image<IsSolidMaskType> firstImage;
        firstImage.Load(initialImagePaths[0]);
        actualDimensionsRef[Axis::X] = firstImage.GetSizeX();
        actualDimensionsRef[Axis::Y] = firstImage.GetSizeY();
    }

    void ImageResampler::ResampleImagesByXY(const DiscreteSpatialVector& actualDimensions, const vector<string>& imagePathsBeforeResamplingWithDuplicates,
            const vector<string>& imagePathsAfterResamplingWithPostfixes) const
    {
        IsSolidMaskType** initialImage = MemoryUtility::Allocate2DArray<IsSolidMaskType>(actualDimensions[Axis::X], actualDimensions[Axis::Y]);
        IsSolidMaskType** finalImage = MemoryUtility::Allocate2DArray<IsSolidMaskType>(config->imageSize[Axis::X], config->imageSize[Axis::Y]);

        Model::Rectangle initialRectangle;
        VectorUtilities::InitializeWith(&initialRectangle.leftCorner, 0);
        initialRectangle.exclusiveRightCorner[Axis::X] = actualDimensions[Axis::X];
        initialRectangle.exclusiveRightCorner[Axis::Y] = actualDimensions[Axis::Y];
        initialRectangle.boxSize = initialRectangle.exclusiveRightCorner;

        Model::Rectangle finalRectangle;
        VectorUtilities::InitializeWith(&finalRectangle.leftCorner, 0);
        finalRectangle.exclusiveRightCorner[Axis::X] = config->imageSize[Axis::X];
        finalRectangle.exclusiveRightCorner[Axis::Y] = config->imageSize[Axis::Y];
        finalRectangle.boxSize = finalRectangle.exclusiveRightCorner;

        Image<IsSolidMaskType> initialImageFile;
        Image<IsSolidMaskType> finalImageFile;

        size_t imagesSize = imagePathsBeforeResamplingWithDuplicates.size();
        for (size_t i = 0; i < imagesSize; ++i)
        {
            printf("Resampling image by x and y " SIZE_T_FORMAT " / " SIZE_T_FORMAT "...\n", i + 1, imagesSize);

            initialImageFile.Load(imagePathsBeforeResamplingWithDuplicates[i]);
            initialImageFile.ReadRectangle(initialRectangle, initialImage);

            finalImageFile.Load(imagePathsAfterResamplingWithPostfixes[i]);
            finalImageFile.EnsureBitsPerPixel();
            finalImageFile.ResizeIfNecessary(config->imageSize[Core::Axis::X], config->imageSize[Core::Axis::Y]);
            finalImageFile.ReadRectangle(finalRectangle, finalImage);

            ResampleImageByXY(actualDimensions, initialImage, finalImage);

            finalImageFile.Save(imagePathsAfterResamplingWithPostfixes[i]);
        }

        MemoryUtility::Free2DArray(initialImage);
        MemoryUtility::Free2DArray(finalImage);
    }

    void ImageResampler::ResampleImageByXY(const DiscreteSpatialVector& actualDimensions, IsSolidMaskType** initialImage, IsSolidMaskType** finalImage) const
    {
        for (int x = 0; x < config->imageSize[Axis::X]; ++x)
        {
            for (int y = 0; y < config->imageSize[Axis::Y]; ++y)
            {
                // automatic floor
                int actualImageX = x / config->normalizedResolutionOfInitialImages[Axis::X];
                int actualImageY = y / config->normalizedResolutionOfInitialImages[Axis::Y];
                finalImage[x][y] = initialImage[actualImageX][actualImageY];
            }
        }
    }

    void ImageResampler::ResampleImagesByZ(const vector<string>& imagePathsBeforeResamplingWithDuplicates,
            const vector<string>& imagePathsAfterResamplingWithPostfixes) const
    {
        size_t imagesSize = imagePathsBeforeResamplingWithDuplicates.size();
        for (size_t i = 0; i < imagesSize; ++i)
        {
            printf("Resampling image by z " SIZE_T_FORMAT " / " SIZE_T_FORMAT "...\n", i + 1, imagesSize);
            Path::CopyFile(imagePathsBeforeResamplingWithDuplicates[i], imagePathsAfterResamplingWithPostfixes[i]);
        }
    }

    void ImageResampler::FillImageMapping(vector<string>* imagePathsBeforeResamplingWithDuplicates,
                            vector<string>* imagePathsAfterResamplingWithPostfixes) const
    {
        string initialImagesBeforeResamplingPath = Path::Append(config->baseFolder, INITIAL_IMAGES_BEFORE_RESAMPLING_FOLDER_NAME);

        vector<string> imagePathsBeforeResampling;
        Serializer::FillImagePaths(initialImagesBeforeResamplingPath, &imagePathsBeforeResampling);

        FillImagePathsBeforeResamplingWithDuplicates(imagePathsBeforeResampling, imagePathsBeforeResamplingWithDuplicates);
        FillImagePathsAfterResamplingWithPostfixes(*imagePathsBeforeResamplingWithDuplicates, imagePathsAfterResamplingWithPostfixes);
    }

    void ImageResampler::FillImagePathsAfterResamplingWithPostfixes(const vector<string>& imagePathsBeforeResamplingWithDuplicates,
                    std::vector<std::string>* imagePathsAfterResamplingWithPostfixes) const
    {
        std::vector<std::string>& imagePathsAfterResamplingWithPostfixesRef = *imagePathsAfterResamplingWithPostfixes;

        // Get the names from the main folder
        imagePathsAfterResamplingWithPostfixesRef.resize(imagePathsBeforeResamplingWithDuplicates.size());
        Core::StlUtilities::Copy(imagePathsBeforeResamplingWithDuplicates, imagePathsAfterResamplingWithPostfixes);

        // Change the base folder, add postfixes to name duplicates
        std::string previousImageName = "";
        int duplicateIndex = 0;
        for (size_t i = 0; i < imagePathsAfterResamplingWithPostfixesRef.size(); ++i)
        {
            std::string imagePath = imagePathsAfterResamplingWithPostfixesRef[i];
            std::string imageName = Core::Path::GetFileName(imagePath);
            if (imageName == previousImageName)
            {
                duplicateIndex++;
                imageName = Core::Path::GetFileNameWithoutExtension(imagePath) + "_" + Core::Utilities::ConvertToString(duplicateIndex) + Core::Path::GetExtension(imagePath);
            }
            else
            {
                duplicateIndex = 0;
                previousImageName = imageName;
            }

            string initialFolderPath = Path::Append(config->baseFolder, INITIAL_IMAGES_FOLDER_NAME);
            imagePathsAfterResamplingWithPostfixesRef[i] = Path::Append(initialFolderPath, imageName);
        }
    }

    void ImageResampler::FillImagePathsBeforeResamplingWithDuplicates(const vector<string>& actualImagePaths, vector<string>* imagePathsBeforeResamplingWithDuplicates) const
    {
        std::vector<std::string>& imagePathsBeforeResamplingWithDuplicatesRef = *imagePathsBeforeResamplingWithDuplicates;
        imagePathsBeforeResamplingWithDuplicatesRef.resize(config->imageSize[Core::Axis::Z]);
        for (int i = 0; i < config->imageSize[Core::Axis::Z]; ++i)
        {
            int actualImageIndex = i / config->normalizedResolutionOfInitialImages[Core::Axis::Z]; // automatic floor
            imagePathsBeforeResamplingWithDuplicatesRef[i] = actualImagePaths[actualImageIndex];
        }
    }

    ImageResampler::~ImageResampler()
    {
    }
}

