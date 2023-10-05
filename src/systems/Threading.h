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

	Threader() : sema(2) {}
};

extern Threader threader;

// small wrapper around std::shared_future that works with our sema setup 
template<typename T>
struct Future {
	std::shared_future<T> f;

	T get() {
		threader.sema.release();
		T val = f.get();
		threader.sema.acquire();
		return val;
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
