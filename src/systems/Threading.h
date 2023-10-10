/*

	System for managing concurrency in amu. 

	TODO
	-----
	This is really bulky atm, we should probably consider moving towards using something like fibers in some places.

*/

#ifndef AMU_THREADING_H
#define AMU_THREADING_H

#include <condition_variable>
#include <mutex>
#include <semaphore>
namespace amu {

template<typename T>
struct Future;

template<typename R, typename F, typename... Args> void
wrapper(std::promise<R> p, F f, Args... args);

struct Threader {
	std::counting_semaphore<2> sema;

	template<typename F, typename... Args> Future<std::invoke_result_t<F, Args...>>
	start(F f, Args... args);

	// allows passing a Future which will be immediately waited on by the thread 
	// the promise associated with the future may be used to wake it up
	template<typename F, typename... Args> Future<std::invoke_result_t<F, Args...>>
	start_deferred(Future<void> fext, F f, Args... args);

	Threader() : sema(2) {}
};

extern Threader threader;

// small wrapper around std::shared_future that works with our sema setup 
template<typename T>
struct Future {
	std::shared_future<T> f;

	template<typename U = T>
	std::enable_if_t<std::is_void_v<U>> 
	get() { 
		threader.sema.release();
		f.get();
		threader.sema.acquire();
	}

	template<typename U = T>
	std::enable_if_t<!std::is_void_v<U>, U>
	get() { 
		threader.sema.release();
		T val = f.get();
		threader.sema.acquire();
		return val;
	}
};

// small wrapper around std's cond vars 
// that integrates with our sema setup
struct ConditionVariable {
	std::condition_variable_any cv;

	template<typename T> void
	wait(T& ul) { 
		threader.sema.release();
		cv.wait(ul);
		threader.sema.acquire();
	}

	void
	notify_all() { 
		cv.notify_all();
	}
};

struct Mutex {
	std::mutex mtx;

	void
	lock() {
		threader.sema.release();
		mtx.lock();
		threader.sema.acquire();
	}

	void
	unlock() {
		mtx.unlock();
	}
};

// TODO(sushi) come back and implement a custom semaphore later 
//             so we can decide at runtime how many threads may be active
//struct Semaphore {
//	s64 max;
//	std::atomic<s64> val;
//
//	std::mutex m;
//	std::condition_variable cv;
//
//	void acquire() {
//		if(--val < 0) {
//			do {
//				auto ul = std::unique_lock(m);
//				cv.wait(ul);
//			} while(1);
//
//		}
//	}
//};

} // namespace amu

#endif // AMU_THREADING_H
