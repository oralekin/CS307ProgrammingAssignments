mutex:
	atomic bool guard
		for spinlocking mutex operations.
	atomic bool lock
		is this mutex owned.
	
	garage
		parked stuff
	
	mlfq
		i should implement this seperately, as a concurrent structure

	lock
		check lock by CAS:
			if unowned, return.
		


mlfq
	array of concurrent queues.
	the queues are concurrent already, so i shouldnt need to lock the MLFQ itself?
