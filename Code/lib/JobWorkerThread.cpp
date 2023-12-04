//
// Created by twitc on 9/7/2023.
//

#include "JobWorkerThread.h"
#include "JobSystem.h"

JobWorkerThread::JobWorkerThread(const char *uniqueName, unsigned long workerJobChannels, JobSystem *jobSystem) : m_uniqueName(uniqueName),
                                                                                                                  m_workerJobChannels(workerJobChannels),
                                                                                                                  m_jobSystem(jobSystem)
{
}

JobWorkerThread::~JobWorkerThread()
{
    // If we havenot already signal
    //  thread that we should exit as soon as it can (after the current job if any)
    ShutDown();

    // Block on the thread's main until it has a chance to finish its current job and exit
    m_thread->join();
    delete m_thread;
    m_thread = nullptr;
}

void JobWorkerThread::StartUp()
{
    m_thread = new std::thread(WorkerThreadMain, this);
}

void JobWorkerThread::Work()
{
    while (!IsStopping())
    {
        m_workerStatusMutex.lock();
        unsigned long workerJobChannels = m_workerJobChannels;
        m_workerStatusMutex.unlock();

        Job *job = m_jobSystem->ClaimAJob(m_workerJobChannels);
        if (job)
        {
            job->Execute();
            m_jobSystem->OnJobCompleted(job);
        }

        std::this_thread::sleep_for(std::chrono::microseconds(1));
    }
}

void JobWorkerThread::ShutDown()
{
    m_workerStatusMutex.lock();
    m_isStopping = true;
    m_workerStatusMutex.unlock();
}

bool JobWorkerThread::IsStopping() const
{
    m_workerStatusMutex.lock();
    bool shouldClose = m_isStopping;
    m_workerStatusMutex.unlock();

    return shouldClose;
}

void JobWorkerThread::SetWorkerJobChannels(unsigned long workerJobChannels)
{
    m_workerStatusMutex.lock();
    m_workerJobChannels = workerJobChannels;
    m_workerStatusMutex.unlock();
}

void JobWorkerThread::WorkerThreadMain(void *workerThreadObject)
{
    JobWorkerThread *thisWorker = (JobWorkerThread *)workerThreadObject;
    thisWorker->Work();
}