// Copyright (c) 2013 Vasili Baranau
// Distributed under the MIT software license
// See the accompanying file License.txt or http://opensource.org/licenses/MIT

#ifndef ImageProcessing_Model_Headers_Image_h
#define ImageProcessing_Model_Headers_Image_h

// FreeImage libraries include windows.h (for windows compilation) a couple of times.
// windows.h defines some weird macros like max, min, DeleteFile, CopyFile,
// which we have to undefine right now.
#ifdef WINDOWS
    #define NOMINMAX
#endif
#include "FreeImagePlus.h"
#ifdef WINDOWS
    #undef DeleteFile
    #undef CopyFile
#endif

#include <cstring>
#include "Core/Headers/Macros.h"
#include "Core/Headers/Exceptions.h"
#include "Core/Headers/Types.h"
#include "ImageProcessing/Model/Headers/Types.h"

namespace Model
{
    template<class TData>
    class Image
    {
    private:
        fipImage currentImage;

    public:
        Image()
        {
            const int bitsInByte = 8;
            int expectedBitsPerPixel = sizeof(TData) * bitsInByte;

            // 8 bits will be saved as greyscale images
            // 32 bits will be saved as CMYK images. It will automatically provide some scale to view them in separate image browsing programs.
            // The RGBA scale can't be used, as low integer values correspond to full transparency and will not be seen, while in the CMYK scale they correspond to cyan.
//            if (expectedBitsPerPixel != 8 && expectedBitsPerPixel != 32 && expectedBitsPerPixel != 64)
//            {
//                throw Core::InvalidOperationException("Images support only 8, 32, and 64 bits in a pixel. Size of TData for DataArray class shall be equal to one of these values");
//            }

            // Saving 64 bit images as CMYK with 16 bit per channel produces Segmentation fault on GWDG
            if (expectedBitsPerPixel != 8 && expectedBitsPerPixel != 32)
            {
                throw Core::InvalidOperationException("Images support only 8 and 32 bits in a pixel. Size of TData for DataArray class shall be equal to one of these values");
            }
        }

        void Load(std::string imagePath)
        {
            // TIFF_CMYK flag will load CMYK bitmaps as separated CMYK (default is conversion to RGB).
            // If the image already is RGB, the flag is ignored.
            currentImage.load(imagePath.c_str(), TIFF_CMYK);
        }

        int GetSizeX() const
        {
            return currentImage.getHeight();
        }

        int GetSizeY() const
        {
            return currentImage.getWidth();
        }

        void Save(std::string imagePath) const
        {
            unsigned int bitsPerPixel = currentImage.getBitsPerPixel();
            if (bitsPerPixel == 8)
            {
                currentImage.save(imagePath.c_str(), TIFF_DEFAULT);
            }
            else if (bitsPerPixel == 32 || bitsPerPixel == 64)
            {
                currentImage.save(imagePath.c_str(), TIFF_CMYK | TIFF_LZW);
            }
        }

        void ReadRectangle(const Model::Rectangle& rectangle, TData** values) const
        {
            CopySubImage(rectangle, values, true);
        }

        void WriteRectangle(const Model::Rectangle& rectangle, TData** values)
        {
            CopySubImage(rectangle, values, false);
        }

        void Fill(TData value)
        {
            unsigned int width = currentImage.getWidth();
            unsigned int height = currentImage.getHeight();
            unsigned int scanLineWidth = currentImage.getScanWidth();

            BYTE* bits = currentImage.accessPixels();
            for (unsigned int x = 0; x < height; ++x)
            {
                TData* pixels = (TData*)bits;
                std::fill(pixels, pixels + width, value);
                bits += scanLineWidth;
            }
        }

        void EnsureBitsPerPixel()
        {
            const int bitsInByte = 8;
            int expectedBitsPerPixel = sizeof(TData) * bitsInByte;
            if (currentImage.getBitsPerPixel() != sizeof(TData) * bitsInByte)
            {
                EnsureToBitsPerPixel(expectedBitsPerPixel);
            }
        }

        void ResizeIfNecessary(int expectedSizeX, int expectedSizeY)
        {
            // Ensure width and height as well
            if (GetSizeX() != expectedSizeX || GetSizeY() != expectedSizeY)
            {
                // It is slow. Worse, it's only called before rewriting with some default value
                // But rescaling is called very rarely in this program (only once per image, if voxel sizes by x and y are not equal)
                // But method setSize seems to completely change the image, including metadata (which i want to preserve).
                // TODO: improve
                currentImage.rescale(expectedSizeY, expectedSizeX, FILTER_BILINEAR);
            }
        }

        void CheckBitsPerPixel() const
        {
            const int bitsInByte = 8;
            if (currentImage.getBitsPerPixel() != sizeof(TData) * bitsInByte)
            {
                throw Core::InvalidOperationException("Bits per pixel in the images are not equal to the size of the data element");
            }
        }

        void CheckSize(int expectedSizeX, int expectedSizeY) const
        {
            // Check width and height
            if (GetSizeX() != expectedSizeX || GetSizeY() != expectedSizeY)
            {
                throw Core::InvalidOperationException("Image size is incorrect");
            }
        }

        virtual ~Image()
        {
        }

    private:
        void EnsureToBitsPerPixel(int bitsPerPixel)
        {
            // can't actually happen
            if (bitsPerPixel == 4)
            {
                currentImage.convertTo4Bits();
            }
            else if (bitsPerPixel == 8)
            {
                currentImage.convertTo8Bits();
            }
            else if (bitsPerPixel == 24)
            {
                currentImage.convertTo24Bits();
            }
            else if (bitsPerPixel == 32)
            {
                currentImage.convertTo32Bits();
            }
            else if (bitsPerPixel == 64)
            {
                currentImage.convertToType(FIT_RGBA16, false);
            }
            else
            {
                throw Core::InvalidOperationException("Bits per pixel value is not supported");
            }
        }

//        NOTE
//        In FreeImage, FIBITMAP are based on a coordinate system that is upside down
//        relative to usual graphics conventions. Thus, the scanlines are stored upside down,
//        with the first scan in memory being the bottommost scan in the image.
        void CopySubImage(Model::Rectangle rectangle, TData** values, bool copyToMemory) const
        {
            unsigned int scanLineWidth = currentImage.getScanWidth();
            BYTE* bits = currentImage.accessPixels();

            int freeImageXCoordinate = currentImage.getHeight() - rectangle.leftCorner[Core::Axis::X] - 1;
            bits += scanLineWidth * freeImageXCoordinate;
            int maxInclusiveX = rectangle.boxSize[Core::Axis::X];
            for (int x = 0; x < maxInclusiveX; ++x)
            {
                TData* pixels = (TData*)bits;
                pixels += rectangle.leftCorner[Core::Axis::Y];

                if (copyToMemory)
                {
                    memcpy(values[x], pixels, rectangle.boxSize[Core::Axis::Y] * sizeof(TData));
                }
                else
                {
                    memcpy(pixels, values[x], rectangle.boxSize[Core::Axis::Y] * sizeof(TData));
                }

                // next line
                bits -= scanLineWidth;
            }
        }

        DISALLOW_COPY_AND_ASSIGN(Image);
    };
}

#endif /* ImageProcessing_Model_Headers_Image_h */

