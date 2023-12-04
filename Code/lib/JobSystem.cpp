#include "JobSystem.h"
#include "JobWorkerThread.h"
#include "string.h"
#include <iostream>

JobSystem *JobSystem::s_jobSystem = nullptr;

typedef void (*JobCallback)(Job *completedJob);

JobSystem::JobSystem()
{
    m_jobHistory.reserve(256 * 1024);
}

JobSystem::~JobSystem()
{
    m_workerThreadsMutex.lock();
    int numWorkerThreads = (int)m_workerThreads.size();

    // First. tell each worker thread to stop picking up jobs
    for (int i = 0; i < numWorkerThreads; ++i)
    {
        m_workerThreads[i]->ShutDown();
    }

    while (!m_workerThreads.empty())
    {
        delete m_workerThreads.back();
        m_workerThreads.pop_back();
    }
    m_workerThreadsMutex.unlock();
}

JobSystem *JobSystem::CreateOrGet()
{
    if (!s_jobSystem)
    {
        s_jobSystem = new JobSystem();
    }
    return s_jobSystem;
}

void JobSystem::Destroy()
{
    if (s_jobSystem)
    {
        delete s_jobSystem;
        s_jobSystem = nullptr;
    }
}

void JobSystem::CreateWorkerThread(json input)
{
    const char *threadName = input["threadName"].get<std::string>().c_str();
    unsigned long threadChannel = input["threadChannel"].get<unsigned long>();
    JobWorkerThread *newWorker = new JobWorkerThread(threadName, threadChannel, this);
    m_workerThreadsMutex.lock();
    m_workerThreads.push_back(newWorker);
    m_workerThreadsMutex.unlock();

    newWorker->StartUp(); // Start the new worker thread
}

void JobSystem::DestroyWorkerThread(const char *uniqueName)
{
    m_workerThreadsMutex.lock();
    JobWorkerThread *doomedWorker = nullptr;
    std::vector<JobWorkerThread *>::iterator it = m_workerThreads.begin();
    for (; it != m_workerThreads.end(); ++it)
    {
        if (strcmp((*it)->m_uniqueName, uniqueName) == 0)
        {
            doomedWorker = *it;
            m_workerThreads.erase(it);
            break;
        }
    }
    m_workerThreadsMutex.unlock();
    if (doomedWorker)
    {
        doomedWorker->ShutDown();
        delete doomedWorker;
    }
}

void JobSystem::QueueJob(Job *job)
{
    m_jobsQueuedMutex.lock();

    m_jobHistoryMutex.lock();
    m_jobHistory.emplace_back(JobHistoryEntry(job->m_jobType, JOB_STATUS_QUEUED));
    m_jobHistoryMutex.unlock();

    m_jobsQueued.push_back(job);
    m_jobsQueuedMutex.unlock();
}

JobStatus JobSystem::GetJobStatus(json input) const
{
    int jobID = input["jobID"].get<int>();
    m_jobHistoryMutex.lock();
    JobStatus jobStatus = JOB_STATUS_NEVER_SEEN;
    if (jobID < (int)m_jobHistory.size())
    {
        jobStatus = (JobStatus)m_jobHistory[jobID].m_jobStatus;
    }
    m_jobHistoryMutex.unlock();
    return jobStatus;
}

bool JobSystem::IsJobComplete(json input) const
{
    return (GetJobStatus(input)) == (JOB_STATUS_COMPLETED);
}

void JobSystem::FinishCompletedJobs()
{
    std::deque<Job *> jobsCompleted;

    m_jobsCompletedMutex.lock();
    jobsCompleted.swap(m_jobsCompleted);
    m_jobsCompletedMutex.unlock();

    for (Job *job : jobsCompleted)
    {
        job->JobCompleteCallback();
        m_jobHistoryMutex.lock();
        m_jobHistory[job->m_jobID].m_jobStatus = JOB_STATUS_RETIRED;
        m_jobHistoryMutex.unlock();
        delete job;
    }
}

void JobSystem::FinishJob(json input)
{
    int jobID = input["jobID"].get<int>();
    while (!IsJobComplete(input))
    {
        JobStatus jobStatus = GetJobStatus(input);
        if (jobStatus == JOB_STATUS_NEVER_SEEN || jobStatus == JOB_STATUS_RETIRED)
        {
            std::cout << "ERROR: Waiting for Job(#" << jobID << ")   - no such job in JobSystem" << std::endl;
            return;
        }
    }
    m_jobsCompletedMutex.lock();
    Job *thisCompletedJob = nullptr;
    for (auto jcIter = m_jobsCompleted.begin(); jcIter != m_jobsCompleted.end(); ++jcIter)
    {
        Job *someCompletedJob = *jcIter;
        if (someCompletedJob->m_jobID == jobID)
        {
            thisCompletedJob = someCompletedJob;
            m_jobsCompleted.erase(jcIter);
            break;
        }
    }
    m_jobsCompletedMutex.unlock();

    if (thisCompletedJob == nullptr)
    {
        std::cout << "ERROR: Job#" << jobID << " was status complete but not found in jobSystem" << std::endl;
    }
    thisCompletedJob->JobCompleteCallback();
    m_jobHistoryMutex.lock();
    m_jobHistory[thisCompletedJob->m_jobID].m_jobStatus = JOB_STATUS_RETIRED;
    m_jobHistoryMutex.unlock();
}

void JobSystem::OnJobCompleted(Job *jobJustExecuted)
{
    totalJobs++;
    m_jobsCompletedMutex.lock();
    m_jobsRunningMutex.lock();

    std::deque<Job *>::iterator runningJobItr = m_jobsRunning.begin();
    for (; runningJobItr != m_jobsRunning.end(); ++runningJobItr)
    {
        if (jobJustExecuted == *runningJobItr)
        {
            m_jobHistoryMutex.lock();
            m_jobsRunning.erase(runningJobItr);
            m_jobsCompleted.push_back(jobJustExecuted);
            m_jobHistory[jobJustExecuted->m_jobID].m_jobStatus = JOB_STATUS_COMPLETED;
            m_jobHistoryMutex.unlock();
            break;
        }
    }
    m_jobsRunningMutex.unlock();
    m_jobsCompletedMutex.unlock();
}

Job *JobSystem::ClaimAJob(unsigned long workerJobFlags)
{
    m_jobsQueuedMutex.lock();
    m_jobsRunningMutex.lock();

    Job *claimedJob = nullptr;
    std::deque<Job *>::iterator queuedJobIter = m_jobsQueued.begin();
    for (; queuedJobIter != m_jobsQueued.end(); ++queuedJobIter)
    {
        Job *queuedJob = *queuedJobIter;
        if ((queuedJob->m_jobChannels & workerJobFlags) != 0)
        {
            claimedJob = queuedJob;
            m_jobHistoryMutex.lock();
            m_jobsQueued.erase(queuedJobIter);
            m_jobsRunning.push_back(queuedJob);
            m_jobHistory[claimedJob->m_jobID].m_jobStatus = JOB_STATUS_RUNNING;
            m_jobHistoryMutex.unlock();
            break;
        }
    }
    m_jobsRunningMutex.unlock();
    m_jobsQueuedMutex.unlock();
    return claimedJob;
}

void JobSystem::registerNewJobType(json input, Job *(*factoryFunction)(json))
{
    std::string newJobType = input["newJobType"].get<std::string>();
    m_jobMap[newJobType] = factoryFunction;
}

void JobSystem::printJobTypes()
{
    std::cout << "Availible job Types: " << std::endl;
    for (auto i : m_jobMap)
    {
        std::cout << i.first << std::endl;
    }
}

void JobSystem::CreateNewJob(json input)
{
    std::string newJobType = input["newJobType"].get<std::string>();
    // Check if the map contains the jobType
    if (m_jobMap.find(newJobType) != m_jobMap.end())
    {
        Job *customJob = m_jobMap[newJobType](input);
        s_jobSystem->QueueJob(customJob);
    }
    else
    {
        std::cout << "ERROR: Job type '" << newJobType << "' not found." << std::endl;
    }
}

void JobSystem::DestroyJob(json input)
{
    int jobID = input["jobID"].get<int>();
    m_jobsQueuedMutex.lock();
    m_jobsRunningMutex.lock();
    m_jobsCompletedMutex.lock();

    // Check completed jobs first
    for (auto completedJobIter = m_jobsCompleted.begin(); completedJobIter != m_jobsCompleted.end(); ++completedJobIter)
    {
        if ((*completedJobIter)->m_jobID == jobID)
        {
            delete *completedJobIter;
            m_jobsCompleted.erase(completedJobIter);

            m_jobsQueuedMutex.unlock();
            m_jobsRunningMutex.unlock();

            m_jobHistoryMutex.lock();
            m_jobHistory[jobID].m_jobStatus = JOB_STATUS_RETIRED;
            m_jobHistoryMutex.unlock();

            m_jobsCompletedMutex.unlock();
            std::cout << "Job " << jobID << " has been destoryed" << std::endl;
            return; // Job found and deleted, exit the function.
        }
    }

    // If the job is not found in completed jobs, unlock the mutexes and handle the case.
    m_jobsQueuedMutex.unlock();
    m_jobsRunningMutex.unlock();
    m_jobsCompletedMutex.unlock();

    // If the job is not found in any of the job queues, it may not exist or is already retired.
    std::cout << "ERROR: Job#" << jobID << " not found or already retired." << std::endl;
}