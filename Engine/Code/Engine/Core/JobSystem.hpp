#pragma once
#include <vector>
#include <deque>
#include <mutex>
#include <atomic>
#include <functional>

enum class JobType : uint32_t 
{
	GENERAL = 1 << 0,
	FILE_IO = 1 << 1,
};

class Job
{
public:
	virtual ~Job() = default;
	virtual void Execute() = 0;
	JobType m_jobType = JobType::GENERAL;
	std::atomic<bool> m_abortRequested = false;
	void RequestAbort() { m_abortRequested.store(true); }
};

class JobSystem;

class JobWorkerThread
{
public:
	JobWorkerThread(unsigned int id, JobType type, JobSystem* system);
	~JobWorkerThread();
	void Shutdown();

private:
	void ThreadMain();

	unsigned int m_id;
	JobType m_jobType;
	JobSystem* m_jobSystem;

	std::thread* m_thread;
	bool m_isRunning = false;
};

struct JobSystemConfig
{
	unsigned int m_numWorkerThreads = 0;
};

class JobSystem
{
public:
	JobSystem(JobSystemConfig const& config);
	~JobSystem();
	void Startup();
	void Shutdown();
	void AbortAllJobs();
	void ResetAbort() {m_abort = false;}

	bool IsAborting() const{return m_abort;}
	void SubmitJob(Job* job);
	std::vector<Job*> RetrieveCompletedJobs();

	int GetNumPendingJobs();
	int GetNumExecutingJobs();
	int GetNumCompletedJobs();
	int GetNumWorkerThreads() const;
	int GetTotalNumberJobs();

private:
	friend class JobWorkerThread;
	Job* ClaimPendingJob(JobType jobType);
	void MarkJobExecuting(Job* job);
	void MarkJobCompleted(Job* job);

private:
	JobSystemConfig m_config;

	std::deque<Job*> m_pendingJobs;
	std::mutex m_pendingJobsMutex;
	std::deque<Job*> m_executingJobs;
	std::mutex m_executingJobsMutex;
	std::deque<Job*>m_completedJobs;
	std::mutex m_completedJobsMutex;

	std::vector<JobWorkerThread*>m_workers;

	std::condition_variable m_condition;
	bool m_isRunning = false;
	bool m_abort = false;


};

