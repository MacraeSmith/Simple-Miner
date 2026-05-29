#include "Engine/Core/JobSystem.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Math/MathUtils.hpp"

//Job System
//--------------------------------------------------------------------------------------
JobSystem::JobSystem(JobSystemConfig const& config)
	:m_config(config)
	,m_isRunning(false)
{

}

JobSystem::~JobSystem() 
{
	for (int i = 0; i < (int)m_workers.size(); ++i)
	{
		delete(m_workers[i]);
		m_workers[i] = nullptr;
	}
}

void JobSystem::Startup()
{
	m_isRunning = true;
	m_abort = false;

	int maxNumWorkers = (int)(std::thread::hardware_concurrency() - 5);
	m_config.m_numWorkerThreads = (unsigned int)GetClampedInt(m_config.m_numWorkerThreads, 1, maxNumWorkers);
	const int NUM_WORKERS = m_config.m_numWorkerThreads;
	m_workers.reserve(NUM_WORKERS);
	for (int i = 0; i < NUM_WORKERS; ++i)
	{
		m_workers.push_back(new JobWorkerThread((unsigned int)i, JobType::GENERAL, this));
	}

}

void JobSystem::Shutdown()
{
	m_isRunning = false;
	m_condition.notify_all();

	for (int i = 0; i < (int)m_workers.size(); ++i)
	{
		if (m_workers[i])
		{
			m_workers[i]->Shutdown();
		}
	}
}

void JobSystem::AbortAllJobs()
{
	{
		std::scoped_lock<std::mutex> lock(m_pendingJobsMutex);

		// mark abort state
		m_abort = true;

		// free all pending jobs immediately (optional depending on your needs)
		for (Job* job : m_pendingJobs)
		{
			delete job;   // or move them into an "aborted" list
		}
		m_pendingJobs.clear();
	}

	// Abort all executing jobs too
	{
		std::scoped_lock<std::mutex> lockExec(m_executingJobsMutex);
		for (Job* job : m_executingJobs)
		{
			job->RequestAbort();
		}
	}

	// wake all worker threads so they can exit
	m_condition.notify_all();
}


void JobSystem::SubmitJob(Job* job)
{
	std::scoped_lock<std::mutex> lock(m_pendingJobsMutex);
	m_pendingJobs.push_back(job);
	m_condition.notify_one();
}

std::vector<Job*> JobSystem::RetrieveCompletedJobs()
{
	std::scoped_lock<std::mutex> lock(m_completedJobsMutex);
	std::vector<Job*> retiringJobs;

	if(m_completedJobs.size() < 1)
		return retiringJobs;

	retiringJobs.reserve(m_completedJobs.size());

	while (!m_completedJobs.empty())
	{
		Job* job = m_completedJobs.front();
		retiringJobs.push_back(job);
		m_completedJobs.pop_front();
	}

	return retiringJobs;
}

int JobSystem::GetNumPendingJobs()
{
	std::scoped_lock<std::mutex> lock(m_pendingJobsMutex);
	return (int)m_pendingJobs.size();
}

int JobSystem::GetNumExecutingJobs()
{
	std::scoped_lock<std::mutex> lock(m_executingJobsMutex);
	return (int)m_executingJobs.size();
}

int JobSystem::GetNumCompletedJobs()
{
	std::scoped_lock<std::mutex> lock(m_completedJobsMutex);
	return (int)m_completedJobs.size();
}

int JobSystem::GetNumWorkerThreads() const
{
	return (int)m_workers.size();
}

int JobSystem::GetTotalNumberJobs()
{
	int numJobs = 0;
	numJobs += GetNumPendingJobs();
	numJobs += GetNumExecutingJobs();
	numJobs += GetNumCompletedJobs();
	return numJobs;
}


Job* JobSystem::ClaimPendingJob(JobType jobType)
{
	UNUSED(jobType);
	//does not need mutex locked because this function is called in ThreadWorker::Main() and is locked before called

	if (m_pendingJobs.empty())
		return nullptr;

	Job* job = m_pendingJobs.front();
	m_pendingJobs.pop_front();
	MarkJobExecuting(job);
	return job;
}

void JobSystem::MarkJobExecuting(Job* job)
{
	std::scoped_lock<std::mutex> lock(m_executingJobsMutex);
	m_executingJobs.push_back(job);
}

void JobSystem::MarkJobCompleted(Job* job)
{
	{
		std::scoped_lock<std::mutex> lock(m_completedJobsMutex);
		m_completedJobs.push_back(job);
	}
	
	{
		std::scoped_lock<std::mutex> lock(m_executingJobsMutex);
		auto it = std::find(m_executingJobs.begin(), m_executingJobs.end(), job);
		if (it != m_executingJobs.end())
		{
			m_executingJobs.erase(it);
		}

		else
		{
			ERROR_AND_DIE("Completed Job could not be found in executing list");
		}
	}
}


//Job Worker Thread
//--------------------------------------------------------------------------------------
JobWorkerThread::JobWorkerThread(unsigned int id, JobType type, JobSystem* system)
	:m_id(id)
	,m_jobType(type)
	,m_jobSystem(system)
{
	m_isRunning = true;
	m_thread = new std::thread(&JobWorkerThread::ThreadMain, this);
}

JobWorkerThread::~JobWorkerThread()
{
	delete m_thread;
	m_thread = nullptr;
}


void JobWorkerThread::Shutdown()
{
	m_isRunning = false;
	if (m_thread->joinable())
	{
		m_thread->join();
	}
}


void JobWorkerThread::ThreadMain()
{
	while (m_jobSystem->m_isRunning)
	{
		Job* job = nullptr;
		{
			std::unique_lock<std::mutex> lock(m_jobSystem->m_pendingJobsMutex);

			if(m_jobSystem->m_abort)
				break;

			if (m_jobSystem->m_isRunning && !m_jobSystem->m_pendingJobs.empty())
			{
				job = m_jobSystem->ClaimPendingJob(m_jobType);
			}

			else
			{
				m_jobSystem->m_condition.wait(lock, [this]
					{
						return !m_jobSystem->m_pendingJobs.empty() || !m_jobSystem->m_isRunning || m_jobSystem->m_abort;
					});

				if (m_jobSystem->m_abort)
					break;

				if (!m_jobSystem->m_isRunning && m_jobSystem->m_pendingJobs.empty())
					break;

				job = m_jobSystem->ClaimPendingJob(m_jobType);
			}
			
		}

		if (job)
		{
			job->Execute();
			m_jobSystem->MarkJobCompleted(job);
			std::this_thread::yield();
		}
	}
}
