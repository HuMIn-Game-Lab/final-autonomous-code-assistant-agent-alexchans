#pragma once
#include <mutex>
#include <map>
#include <deque>
#include <vector>
#include <thread>
#include "CompileJob.h"
using json = nlohmann::json;

constexpr int JOB_TYPE_ANY = -1;

class JobWorkerThread;

enum JobStatus
{
    JOB_STATUS_NEVER_SEEN,
    JOB_STATUS_QUEUED,
    JOB_STATUS_RUNNING,
    JOB_STATUS_COMPLETED,
    JOB_STATUS_RETIRED,
    NUM_JOB_STATUSES
};

struct JobHistoryEntry
{
    JobHistoryEntry(int jobType, JobStatus jobStatus)
        : m_jobType(jobType), m_jobStatus(jobStatus) {}

    int m_jobType = -1;
    int m_jobStatus = JOB_STATUS_NEVER_SEEN;
};

class Job;

class JobSystem
{
    friend class JobWorkerThread;

public:
    JobSystem();
    ~JobSystem();

    static JobSystem *CreateOrGet();
    static void Destroy();
    int totalJobs = 0;

    void CreateWorkerThread(json input);
    void DestroyWorkerThread(const char *uniqueName);
    void QueueJob(Job *job);

    // Status Queries
    JobStatus GetJobStatus(json input) const;
    bool IsJobComplete(json input) const;

    void FinishCompletedJobs();
    void FinishJob(json input);

    void registerNewJobType(json input, Job *(*factoryFunction)(json input));
    void printJobTypes();
    void CreateNewJob(json input);
    void DestroyJob(json input);

private:
    Job *ClaimAJob(unsigned long workerJobFlags);
    void OnJobCompleted(Job *jobJustExecuted);

    static JobSystem *s_jobSystem;

    std::vector<JobWorkerThread *> m_workerThreads;
    mutable std::mutex m_workerThreadsMutex;
    std::deque<Job *> m_jobsQueued;
    std::deque<Job *> m_jobsRunning;
    std::deque<Job *> m_jobsCompleted;
    mutable std::mutex m_jobsQueuedMutex;
    mutable std::mutex m_jobsRunningMutex;
    mutable std::mutex m_jobsCompletedMutex;

    std::vector<JobHistoryEntry> m_jobHistory;
    mutable int m_jobHistoryLowestActiveIndex = 0;
    mutable std::mutex m_jobHistoryMutex;

    std::map<std::string, Job *(*)(json)> m_jobMap;
};