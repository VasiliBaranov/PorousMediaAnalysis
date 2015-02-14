// Copyright (c) 2013 Vasili Baranau
// Distributed under the MIT software license
// See the accompanying file License.txt or http://opensource.org/licenses/MIT

#ifndef ImageProcessing_Services_Headers_Serializer_h
#define ImageProcessing_Services_Headers_Serializer_h

#include <stdio.h>
#include <string>
#include <vector>
#include <map>
#include <boost/array.hpp>
#include "Core/Headers/Macros.h"
#include "Core/Headers/Utilities.h"
#include "Core/Headers/Types.h"
#include "Core/Headers/IEndiannessProvider.h"
#include "ImageProcessing/Model/Headers/Types.h"

namespace Model { class Config; }
namespace Model { struct IntermediateStatistics; }

namespace Services
{
    class Serializer
    {
    private:
        Core::IEndiannessProvider* endiannessProvider;

    public:
        explicit Serializer(Core::IEndiannessProvider* endiannessProvider);

        // Reads
        void ReadConfig(std::string baseFolder, Model::Config* config) const;

        void FillImageSize(Model::Config* config) const;

        void ReadImage(std::string imagePath) const;

        static void FillImagePaths(std::string folder, std::vector<std::string>* imagePaths);

        void ReadSharedPixelTypes(std::string sharedPixelTypesPath, std::map<Model::PoreThroatIdType, Model::SharedPixelType>* sharedPixelTypesByIds) const;

        void ReadPores(std::string poresPath, std::map<Model::PoreThroatIdType, Model::Pore>* poresByIds) const;

        void ReadIntermediateStatistics(std::string intermediateStatisticsPath, Model::IntermediateStatistics* intermediateStatistics) const;

        // Writes
        void WritePores(std::string poreCentersPath, const std::map<Model::PoreThroatIdType, Model::Pore>& poresByIds) const;

        void WriteSharedPixelTypes(std::string throatsPath, const std::map<Model::PoreThroatIdType, Model::SharedPixelType>& throatsByIds) const;

        void WriteIntermediateStatistics(std::string intermediateStatisticsPath, const Model::IntermediateStatistics& intermediateStatistics) const;

        void WriteEuclideanDistanceSquareCounts(std::string euclideanDistanceSquaresCountsPath, const std::vector<size_t>& euclideanDistanceSquaresCounts) const;

        void WriteCavityDescriptions(std::string cavityDescriptionsPath, const std::vector<Model::CavityDescription>& cavityDescriptions) const;

        virtual ~Serializer();

    private:
        void ReadNextLine(FILE* file, std::string* line) const;

        boost::array<Core::FLOAT_TYPE, 3> MakeSpatialVectorThreeDimensional(const Core::SpatialVector& vector) const;

        DISALLOW_COPY_AND_ASSIGN(Serializer);

        // Though one may need to read float values into a structure, so passing void* instead of T* makes sense,
        // i prefer to pass T* for type safety and to make casts explicit.
        template<class T>
        void SwapBytesIfNecessary(const T* source, T* target, int valuesCount) const
        {
            if (endiannessProvider->IsBigEndian())
            {
                for (int i = 0; i < valuesCount; ++i)
                {
                    target[i] = Core::Utilities::DoByteSwap(source[i]);
                }
            }
            else
            {
                // Can't pass TRandomAccessIteratorSource, TRandomAccessIteratorTarget to this function and pass vector.begin(), if necessary,
                // because will not be able to perform this comparison.
                if (source != target)
                {
                    std::copy(source, source + valuesCount, target);
                }
            }
        }

        template<class T>
        void WriteLittleEndian(const T* values, size_t valuesCount, FILE* file) const
        {
            std::vector<T> valuesCopy(valuesCount);
            T* valuesCopyArray = &valuesCopy[0];
            SwapBytesIfNecessary(values, valuesCopyArray, valuesCount);

            fwrite(valuesCopyArray, sizeof(T), valuesCount, file);
            fflush(file); // if we do not flush and valuesCopy is deleted, nothing may be written to the file
        }

        // Not a generic container to be sure that elements are in consecutive order
        template<class T>
        void WriteLittleEndian(const std::vector<T>& values, FILE* file) const
        {
            // This is a hack, but it's the most popular answer in http://stackoverflow.com/a/1693505/2029962
            // "Fastest way to write large STL vector to file using STL".
            WriteLittleEndian<T>(&values[0], values.size(), file);
        }

        template<class T>
        bool ReadLittleEndian(T* values, size_t valuesCount, FILE* file) const
        {
            size_t readValuesCount = fread(values, sizeof(T), valuesCount, file);
            SwapBytesIfNecessary(values, values, valuesCount);
            return readValuesCount == valuesCount;
        }
    };
}

#endif /* ImageProcessing_Services_Headers_Serializer_h */

