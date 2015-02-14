// Copyright (c) 2013 Vasili Baranau
// Distributed under the MIT software license
// See the accompanying file License.txt or http://opensource.org/licenses/MIT

#ifndef ImageProcessing_Model_Headers_DataArray_h
#define ImageProcessing_Model_Headers_DataArray_h

#include "Image.h"

#include "Core/Headers/StlUtilities.h"
#include "Core/Headers/Path.h"
#include "Core/Headers/MemoryUtility.h"
#include "ImageProcessing/Model/Headers/Config.h"
#include "ImageProcessing/Model/Headers/Constants.h"
#include "ImageProcessing/Services/Headers/Serializer.h"
#include "ImageProcessing/Model/Headers/IDataArray.h"

namespace Model
{
    template<class TData>
    class DataArray : public virtual IDataArray
    {
    public:
        TData defaultValue;

    private:
        // Const variables
        const Config* config;
        const std::vector<ActiveArea>* activeAreas;

        std::string workingPath;
        std::vector<std::string> initialImageFilePaths;
        std::vector<std::string> imageFilePaths;

        TData*** dataArray;

        // Dynamic variables
        ActiveArea currentActiveArea;
        bool imageChanged;
        bool activeAreaLoaded;

    public:
        DataArray()
        {
            defaultValue = 0;
            dataArray = NULL;
        }

        OVERRIDE void Initialize(const Config& currentConfig, std::string currentWorkingPath, const std::vector<ActiveArea>& currentActiveAreas)
        {
            config = &currentConfig;
            activeAreas = &currentActiveAreas;
            workingPath = currentWorkingPath;

            Clear();

            FillImagePaths();

            imageChanged = false;
            activeAreaLoaded = false;

            AllocateMemory();
        }

        std::string GetWorkingPath() const
        {
            return workingPath;
        }

        virtual ~DataArray()
        {
            Clear();
        }

        OVERRIDE int GetBytesPerPixel() const
        {
            return sizeof(TData);
        }

        inline TData GetPixel(const Core::DiscreteSpatialVector& position) const
        {
            return GetPixel(position[Core::Axis::X], position[Core::Axis::Y], position[Core::Axis::Z]);
        }

        inline TData GetPixel(int x, int y, int z) const
        {
            int localXIndex = x - currentActiveArea.boxWithMargins.leftCorner[Core::Axis::X];
            int localYIndex = y - currentActiveArea.boxWithMargins.leftCorner[Core::Axis::Y];
            int localZIndex = z - currentActiveArea.boxWithMargins.leftCorner[Core::Axis::Z];

            // The first dimension is z. It's no mistake. 2D images have row-major order (as used in C++)
            // but adding them together makes z axis the major one.
            return dataArray[localZIndex][localXIndex][localYIndex];
        }

//        inline TData& GetPixelReference(const Core::DiscreteSpatialVector& position) const
//        {
//            return GetPixelReference(position[Core::Axis::X], position[Core::Axis::Y], position[Core::Axis::Z]);
//        }
//
//        inline TData& GetPixelReference(int x, int y, int z) const
//        {
//            int localXIndex = x - currentActiveArea.boxWithMargins.leftCorner[Core::Axis::X];
//            int localYIndex = y - currentActiveArea.boxWithMargins.leftCorner[Core::Axis::Y];
//            int localZIndex = z - currentActiveArea.boxWithMargins.leftCorner[Core::Axis::Z];
//
//            // The first dimension is z. It's no mistake. 2D images have row-major order (as used in C++)
//            // but adding them together makes z axis the major one.
//            return dataArray[localZIndex][localXIndex][localYIndex];
//        }

        inline void SetPixel(const Core::DiscreteSpatialVector& position, TData value)
        {
            SetPixel(position[Core::Axis::X], position[Core::Axis::Y], position[Core::Axis::Z], value);
        }

        inline void SetPixel(int x, int y, int z, TData value)
        {
            imageChanged = true;

            int localXIndex = x - currentActiveArea.boxWithMargins.leftCorner[Core::Axis::X];
            int localYIndex = y - currentActiveArea.boxWithMargins.leftCorner[Core::Axis::Y];
            int localZIndex = z - currentActiveArea.boxWithMargins.leftCorner[Core::Axis::Z];

            dataArray[localZIndex][localXIndex][localYIndex] = value;
        }

        // Saves the current active area to disk, loads the new active area from disk. If there are no images for the new active area, initializes the missing values with zeros
        OVERRIDE void ChangeActiveArea(int activeAreaIndex)
        {
            Core::Path::EnsureDirectory(workingPath);

            const Model::ActiveArea& newActiveArea = activeAreas->at(activeAreaIndex);

            // If the current active area covers the entire image, return
            if (activeAreaLoaded && currentActiveArea.boxWithMargins.boxSize == config->imageSize)
            {
                return;
            }

            // If the new active area is the same as the old one
            if (activeAreaLoaded && newActiveArea.boxWithMargins == currentActiveArea.boxWithMargins)
            {
                return;
            }

            Core::Path::EnsureDirectory(workingPath);

            // Save the current active area to the disk, if necessary
            if (activeAreaLoaded && imageChanged)
            {
                WriteCurrentActiveAreaSafe();
            }

            imageChanged = false;
            ReindexMemory(currentActiveArea, newActiveArea);
            currentActiveArea = newActiveArea;

            ReadCurrentActiveAreaSafe();

            activeAreaLoaded = true;
        }

        // Writes current active area to disk, if necessary
        OVERRIDE void WriteCurrentActiveArea() const
        {
            if (activeAreaLoaded && imageChanged)
            {
                Core::Path::EnsureDirectory(workingPath);

                WriteCurrentActiveAreaSafe();
            }
        }

        OVERRIDE void Clear()
        {
            if (dataArray != NULL)
            {
                Core::MemoryUtility::Free3DArray(dataArray);
                dataArray = NULL;
            }
        }

    private:
        void ReadCurrentActiveAreaSafe()
        {
            // Load the next active area from disk
            int startImageIndex = currentActiveArea.boxWithMargins.leftCorner[Core::Axis::Z];
            int endImageIndex = currentActiveArea.boxWithMargins.exclusiveRightCorner[Core::Axis::Z];

            printf("Reading images %d - %d from folder %s...\n", startImageIndex + 1, endImageIndex, workingPath.c_str());

            #pragma omp parallel for schedule(static)
            for (int i = startImageIndex; i < endImageIndex; ++i)
            {
                if ((i - startImageIndex + 1) % 10 == 0)
                {
                    printf("Reading image %d / %d...\n", i - startImageIndex + 1, endImageIndex - startImageIndex);
                }

                std::string imagePath = imageFilePaths[i];
                if (!Core::Path::Exists(imagePath))
                {
                    // Copy image from the initial folder path (to preserve all the metadata)
                    Core::Path::CopyFile(initialImageFilePaths[i], imagePath);

                    // Prepare the image and fill in the entire image with default values
                    ResizeAndResetImage(imagePath, defaultValue);

                    // Fill in the image in memory with default values
                    // NOTE: I may proceed to the function CopySubImage below as well, but it's a little slower
                    int localZIndex = i - currentActiveArea.boxWithMargins.leftCorner[Core::Axis::Z];
                    int elementsCount = currentActiveArea.boxWithMargins.boxSize[Core::Axis::X] * currentActiveArea.boxWithMargins.boxSize[Core::Axis::Y];
                    TData* valuesArray = dataArray[localZIndex][0]; // NOTE: I know that Allocate3DArray allocates the data in last dimension sequentially
                    std::fill(valuesArray, valuesArray + elementsCount, defaultValue);
                }
                else
                {
                    Rectangle imageRectangle(currentActiveArea.boxWithMargins);
                    TData** currentImageActiveArea = GetCurrentImageActiveArea(i);
                    LoadRectangle(imagePath, imageRectangle, currentImageActiveArea);
                }
            }
        }

        void WriteCurrentActiveAreaSafe() const
        {
            int startImageIndex = currentActiveArea.boxWithMargins.leftCorner[Core::Axis::Z];
            int endImageIndex = currentActiveArea.boxWithMargins.exclusiveRightCorner[Core::Axis::Z];

            printf("Writing images %d - %d to folder %s...\n", startImageIndex + 1, endImageIndex, workingPath.c_str());

            #pragma omp parallel for schedule(static)
            for (int i = startImageIndex; i < endImageIndex; ++i)
            {
                if ((i - startImageIndex + 1) % 10 == 0)
                {
                    printf("Writing image  %d / %d...\n", i - startImageIndex + 1, endImageIndex - startImageIndex);
                }

                std::string imagePath = imageFilePaths[i];
                if (!Core::Path::Exists(imagePath))
                {
                    // Copy an initial image from the initial images folder
                    Core::Path::CopyFile(initialImageFilePaths[i], imageFilePaths[i]);
                }

                Rectangle imageRectangle(currentActiveArea.boxWithMargins);
                TData** currentImageActiveArea = GetCurrentImageActiveArea(i);
                SaveRectangle(imagePath, imageRectangle, currentImageActiveArea);
            }
        }

        void AllocateMemory()
        {
            // The first dimension is z. It's no mistake. 2D images have row-major order (as used in C++)
            // but adding them together makes z axis the major one.
            size_t maxZSize = 0;
            size_t maxZXSize = 0;
            size_t maxActiveAreaSize = 0;

            // Find max active area size, allocate memory
            for (size_t i = 0; i < activeAreas->size(); ++i)
            {
                const Core::DiscreteSpatialVector& boxSize = activeAreas->at(i).boxWithMargins.boxSize;
                size_t currentAreaSize = Core::VectorUtilities::GetProductGeneric<Core::DiscreteSpatialVector, size_t>(boxSize);
                if (currentAreaSize > maxActiveAreaSize)
                {
                    maxActiveAreaSize = currentAreaSize;
                }

                size_t currentZXSize = static_cast<size_t>(boxSize[Core::Axis::Z]) * boxSize[Core::Axis::X];
                if (currentZXSize > maxZXSize)
                {
                    maxZXSize = currentZXSize;
                }

                if (static_cast<size_t>(boxSize[Core::Axis::Z]) > maxZSize)
                {
                    maxZSize = boxSize[Core::Axis::Z];
                }
            }

            dataArray = new TData** [maxZSize];
            dataArray[0] = new TData* [maxZXSize];
            dataArray[0][0] = new TData[maxActiveAreaSize];
        }

        void ResizeAndResetImage(std::string imagePath, TData defaultValue) const
        {
            Image<TData> currentImage;
            currentImage.Load(imagePath);
            currentImage.EnsureBitsPerPixel();
            currentImage.ResizeIfNecessary(config->imageSize[Core::Axis::X], config->imageSize[Core::Axis::Y]);
            currentImage.Fill(defaultValue);
            currentImage.Save(imagePath);
        }

        void LoadRectangle(std::string imagePath, const Model::Rectangle& rectangle, TData** values) const
        {
            Image<TData> currentImage;
            currentImage.Load(imagePath);
            currentImage.CheckBitsPerPixel();
            currentImage.CheckSize(config->imageSize[Core::Axis::X], config->imageSize[Core::Axis::Y]);
            currentImage.ReadRectangle(rectangle, values);
        }

        void SaveRectangle(std::string imagePath, const Model::Rectangle& rectangle, TData** values) const
        {
            Image<TData> currentImage;
            currentImage.Load(imagePath);
            currentImage.EnsureBitsPerPixel();
            currentImage.ResizeIfNecessary(config->imageSize[Core::Axis::X], config->imageSize[Core::Axis::Y]);
            currentImage.WriteRectangle(rectangle, values);
            currentImage.Save(imagePath);
        }

        TData** GetCurrentImageActiveArea(int zIndex) const
        {
            int localZIndex = zIndex - currentActiveArea.boxWithMargins.leftCorner[Core::Axis::Z];
            TData** currentImageActiveArea = dataArray[localZIndex];
            return currentImageActiveArea;
        }

        void ReindexMemory(const ActiveArea& previousActiveArea, const ActiveArea& currentActiveArea)
        {
            bool existingImageSizeChanges = activeAreaLoaded && (previousActiveArea.boxWithMargins.boxSize != currentActiveArea.boxWithMargins.boxSize);

            if (!activeAreaLoaded || existingImageSizeChanges)
            {
                const Core::DiscreteSpatialVector& boxSize = currentActiveArea.boxWithMargins.boxSize;

                // The first dimension is z. It's no mistake. 2D images have row-major order (as used in C++)
                // but adding them together makes z axis the major one.
                Core::MemoryUtility::ReindexMemory<TData>(boxSize[2], boxSize[0], boxSize[1], dataArray);
            }
        }

        void FillImagePaths()
        {
            // read final images
            Services::Serializer::FillImagePaths(workingPath, &imageFilePaths);

            // if their count is OK
            size_t expectedImageCount = static_cast<size_t>(config->imageSize[Core::Axis::Z]);
            if (imageFilePaths.size() == expectedImageCount)
            {
                // copy them to the initialImageFilePaths
                initialImageFilePaths.resize(expectedImageCount);
                Core::StlUtilities::Copy(imageFilePaths, &initialImageFilePaths);
            }
            // if it is zero
            else if (imageFilePaths.size() == 0)
            {
                // read initial images
                std::string initialImagesFolder = Core::Path::Append(config->baseFolder, INITIAL_IMAGES_FOLDER_NAME);
                Services::Serializer::FillImagePaths(initialImagesFolder, &initialImageFilePaths);

                // if their count is not OK, throw exception
                if (initialImageFilePaths.size() != expectedImageCount)
                {
                    // throw exception
                    throw Core::InvalidOperationException("Working folder contains no images and initial folder contains wrong number of images");
                }

                // copy them to the image path, change the folder
                ChangeFolder(initialImagesFolder, workingPath, initialImageFilePaths, &imageFilePaths);
            }
            // if it is not zero
            else
            {
                // throw exception
                throw Core::InvalidOperationException("Number of images is incorrect: it is neither zero nor the expected number of images");
            }
        }

        void ChangeFolder(std::string sourceFolder, std::string targetFolder, const std::vector<std::string>& sourcePaths, std::vector<std::string>* targetPaths) const
        {
            std::vector<std::string>& targetPathsRef = *targetPaths;
            targetPathsRef.resize(sourcePaths.size());

            for (size_t i = 0; i < targetPathsRef.size(); ++i)
            {
                std::string sourcePath = sourcePaths[i];
                std::string targetName = Core::Path::GetFileName(sourcePath);

                targetPathsRef[i] = Core::Path::Append(targetFolder, targetName);
            }
        }

        DISALLOW_COPY_AND_ASSIGN(DataArray);
    };
}

#endif /* ImageProcessing_Model_Headers_DataArray_h */

