// Copyright (c) 2013 Vasili Baranau
// Distributed under the MIT software license
// See the accompanying file License.txt or http://opensource.org/licenses/MIT

#include "../Headers/CommandLineDispatcher.h"

#include <cstring>
#include "stdio.h"
#include "Core/Headers/Path.h"
#include "Core/Headers/EndiannessProvider.h"
#include "ImageProcessing/Execution/Headers/ImageProcessingManager.h"
#include "ImageProcessing/Model/Headers/Constants.h"
#include "ImageProcessing/Services/Headers/ActiveAreaComputer.h"
#include "ImageProcessing/Services/Headers/Serializer.h"
#include "ImageProcessing/Services/Headers/ImageResampler.h"
#include "ImageProcessing/Services/Headers/EuclideanDistanceComputer.h"
#include "ImageProcessing/Services/Headers/ContainingBallsAssigner.h"
#include "ImageProcessing/Services/Headers/PoreAndThroatComputer.h"
#include "ImageProcessing/Services/Headers/EmptyPoresRemover.h"
#include "ImageProcessing/Services/Headers/WatershedComputer.h"
#include "ImageProcessing/Services/Headers/ShortestPathComputer.h"

using namespace std;
using namespace Model;
using namespace Core;
using namespace Services;

namespace Execution
{
    CommandLineDispatcher::CommandLineDispatcher()
    {
    }

    void CommandLineDispatcher::DispatchCommandLineParameters(string baseFolder, int argc, char **argv)
    {
        this->baseFolder = baseFolder;
        ParseConsoleArguments(argc, argv);
        ExecutionMode::Type executionMode = ParseExecutionMode();

        ActiveAreaComputer activeAreaComputer;
        EndiannessProvider endiannessProvider;
        Serializer serializer(&endiannessProvider);

        ImageResampler imageResampler;
        EuclideanDistanceComputer euclideanDistanceComputer(activeAreaComputer, serializer);
        ContainingBallsAssigner containingBallsAssigner(activeAreaComputer, serializer);
        PoreAndThroatComputer poreAndThroatComputer(activeAreaComputer, serializer);
        EmptyPoresRemover emptyPoresRemover(activeAreaComputer, serializer);
        WatershedComputer watershedComputer(activeAreaComputer, serializer);
        ShortestPathComputer shortestPathComputer(activeAreaComputer, serializer);

        ImageProcessingManager imageProcessingManager(&serializer,
                &imageResampler,
                &euclideanDistanceComputer,
                &containingBallsAssigner,
                &poreAndThroatComputer,
                &emptyPoresRemover,
                &watershedComputer,
                &shortestPathComputer);

        vector<string> foldersWithConfigPaths;
        Path::FillFoldersRecursively(baseFolder, CONFIG_FILE_NAME, &foldersWithConfigPaths);

        // All the additional console parameters will also go to the user config
        Config userConfig;
        for (size_t i = 0; i < foldersWithConfigPaths.size(); ++i)
        {
            userConfig.baseFolder = foldersWithConfigPaths[i];
            imageProcessingManager.SetUserConfig(userConfig);
            CallCorrectMethod(&imageProcessingManager, executionMode);
        }
    }

    void CommandLineDispatcher::CallCorrectMethod(ImageProcessingManager* imageProcessingManager, ExecutionMode::Type executionMode) const
    {
        if (executionMode == ExecutionMode::Unknown)
        {
            printf("Console parameters are incorrect. Exiting.\n\n" \
                    "The following console parameters are supported:\n" \
                    "-edt - calculates Euclidean distance transform (maximum inscribed balls)\n" \
                    "-rib - removes inner balls (balls fully contained in other balls)\n" \
                    "-apt - assigns pores and throats\n" \
                    "-all - determines maximum inscribed balls, medial axis, pores, throats, and pore connectivity statistics\n" \
                    "-spd - computes shortest path distances (requires only initial binary image)\n");
        }

        if (executionMode == ExecutionMode::All)
        {
            imageProcessingManager->DoAll();
        }
        else if (executionMode == ExecutionMode::EuclideanDistanceTransform)
        {
            imageProcessingManager->CalculateEuclideanDistanceTransform();
        }
        else if (executionMode == ExecutionMode::InnerBallsRemoval)
        {
            imageProcessingManager->RemoveInnerBalls();
        }
        else if (executionMode == ExecutionMode::PoreAndThroatAssignment)
        {
            imageProcessingManager->AssignPoresAndThroats();
        }
        else if (executionMode == ExecutionMode::ShortestPathDistancesComputation)
        {
            imageProcessingManager->CalculateShortestPathDistances();
        }
    }

    void CommandLineDispatcher::ParseConsoleArguments(int argc, char **argv)
    {
        if (argc < 2)
        {
            return;
        }

        consoleArguments.reserve(argc - 1);
        for (int i = 1; i < argc; ++i)
        {
            consoleArguments.push_back(argv[i]);
        }
    }

    ExecutionMode::Type CommandLineDispatcher::ParseExecutionMode()
    {
        if (consoleArguments.size() == 0)
        {
            return ExecutionMode::Unknown;
        }

        if (consoleArguments[0] == "-all")
        {
            return ExecutionMode::All;
        }
        else if (consoleArguments[0] == "-edt")
        {
            return ExecutionMode::EuclideanDistanceTransform;
        }
        else if (consoleArguments[0] == "-rib")
        {
            return ExecutionMode::InnerBallsRemoval;
        }
        else if (consoleArguments[0] == "-apt")
        {
            return ExecutionMode::PoreAndThroatAssignment;
        }
        else if (consoleArguments[0] == "-spd")
        {
            return ExecutionMode::ShortestPathDistancesComputation;
        }
        else
        {
            return ExecutionMode::Unknown;
        }
    }

    CommandLineDispatcher::~CommandLineDispatcher()
    {

    }
}

