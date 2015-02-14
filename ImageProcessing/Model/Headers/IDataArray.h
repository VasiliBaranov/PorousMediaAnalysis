// Copyright (c) 2013 Vasili Baranau
// Distributed under the MIT software license
// See the accompanying file License.txt or http://opensource.org/licenses/MIT

#ifndef ImageProcessing_Model_Headers_IDataArray_h
#define ImageProcessing_Model_Headers_IDataArray_h

#include "ImageProcessing/Model/Headers/Types.h"
namespace Model { class Config; }

namespace Model
{
    class IDataArray
    {
    public:
        virtual void Initialize(const Config& currentConfig, std::string currentWorkingPath, const std::vector<ActiveArea>& currentActiveAreas) = 0;

        virtual void ChangeActiveArea(int activeAreaIndex) = 0;

        virtual void WriteCurrentActiveArea() const = 0;

        virtual int GetBytesPerPixel() const = 0;

        virtual void Clear() = 0;

        virtual ~IDataArray() { };
    };
}

#endif /* ImageProcessing_Model_Headers_IDataArray_h */

