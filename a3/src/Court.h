#pragma once
#include <pthread.h>
#include <exception>

#include <unistd.h>

using namespace std;

void expect(const bool &b, char const *what = "An error occurred.") {
	if (!b) throw invalid_argument(what);
}

class Court {
	const int playersNeeded;
	const bool refereeNeeded;
	const int totalNeeded = playersNeeded + refereeNeeded;

	// "gate" for entering the court
	// acts like a mutex for enter() while no game is being played
	// not available while game is being played
	sem_t entryway_lock;

	// used if there is no referee: players will synchronize
	pthread_barrier_t matchEnd;
	// used if theres a referee, referee will unlock the turnstile for other players
	sem_t matchEnd_turnstile;

	// mutex for changing playing state
	// when a thread wants to start a match, it must acquire this
	// when a thread wants to check if a match is being played, they must acquire this lock
	sem_t playing_lock;
	// state variables, protected by
	bool playing = false;
	int onCourt_count = 0;

	// id of the referee thread, if exists.
	pthread_t referee;


	public:

	Court(int players, int referee) :
			playersNeeded(players), refereeNeeded(referee) {
		expect(players > 0);
		expect(referee == 1 || referee == 0);


		sem_init(&entryway_lock, 0, 1);
		sem_init(&playing_lock, 0, 1);

		// will be used when players are leave()'ing
		if (referee) sem_init(&matchEnd_turnstile, 0, 0);
		else pthread_barrier_init(&matchEnd, 0, totalNeeded);
	}

	/*
  X player enters
  - player obtains entryway lock
  #if playing
    wait for the current match to end.

  - court player count increments.
  #if playerCount == playersNeeded: 
    match will be started
    this player might become referee
  #else
    - return
    (this player will play(), and then leave())
  */
	void enter() {

		printf("Thread ID: %ld, I have arrived at the court.\n", pthread_self());

		// wait until other player is done entering and no match is being played.
		sem_wait(&entryway_lock);

		sem_wait(&playing_lock);
		// waited until there is no match, i will get on court.
		onCourt_count++;

		// if we have enough players, i will start the match
		// become referee if needed
		if (onCourt_count == playersNeeded + refereeNeeded) {
			printf(
				"Thread ID: %ld, There are enough players, starting a match.\n",
				pthread_self()
			);

			playing = true;
			sem_post(&playing_lock);


			if (refereeNeeded) {
				referee = pthread_self();
				// TODO:
			}


		} else {
			sem_post(&playing_lock);
			printf(
				"Thread ID: %ld, There are only %d players, passing some time.\n",
				pthread_self(),
				onCourt_count
			);
			sem_post(&entryway_lock);
		}
	}

	void leave() {

		// fast path: if entryway is unlocked, there is no match underway, i can just leave
		if (sem_trywait(&entryway_lock) == 0) {
			onCourt_count--;
			printf(
				"Thread ID: %ld, I was not able to find a match and I have to leave "
				"FAST.\n",
				pthread_self()
			);
			sem_post(&entryway_lock);
			return;
		}

		// i will check playing state, therefore need mutex.
		sem_wait(&playing_lock);

		// leave() is unreachable by threads if there is a match that they are not a part of.
		//   => if playing==true, this thread is part of the match.
		if (!playing) {
			// no match underway, i will leave
			onCourt_count--;
			printf(
				"Thread ID: %ld, I was not able to find a match and I have to leave.\n",
				pthread_self()
			);
			sem_post(&playing_lock);
			return;
		} else {	// im trying to leave a match.

			// im done modifying state for now.
			sem_post(&playing_lock);

			if (refereeNeeded) {
				if (pthread_equal(referee, pthread_self())) {	 // im referee
					printf(
						"Thread ID: %ld, I am the referee and now, "
						"match is over. I am leaving.\n",
						pthread_self()
					);

					onCourt_count--; // referee leaves

					// unlock turnstile for other players.
					sem_post(&matchEnd_turnstile);
					
				} else {	// this thread is not the referee
					// wait for unlock from ref (or player leaving before me)
					sem_wait(&matchEnd_turnstile);
					printf(
						"Thread ID: %ld, I am a player and now, I am leaving TURNSTILE.\n",
						pthread_self()
					);

					onCourt_count--;

					/* precondition for last player to leave:
           *   other threads cannot be in leave() when i am in turnstile
					 *     all playing in this match left
					 *     all others are waiting for me to end the match
          */
					if (onCourt_count == 0) {
						printf(
							"Thread ID: %ld, everybody left, letting any waiting people "
							"know.\n",
							pthread_self()
						);

						// mark match ended
						// by precondition, no other thread may be at a point where they access playing.
						playing = false;
						// unlock entryway
						sem_post(&entryway_lock);
						//do NOT unlock turnstile. referee will unlock next time the match ends.
					} else {	// not last: leave turnstile unlocked for players after me
						sem_post(&matchEnd_turnstile);
					}
				}
			} else {
				// players will leave when everyone is done playing
				// TODO:
				throw "unimplemented";
			}
		}
	}

	/**
   * Passing some time is simulated using a play method that will be provided by
   * us when running test cases. For your PA you are not responsible for how a
   * player passes its time in the court. You can assume that if the match started
   * when a player was waiting, it automatically joins the match. Please, declare
   * this method in the class signature but do not implement it.
   */
	void play();
};