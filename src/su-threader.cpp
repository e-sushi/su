#define ThreadDebugPrint(threadptr, ...) if(threaddebugprint) log("thread", (threadptr->comment.count ? threadptr->comment : to_string(threadptr)), __VA_ARGS__);

b32 threaddebugprint = true;

template<typename T> struct is_pointer { static const bool value = false; };
template<typename T> struct is_pointer<T*> { static const bool value = true; };

template<typename T> T& deref_if_pointer(T& x){return x;}
template<typename T> T& deref_if_pointer(T* x){return *x;}


enum ThreadState{
    ThreadState_NotInitialized,
    ThreadState_Initializing,
	ThreadState_Close,
	ThreadState_Sleep,
	ThreadState_CallFunction,
};

struct Thread;

template<typename FuncToRun, typename... FuncArgs>
void threadfunc(Thread* me, FuncToRun f, FuncArgs... args);
//persistent thread struct
//the purpose of this is to assign a thread a function it can repeatedly execute without closing
//TODO changing arguments without resetting the function
struct Thread {
	std::thread me;
	std::condition_variable cond;
	std::condition_variable wait;
	std::mutex caller;  //locked by the thread who created this thread while its waiting
	std::mutex waiting; //locked by the thread when its waiting
	ThreadState state;
    string comment;

	int functioncalls = 0;

	void WakeUp() {
#ifdef TRACY_ENABLE
        ZoneScoped;
#endif
		cond.notify_all();
	}

	//causes the calling thread to wait until this one signals that its finished
    //maxTimeToWait is given in milliseconds
    //returns false if the wait timed out
	b32 Wait(u64 timeout = 0) {
#ifdef TRACY_ENABLE
        ZoneScoped;
#endif
        while(state != ThreadState_Sleep) {}
        return true;
        
        //if(state == ThreadState_Sleep) return true;
        //std::unique_lock<std::mutex> lock(caller);
        //if(timeout) 
        //    return (wait.wait_for(lock, std::chrono::milliseconds(timeout)) == std::cv_status::timeout ? false : true);
        //else wait.wait(lock);
        //return true;
	}

    //attempts to run the threads function immediately, but returns early if it can't
	void Run(int count = 1) {
#ifdef TRACY_ENABLE
        ZoneScoped;
#endif
		if (state != ThreadState_Sleep) return; //thread is already running
		functioncalls = count;
		state = ThreadState_CallFunction;
		WakeUp();
	}

    //waits until the the thread finishes executing before telling it to run again
    void WaitToRun(int count = 1){
#ifdef TRACY_ENABLE
        ZoneScoped;
#endif
        Wait();
        Run(count);
    }

	//pauses the calling thread until this one has finished executing
	void RunAndWait(int count = 1) {
#ifdef TRACY_ENABLE
        ZoneScoped;
#endif
		Run(count);
		Wait();
	}

    void WaitToRunAndWait(int count = 1){
#ifdef TRACY_ENABLE        
        ZoneScoped;
#endif
        Wait();
        RunAndWait(count);
    }

    //causes the thread to return
	void Close() {
#ifdef TRACY_ENABLE
        ZoneScoped;
#endif
		state = ThreadState_Close;
		WakeUp();
	}

    //sets the function that the thread calls 
    //TODO maybe theres a way around closing and reopening the thread? probably not with templating
	template<typename FuncToRun, typename... FuncArgs>
	void SetFunction(FuncToRun f, FuncArgs...args) {
#ifdef TRACY_ENABLE
        ZoneScoped;
#endif        
        Close(); state = ThreadState_Initializing;
		me = std::thread(threadfunc<FuncToRun, FuncArgs...>, this, f, args...);
        me.detach();
	}
    //sets the function that the thread calls and waits until its done initializing
	template<typename FuncToRun, typename... FuncArgs>
	void SetFunctionAndWait(FuncToRun f, FuncArgs...args) {
#ifdef TRACY_ENABLE
        ZoneScoped;
#endif
		Close(); state = ThreadState_Initializing;
		me = std::thread(threadfunc<FuncToRun, FuncArgs...>, this, f, args...);
        me.detach();
		Wait();
	}

	~Thread() {
        if (state == ThreadState_NotInitialized) return;
		if (state != ThreadState_Sleep) Wait();
		Close();
        Wait();    
    }
};

template<typename FuncToRun, typename... FuncArgs>
void threadfunc(Thread* me, FuncToRun f, FuncArgs... args) {
#ifdef TRACY_ENABLE
    ZoneScoped;
#endif
    tracy::SetThreadName(me->comment.str);
	ThreadDebugPrint(me, " has been created.");
	while (me->state != ThreadState_Close) {
#ifdef TRACY_ENABLE
        ZoneScoped;
#endif
		if(me->state==ThreadState_CallFunction){
#ifdef TRACY_ENABLE
            ZoneScoped; 
#endif
            ThreadDebugPrint(me, " is calling its function ", me->functioncalls, " times"); 
            while (me->functioncalls--) { f(deref_if_pointer(args)...); }
        }
		me->state = ThreadState_Sleep;
		//me->wait.notify_all();
        ThreadDebugPrint(me, " is going to sleep");
		std::unique_lock<std::mutex> lock(me->waiting);
		me->cond.wait(lock);
	}
	ThreadDebugPrint(me, " is now closing");
}

struct ThreadManager{
    set<Thread*> threads;
    Arena thread_arena;

    Thread* MakeNewThread(const string& comment = ""){
#ifdef TRACY_ENABLE
        ZoneScoped;
#endif
        if(!thread_arena.data) thread_arena.init(sizeof(Thread)*max_threads);
        if(threads.count >= max_threads) return 0;
        Thread* nu = (Thread*)thread_arena.add(Thread());
        if(comment.count) nu->comment = comment;
        threads.add(nu,nu);
        return nu;
    }

    void StopAndDeleteThread(Thread* thread){
#ifdef TRACY_ENABLE
        ZoneScoped;
#endif
        if(Thread** tcheck = threads.at(thread); !tcheck) return; //maybe assert here?
        thread->Close();
        threads.remove(thread);
    }

    void DeleteThread(Thread* thread){
#ifdef TRACY_ENABLE
        ZoneScoped;
#endif
        threads.remove(thread); //no need to check if it exists here
    }

    void WaitForAllThreadsToFinish(u64 timeout = 0){
#ifdef TRACY_ENABLE
        ZoneScoped;
#endif
        for(Thread* t : threads) t->Wait(timeout);
    }

    ~ThreadManager(){
        for(Thread* t : threads) if(t) t->~Thread();
    }
};

ThreadManager tmanager;