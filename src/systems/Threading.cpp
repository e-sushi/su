#include <condition_variable>
#include <functional>
namespace amu {

Threader threader;

template<typename R, typename F, typename Fc, typename... Args> void
wrapper_member(std::promise<R> p, F f, Fc fc, Args... args) {
	if constexpr(std::is_void_v<R>) {
		std::invoke(f, fc, args...);
		p.set_value();
	} else {
		p.set_value(std::invoke(f, fc, args...));
	}
}

template<typename R, typename F, typename... Args> void
wrapper(std::promise<R> p, F f, Args... args) {
	threader.sema.acquire();
	if constexpr(std::is_member_function_pointer_v<F>) {
		wrapper_member(std::move(p), f, args...);
	} else {
		if constexpr(std::is_void<R>::value) {
			f(args...);
			p.set_value();
		} else {
			p.set_value(f(args...));
		}
	} 
	threader.sema.release();
}

template<typename F, typename... Args> Future<std::invoke_result_t<F, Args...>> Threader::
start(F f, Args... args) {
	using R = std::invoke_result_t<F, Args...>;
	auto p = std::promise<R>();
	Future<R> out;
	out.f = p.get_future().share();
	std::thread t(wrapper<R, F, Args...>, std::move(p), f, args...);
	t.detach();
	return out;
}

} // namespace amu
