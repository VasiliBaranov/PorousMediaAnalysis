// Copyright (c) 2013 Vasili Baranau
// Distributed under the MIT software license
// See the accompanying file License.txt or http://opensource.org/licenses/MIT

#ifndef ImageProcessing_Services_Headers_EuclideanDistanceComputer_h
#define ImageProcessing_Services_Headers_EuclideanDistanceComputer_h

#include "ImageProcessing/Services/Headers/BaseDataArrayProcessor.h"

namespace Services
{
    // This class does a more or less naive Euclidean distance computation.
    // The algorithm is based on the very beginning (only introduction) of the paper Meijster et. al., 2000, A General Algorithm for Computing Distance Transforms in Linear Time.

    // As of 2014, there were the following good reviews of EDT algorithms: Fabbri et. al., 2008, 2D Euclidean Distance Transform Algorithms: A Comparative Survey and
    // Broek and Schouten, 2011, Distance transforms: Academics versus industry.

    // In the review of 2008, conclusion is the most valuable. It shows that the fastest are the algorithms of Meijster et. al. and Maurer et. al.
    // (Maurer et. al., 2003, A linear time algorithm for computing exact Euclidean distance transforms of binary images in arbitrary dimensions).
    // They are very similar in performance, but it seems that the algorithm of Maurer et. al. is much more complicated than that of Meijster et. al.

    // In the review of 2011, the most important are Table 3 and Figure 4. The FEED algorithm shall be excluded, as it is the one of the aurthors and it is not actually generic.
    // The data show that the fastest exact EDT algorithms are the one of Maurer (and, automatically, Meijster) and a new algorithm, LLT (Lucet, 2009, New sequential exact Euclidean distance transform algorithms based on convex analysis).
    // The last one is slightly faster, it uses the Legendre transform, but it seems to be easier for implementation than the one of Meijster.

    // So one can in future implement LucetEuclideanDistanceComputer or MeijsterEuclideanDistanceComputer.
    class EuclideanDistanceComputer : public BaseDataArrayProcessor
    {
    private:
        const static bool BOUNDARIES_ARE_SOLID = false;

        // Cache variables
        mutable std::vector<Model::ActiveArea> activeAreas;
        mutable size_t xyActiveAreasSize;

        mutable Model::EuclideanDistanceSquareType maxEuclideanDistanceSquare;
        mutable std::vector<size_t> euclideanDistanceSquareCounts;

    public:
        EuclideanDistanceComputer(const ActiveAreaComputer& currentActiveAreaComputer, const Serializer& currentSerializer);

        void ComputeEuclideanDistances(const Model::Config& config) const;

        virtual ~EuclideanDistanceComputer();

    private:
        void FillActiveAreas() const;

        void WriteResults() const;

        void WriteIntermediateStatistics() const;

        void WriteEuclideanDistanceSquareCounts() const;

        void FindAllDistancesToSolidsInXY() const;

        void FindAllDistancesToSolidsInXYZ() const;

        void FindDistancesToSolidsAlongX() const;

        void FindDistancesToSolidsInXY() const;

        void FindDistancesToSolidsInXYZ() const;

        DISALLOW_COPY_AND_ASSIGN(EuclideanDistanceComputer);
    };
}

#endif /* ImageProcessing_Services_Headers_EuclideanDistanceComputer_h */

