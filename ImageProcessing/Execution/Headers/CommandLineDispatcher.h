// Copyright (c) 2013 Vasili Baranau
// Distributed under the MIT software license
// See the accompanying file License.txt or http://opensource.org/licenses/MIT

#ifndef Execution_Headers_CommandLineDispatcher_h
#define Execution_Headers_CommandLineDispatcher_h

#include <string>
#include "Core/Headers/Macros.h"
#include "ImageProcessing/Model/Headers/Types.h"
namespace Execution { class ImageProcessingManager; }

namespace Execution
{
    class CommandLineDispatcher
    {
    private:
        std::string baseFolder;
        std::vector<std::string> consoleArguments;

    public:
        CommandLineDispatcher();

        void DispatchCommandLineParameters(std::string baseFolder, int argc, char **argv);

        ~CommandLineDispatcher();

        DISALLOW_COPY_AND_ASSIGN(CommandLineDispatcher);

    private:
        void ParseConsoleArguments(int argc, char **argv);

        Model::ExecutionMode::Type ParseExecutionMode();

        void CallCorrectMethod(ImageProcessingManager* generationManager, Model::ExecutionMode::Type executionMode) const;
    };
}

#endif /* Execution_Headers_CommandLineDispatcher_h */

