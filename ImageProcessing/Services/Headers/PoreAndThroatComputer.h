// Copyright (c) 2013 Vasili Baranau
// Distributed under the MIT software license
// See the accompanying file License.txt or http://opensource.org/licenses/MIT

#ifndef ImageProcessing_Services_Headers_PoreAndThroatComputer_h
#define ImageProcessing_Services_Headers_PoreAndThroatComputer_h

#include <stack>
#include "ImageProcessing/Services/Headers/BasePoreAndThroatComputer.h"

namespace Services
{
    class PoreAndThroatComputer : public BasePoreAndThroatComputer
    {
    private:
        typedef Core::DiscreteSpatialVector NodeToVisit;

    public:
        PoreAndThroatComputer(const ActiveAreaComputer& currentActiveAreaComputer, const Serializer& currentSerializer);

        void AssignPoreAndThroatIds(const Model::Config& config) const;

        virtual ~PoreAndThroatComputer();

    private:
        // AssignPoreIds
        void AssignPoreIds(const std::vector<Model::ActiveArea>& activeAreas) const;

        Model::PoreThroatIdType AssignPoreIdsInActiveArea(Model::PoreThroatIdType startingPoreId) const;

        Model::PoreThroatIdType AssignPoreIdsInActiveArea(const std::vector<Model::MaximumBall>& maximumBalls, Model::PoreThroatIdType startingPoreId) const;

        void AssignPoreIdRecursively(Model::Pore* pore) const;

        void AddNeighborsToStack(const NodeToVisit& node, Model::Pore* pore, std::stack<NodeToVisit>* nodesToVisit) const;

        bool UpdatePixelAndSharedBalls(const NodeToVisit& node, Model::Pore* pore) const;

        void RemoveEmptySharedPixelTypes() const;

        void FillDescendingMaximumBalls(std::vector<Model::MaximumBall>* maximumBalls) const;

        DISALLOW_COPY_AND_ASSIGN(PoreAndThroatComputer);
    };
}

#endif /* ImageProcessing_Services_Headers_PoreAndThroatComputer_h */

