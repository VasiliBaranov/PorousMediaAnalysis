// Copyright (c) 2013 Vasili Baranau
// Distributed under the MIT software license
// See the accompanying file License.txt or http://opensource.org/licenses/MIT

#ifndef ImageProcessing_Services_Headers_ContainingBallsAssigner_h
#define ImageProcessing_Services_Headers_ContainingBallsAssigner_h

#include "ImageProcessing/Services/Headers/BaseDataArrayProcessor.h"

namespace Services
{
    class ContainingBallsAssigner : public BaseDataArrayProcessor
    {
    private:
        mutable size_t maximumBallsCount;

    public:
        ContainingBallsAssigner(const ActiveAreaComputer& currentActiveAreaComputer, const Serializer& currentSerializer);

        void SetContainingBalls(const Model::Config& config) const;

        virtual ~ContainingBallsAssigner();

    private:
        void FillActiveAreas(const Model::IntermediateStatistics& intermediateStatistics, std::vector<Model::ActiveArea>* activeAreas) const;

        void SetContainingBalls(const std::vector<Model::ActiveArea>& activeAreas) const;

        void SetContainingBallsInActiveArea() const;

        void SetContainingBall(int x, int y, int z, Model::EuclideanDistanceSquareType ballRadiusSquare) const;

        void SetMaximumBallsMask(const std::vector<Model::ActiveArea>& activeAreas) const;

        void SetMaximumBallsMaskInActiveArea() const;

        DISALLOW_COPY_AND_ASSIGN(ContainingBallsAssigner);
    };
}

#endif /* ImageProcessing_Services_Headers_ContainingBallsAssigner.h_h */

