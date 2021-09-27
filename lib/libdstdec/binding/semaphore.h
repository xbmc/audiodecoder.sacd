#ifndef _SEMAPHORE_H_INCLUDED
#define _SEMAPHORE_H_INCLUDED

#include <mutex>
#include <condition_variable>

using std::mutex;
using std::condition_variable;
using std::lock_guard;
using std::unique_lock;

class semaphore {
	mutex m_mtx;
	condition_variable m_cv;
	int m_cnt = 0;
public:
	semaphore() = default;
	semaphore(const semaphore& sem) = delete;
	semaphore(semaphore&& sem) = delete;
	semaphore operator=(const semaphore& sem) = delete;
	void notify() {
		lock_guard<mutex> lock(m_mtx);
		m_cnt++;
		m_cv.notify_one();
	}
	void wait() {
		unique_lock<mutex> lock(m_mtx);
		while (!m_cnt) {
			m_cv.wait(lock);
		}
		m_cnt--;
	}
	bool try_wait() {
		lock_guard<mutex> lock(m_mtx);
		if (m_cnt) {
			m_cnt--;
			return true;
		}
		return false;
	}
};

#endif
