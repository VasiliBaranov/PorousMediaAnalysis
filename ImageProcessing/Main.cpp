// Copyright (c) 2013 Vasili Baranau
// Distributed under the MIT software license
// See the accompanying file License.txt or http://opensource.org/licenses/MIT

#include <string>
#include <stdlib.h>
#include <stdio.h>
#include <cstring>

// FreeImage libraries include windows.h (for windows compilation) a couple of times.
// windows.h defines some weird macros like max, min, DeleteFile, CopyFile,
// which we have to undefine right now.
#ifdef WINDOWS
    #define NOMINMAX
#endif
#include "FreeImage.h"
#ifdef WINDOWS
    #undef DeleteFile
    #undef CopyFile
#endif

#include "Core/Headers/Path.h"
#include "Core/Headers/OpenMpManager.h"
#include "Execution/Headers/CommandLineDispatcher.h"

using namespace std;
using namespace Execution;
using namespace Core;

int main (int argc, char **argv)
{
    // Change buffering mode to store printf output correctly:
    // 1. when program is terminated with Load Sharing Facility, and buffer may not be flushed
    // 2. when output is redirected or added to a file (> log.txt or | tee log.txt) and an error occurs
    int bufferChangeResult = setvbuf(stdout, NULL, _IONBF, 0);
    if (bufferChangeResult != 0)
    {
        perror("Buffering mode could not be changed");
        return EXIT_FAILURE;
    }

    if (OpenMpManager::IsOpenMpEnabled())
    {
        printf("OpenMP enabled!\n");
    }
    else
    {
        printf("OpenMP NOT enabled!\n");
    }

// FREEIMAGE_LIB is defined only for static linkage under Windows (it is a define from the FreeImage library).
// For static linkage under GCC it's not defined. It is because GCC supports a special "__attribute__((constructor))" attribute,
// 1. which runs FreeImage_Initialise when it's loaded as a shared library and 
// 2. which includes call to FreeImage_Initialise before "main" during static linkage as well.
// Windows supports only DllMain special function, which will be run when the library is dynamically loaded.
// There is no way to add this call during static linkage under windows (probably),
// therefore we need to use a FREEIMAGE_LIB define and call FreeImage_Initialise manually.
#ifdef FREEIMAGE_LIB
    FreeImage_Initialise();
#endif

    try
    {
        CommandLineDispatcher commandLineDispatcher;
        commandLineDispatcher.DispatchCommandLineParameters(Path::GetCurrentWorkingFolder(), argc, argv);
    }

    catch (Exception& e)
    {
#ifdef FREEIMAGE_LIB
        FreeImage_DeInitialise();
#endif

        printf(e.GetMessage().c_str());
    }
    catch (std::exception& e)
    {
#ifdef FREEIMAGE_LIB
        FreeImage_DeInitialise();
#endif

        printf(e.what());
    }

    catch (...)
    {
#ifdef FREEIMAGE_LIB
        FreeImage_DeInitialise();
#endif

        printf("Unknown error encountered");
    }

#ifdef FREEIMAGE_LIB
        FreeImage_DeInitialise();
#endif

    return(0);
}

