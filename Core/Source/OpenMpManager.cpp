// Copyright (c) 2013 Vasili Baranau
// Distributed under the MIT software license
// See the accompanying file License.txt or http://opensource.org/licenses/MIT

#include "../Headers/OpenMpManager.h"

#ifdef _OPENMP
#include <omp.h>
#endif

namespace Core
{
#ifdef _OPENMP
    int OpenMpManager::GetCurrentNumberOfThreads()
    {
        return omp_get_num_threads();
    }

    int OpenMpManager::GetMaxPossibleNumberOfThreads()
    {
        return omp_get_max_threads();
    }

    int OpenMpManager::GetCurrentThreadIndex()
    {
        return omp_get_thread_num();
    }

    bool OpenMpManager::IsMaster()
    {
        return GetCurrentThreadIndex() == 0;
    }

    void OpenMpManager::SetMaxPossibleNumberOfThreads(int maxNumberOfThreads)
    {
        omp_set_num_threads(maxNumberOfThreads);
    }

    void OpenMpManager::DisableDynamicThreadNumber()
    {
        omp_set_dynamic(0); // Explicitly disable dynamic teams
    }

    bool OpenMpManager::IsOpenMpEnabled()
    {
        return true;
    }

    int OpenMpManager::GetNumberOfThreadsInParallelSection()
    {
        int numberOfThreads;
        #pragma omp parallel shared(numberOfThreads)
        {
            if (IsMaster())
            {
                numberOfThreads = GetCurrentNumberOfThreads();
            }
        }
        
        return numberOfThreads;
    }

//no OPENMP
#else
    int OpenMpManager::GetCurrentNumberOfThreads()
    {
        return 1;
    }

    int OpenMpManager::GetMaxPossibleNumberOfThreads()
    {
        return 1;
    }

    int OpenMpManager::GetCurrentThreadIndex()
    {
        return 0;
    }
    
    bool OpenMpManager::IsMaster()
    {
        return true;
    }

    void OpenMpManager::SetMaxPossibleNumberOfThreads(int maxNumberOfThreads)
    {
    }

    void OpenMpManager::DisableDynamicThreadNumber()
    {
    }
    
    bool OpenMpManager::IsOpenMpEnabled()
    {
        return false;
    }

    int OpenMpManager::GetNumberOfThreadsInParallelSection()
    {
        return 1;
    }

#endif
}
