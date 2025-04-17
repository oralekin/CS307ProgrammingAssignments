#ifndef GARAGE_H
#define GARAGE_H

// modified sample: added standard library mutex to all methods.

#include <atomic>
#include <iostream>
#include <mutex>
#include <thread>
#include <unordered_map>

using namespace std;

class Garage {
	private:
	unordered_map<pthread_t, atomic<bool>> flag_map;
	mutex lock;

	public:
	Garage() = default;
	~Garage() = default;

	void setPark() {
		lock_guard<mutex> lg(lock);

		pthread_t current_id = pthread_self();
		atomic<bool> &flag = flag_map[current_id];
		flag = false;
	}

	void park() {
		lock.lock();
		pthread_t current_id = pthread_self();
		atomic<bool> &flag = flag_map[current_id];
		lock.unlock();

		flag.wait(false);
	}

	void unpark(pthread_t id) {
		lock_guard<mutex> lg(lock);

		auto it = flag_map.find(id);
		if (it != flag_map.end()) {
			it->second.store(true);
			it->second.notify_one();
		}
	}
};

#endif
