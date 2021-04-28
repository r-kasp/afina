#ifndef AFINA_CONCURRENCY_EXECUTOR_H
#define AFINA_CONCURRENCY_EXECUTOR_H

#include <chrono>
#include <iostream>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

namespace Afina {
namespace Concurrency {

/**
 * # Thread pool
 */
class Executor {
    enum class State {
        // Threadpool is fully operational, tasks could be added and get executed
        kRun,

        // Threadpool is on the way to be shutdown, no ned task could be added, but existing will be
        // completed as requested
        kStopping,

        // Threadppol is stopped
        kStopped
    };

    Executor(std::string name, int _low_watermark = 1, int _hight_watermark = 2,
    		int _max_queue_size = 100, int _idle_time = 1000)
    {
    	low_watermark = _low_watermark;
    	hight_watermark = _hight_watermark;
    	max_queue_size = _max_queue_size;
    	idle_time = _idle_time;
    	cur_work = 0;
    	cur_threads = 0;
    	
    	std::unique_lock<std::mutex> lock(mutex);
    	for (int i = 0; i < low_watermark; i++)
    	{
    		cur_threads++;
    		threads.push_back( std::thread([this] {return perform(this);} ) );
    	}
    	state = State::kRun;
    }
    
    ~Executor() { Stop(true); }

    /**
     * Signal thread pool to stop, it will stop accepting new jobs and close threads just after each become
     * free. All enqueued jobs will be complete.
     *
     * In case if await flag is true, call won't return until all background jobs are done and all threads are stopped
     */
    void Stop(bool await = false)
    {
    	std::unique_lock<std::mutex> lock(mutex);
    	if (state != State::kStopped)
    	{
    		if (cur_work == 0)
				state = State::kStopped;
			else
    			state = State::kStopping;
			empty_condition.notify_all();
			while (state != State::kStopped)
			{
				stop_condition.wait(lock);
			}
		}
    }

    /**
     * Add function to be executed on the threadpool. Method returns true in case if task has been placed
     * onto execution queue, i.e scheduled for execution and false otherwise.
     *
     * That function doesn't wait for function result. Function could always be written in a way to notify caller about
     * execution finished by itself
     */
    template <typename F, typename... Types> bool Execute(F &&func, Types... args) {
        // Prepare "task"
        auto exec = std::bind(std::forward<F>(func), std::forward<Types>(args)...);

        std::unique_lock<std::mutex> lock(this->mutex);
        if (state != State::kRun) {
            return false;
        }

        // Enqueue new task
        
        if (threads.size() < hight_watermark && threads.size() == cur_work)
        {
        	threads.push_back( std::thread([this] {return perform(this);} ) );
        	cur_threads++;
	}
        
        tasks.push_back(exec);
        empty_condition.notify_one();
        return true;
    }

private:
	int size = 0;
	int low_watermark = 0;
	int hight_watermark = 0;
	int max_queue_size = 0;
	int idle_time = 0;
	int cur_work = 0;
	int cur_threads = 0;

    // No copy/move/assign allowed
    Executor(const Executor &);            // = delete;
    Executor(Executor &&);                 // = delete;
    Executor &operator=(const Executor &); // = delete;
    Executor &operator=(Executor &&);      // = delete;

    /**
     * Main function that all pool threads are running. It polls internal task queue and execute tasks
     */
    friend void perform(Executor *executor)
    {
    	std::unique_lock<std::mutex> lock(executor->mutex);
    	bool flag_time = false;
    	while (executor->state == Executor::State::kRun || !executor->tasks.empty())
    	{    	
    		auto time = std::chrono::steady_clock::now();
    		while (executor->tasks.empty())
		{
			auto wake_up = executor->empty_condition.wait_until(lock, time + std::chrono::milliseconds(executor->idle_time));
			if (executor->state != Executor::State::kRun || 
			(executor->threads.size() > executor->low_watermark && executor->tasks.empty() && wake_up == std::cv_status::timeout))
			{
				flag_time = true;
				break;
			}
		}		
		if (flag_time)
			break;

		auto task = executor->tasks.front();
		executor->tasks.pop_front();
		executor->cur_work++;

		lock.unlock();

		try
		{
			task();
		}
		catch (...)
		{
			std::cout << "Error" << std::endl;
		}

		lock.lock();
		executor->cur_work--;
    	}
    	
    	executor->cur_threads--;
	if (executor->cur_threads == 0 && executor->state != Executor::State::kRun)
	{
		executor->state = Executor::State::kStopped;
		executor->stop_condition.notify_all();
	}
    }

    /**
     * Mutex to protect state below from concurrent modification
     */
    std::mutex mutex;

    /**
     * Conditional variable to await new data in case of empty queue
     */
    std::condition_variable empty_condition;
    
    std::condition_variable stop_condition;

    /**
     * Vector of actual threads that perorm execution
     */
    std::vector<std::thread> threads;

    /**
     * Task queue
     */
    std::deque<std::function<void()>> tasks;

    /**
     * Flag to stop bg threads
     */
    State state;
};

} // namespace Concurrency
} // namespace Afina

#endif // AFINA_CONCURRENCY_EXECUTOR_H
