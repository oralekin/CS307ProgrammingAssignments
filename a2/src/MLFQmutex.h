#pragma once
#include <chrono>
#include <cmath>
#include <optional>
#include <unordered_map>
#include "park.h"
#include "queue.h"

using namespace std;

/**
 * class implementing multi-level queue
 * supports updating item-priority map given quant and time
 * uses locking to guard the underlying map.
 */
template<class T>
class MLFQ {
	// vector of concurrent queues.
	vector<Queue<T>> queues;
	unordered_map<T, int> priorities;

	public:
	MLFQ(int levels) : queues(levels) {}

	/**
	 * enqueues item in the right prio level.
	 * returns the priority of the queue this item was added to.
	 */
	int enqueue(T item) {
		// look item up in priority table
		int prio;
		// 0 by default int constructor.
		prio = priorities[item];
		// enqueue.
		printf("Adding thread with ID: %ld to level %d\n", item, prio);
		queues[prio].enqueue(item);
		return prio;
	}

	/**
	 * updates the priority of an item in the map.
	 * new priority is only used when the item is enqueued next.
	 */
	template<class tr, class tp, class qr, class qp>
	int updatePrio(
		T item,
		chrono::duration<tr, tp> time,
		chrono::duration<qr, qp> quantum
	) {
		int new_prio =
			min<int>(priorities[item] + (time / quantum), queues.size() - 1);
		priorities[item] = new_prio;
		return new_prio;
	};

	/**
	 * returns an item if exists, none if MLFQ empty.
	 */
	optional<T> dequeue() {
		for (Queue<T> &queue : queues)
			if (!queue.isEmpty()) return queue.dequeue();
		return {};
	}

	void print() {
		printf("Waiting threads :\n");
		for (int i = 0; i < queues.size(); i++) {
			printf("Level %d: ", i);
			queues[i].print();
		}
	}
};

class SpinLocked {
	chrono::duration<double> quantum;

	MLFQ<pthread_t> queues;
	Garage garage;


	atomic_flag guard;
	atomic_flag flag;

	chrono::time_point<chrono::steady_clock> timerStart;

	public:
	SpinLocked(int levels, double quantum) : quantum(quantum), queues(levels) {};

	const SpinLocked &operator=(const SpinLocked &) = delete;

	/**
	 * blocks until obtains mutex.
	 */
	void lock() {
		// check mutex by TAS, can get it without work if it was unowned.
		if (flag.test_and_set()) {
			spinForGuard();
			queues.enqueue(pthread_self());
			garage.setPark();
			guard.clear();
			garage.park();
		}

		startTimer();
	};

	/**
	 * releases lock
	 *  assumption: only lock owner can call this
	 */
	void unlock() {
		endTimer();
		spinForGuard();
		auto next = queues.dequeue();

		// if theres a waiting thread, hand the mutex to it.
		// otherwise, release mutex
		if (next) garage.unpark(*next);
		else flag.clear();

		guard.clear();
	};

	void print() {
		queues.print();
	}

	private:
	/**
	 * blocks until guard is found false and obtained.
	 */
	void spinForGuard() {
		while (guard.test_and_set())
			// spin
			;
	}

	void startTimer() {
		timerStart = chrono::steady_clock::now();
	}

	void endTimer() {
		auto timerEnd = chrono::steady_clock::now();
		// we need guard here because we're using the mlfq
		spinForGuard();
		queues.updatePrio(pthread_self(), timerEnd - timerStart, quantum);
		guard.clear();
	}
};

using MLFQMutex = SpinLocked;