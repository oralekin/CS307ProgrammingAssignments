#include "queue.h"
#include "park.h"
#include "MLFQmutex.h"

// for testing, written by chatgpt
#include <stdexcept>
int main()
{
  Queue<int> q;
  cout << "ok" << endl;

  std::cout << "Initial state: isEmpty = " << (q.isEmpty() ? "true" : "false") << std::endl;
  // Expected: true

  std::cout << "Enqueue 10" << std::endl;
  q.enqueue(10);
  std::cout << "Enqueue 20" << std::endl;
  q.enqueue(20);
  std::cout << "Enqueue 30" << std::endl;
  q.enqueue(30);

  std::cout << "Queue after enqueues: ";
  q.print(); // Expected: 10 20 30

  std::cout << "isEmpty = " << (q.isEmpty() ? "true" : "false") << std::endl;
  // Expected: false

  std::cout << "Dequeue: " << q.dequeue() << std::endl;
  // Expected: 10

  std::cout << "Queue after 1 dequeue: ";
  q.print(); // Expected: 20 30

  std::cout << "Dequeue: " << q.dequeue() << std::endl;
  // Expected: 20

  std::cout << "Queue after 2 dequeues: ";
  q.print(); // Expected: 30

  std::cout << "Dequeue: " << q.dequeue() << std::endl;
  // Expected: 30

  std::cout << "Queue after 3 dequeues: ";
  q.print(); // Expected: Empty

  std::cout << "Enqueue 40" << std::endl;
  q.enqueue(40);
  std::cout << "Queue after enqueue 40: ";
  q.print(); // Expected: 40

  std::cout << "Enqueue 50" << std::endl;
  q.enqueue(50);
  std::cout << "Queue after enqueue 50: ";
  q.print(); // Expected: 50

  std::cout << "Dequeue: " << q.dequeue() << std::endl;
  // Expected: 40
  std::cout << "Dequeue: " << q.dequeue() << std::endl;
  // Expected: 50

  std::cout << "isEmpty = " << (q.isEmpty() ? "true" : "false") << std::endl;
  // Expected: true

  // Try to dequeue from an empty queue
  std::cout << "Attempting to dequeue from empty queue..." << std::endl;
  try
  {
    q.dequeue();
  }
  catch (char const *e)
  {
    std::cout << "Caught exception: " << e << std::endl;
    // Expected: Caught exception: Queue is empty (or whatever your exception message is)
  }

  return 0;
}