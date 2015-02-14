// Copyright (c) 2013 Vasili Baranau
// Distributed under the MIT software license
// See the accompanying file License.txt or http://opensource.org/licenses/MIT

#ifndef Core_Headers_OpenMpManager_h
#define Core_Headers_OpenMpManager_h

#include "Core/Headers/Macros.h"

namespace Core
{
    class OpenMpManager
    {
    public:
        static int GetCurrentNumberOfThreads();

        static int GetMaxPossibleNumberOfThreads();

        static int GetCurrentThreadIndex();

        static bool IsMaster();

        static void SetMaxPossibleNumberOfThreads(int maxNumberOfThreads);

        static void DisableDynamicThreadNumber();

        static bool IsOpenMpEnabled();

        static int GetNumberOfThreadsInParallelSection();

    private:
        OpenMpManager();

        virtual ~OpenMpManager();

        DISALLOW_COPY_AND_ASSIGN(OpenMpManager);
    };
}

#endif /* Core_Headers_OpenMpManager_h */
