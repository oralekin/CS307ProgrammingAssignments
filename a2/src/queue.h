#pragma once
#include <iostream>
#include <mutex>
#include <sstream>
#include <vector>
using namespace std;

template<class T>
class SingleLockQueue {
	private:
	struct Node {
		T value;
		Node *next;
		Node(T _v) : value(_v), next(nullptr) {};
		Node(T _v, Node *_next) : Node(_v) {
			next = _next;
		};
	};

	mutex mut;
	// will dequeue from head
	Node *head = nullptr;
	// will enqueue to tail
	Node *tail = nullptr;

	public:
	const SingleLockQueue<T> &operator=(const SingleLockQueue<T> &rhs) = delete;

	void enqueue(T item) {
		lock_guard<mutex> lg(mut);

		if (!tail) {
			// this is tail and also head!
			head = tail = new Node(item);
		} else {
			// add to tail
			tail->next = new Node(item);
			tail = tail->next;
		}
		return;
	};

	T dequeue() {
		if (isEmpty()) throw "queue empty";

		lock_guard<mutex> lg(mut);
		// head isnt nullptr! (because im not empty)
		// save current head
		Node *ret = head;
		// progress head
		head = head->next;
		// if head becomes nullptr, we are at the end of queue
		// => tail was pointing to the element that just got removed.
		if (!head) tail = head;
		return ret->value;
	};

	bool isEmpty() {
		lock_guard<mutex> lg(mut);
		return head == nullptr;
	};

	void print() {
		lock_guard<mutex> lg(mut);

		stringstream ss;

		Node *cur = head;
		if (!cur) ss << "Empty";
		else
			while (cur) {
				ss << cur->value << " ";
				cur = cur->next;
			}
		ss << endl;
		printf("%s", ss.str().c_str());
	}
};

template<class T>
class MichaelScottQueue {
	private:
	struct Node {
		T value;
		Node *next;
		Node() : next(nullptr) {};
		Node(T _v) : value(_v), next(nullptr) {};
		Node(T _v, Node *_next) : Node(_v) {
			next = _next;
		};
	};

	mutex headLock;
	mutex tailLock;
	// will dequeue from head
	Node *dummyHead = new Node();
	// will enqueue to tail
	Node *tail = dummyHead;

	public:
	const SingleLockQueue<T> &operator=(const SingleLockQueue<T> &rhs) = delete;

	void enqueue(T item) {
		Node *newNode = new Node(item);

		lock_guard<mutex> lg(tailLock);
		tail->next = newNode;
		tail = newNode;
	};

	T dequeue() {
		lock_guard<mutex> lg(headLock);
		Node *toDelete = dummyHead;
		Node *newDummy = toDelete->next;

		if (!newDummy) throw "queue empty";

		delete toDelete;
		dummyHead = newDummy;

		return newDummy->value;
	};

	bool isEmpty() {
		lock_guard<mutex> lg(headLock);
		return dummyHead->next == nullptr;
	};

	void print() {
		lock_guard<mutex> lg(headLock);

		stringstream ss;

		Node *cur = dummyHead->next;
		if (!cur) ss << "Empty";
		else
			while (cur) {
				ss << cur->value << " ";
				cur = cur->next;
			}
		ss << endl;
		printf("%s", ss.str().c_str());
	}
};

// use Michael Scott
template<class T>
using Queue = MichaelScottQueue<T>;