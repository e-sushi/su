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
//TODO clean up redundent things in this struct
struct Thread {
	std::thread me;
	std::condition_variable ThreadCondition;
	std::condition_variable CallingThreadCondition;
	std::mutex caller;  //locked by the thread who created this thread while its waiting
	std::mutex waiting; //locked by the thread when its waiting
	ThreadState state;
    string comment;

	int functioncalls = 0;

	void WakeUp() {
        ZoneScoped;
		ThreadCondition.notify_all();
	}

	//this funciton locks the caller mutex and waits until CallingThreadCondition receieves
	//a signal to unblock
	b32 Wait(u64 timeout = 0) {
        ZoneScoped;
        if(state == ThreadState_Sleep) return true;
        std::unique_lock<std::mutex> lock(caller);
        if(timeout) 
            return (CallingThreadCondition.wait_for(lock, std::chrono::milliseconds(timeout)) == std::cv_status::timeout ? false : true);
        else CallingThreadCondition.wait(lock);
        return true;
	}

    //attempts to run the threads function immediately, but returns early if it can't
	void Run(int count = 1) {
        ZoneScoped;
		if (state != ThreadState_Sleep) return; //thread is already running
		functioncalls = count;
		state = ThreadState_CallFunction;
		WakeUp();
	}

    //waits until the the thread finishes executing before telling it to run again
    void WaitToRun(int count = 1){
        ZoneScoped;
        Wait();
        Run(count);
    }

	//pauses the calling thread until this one has finished executing
	void RunAndWait(int count = 1) {
        ZoneScoped;
		Run(count);
		Wait();
	}

    void WaitToRunAndWait(int count = 1){
        ZoneScoped;
        Wait();
        RunAndWait(count);
    }

    //causes the thread to return
	void Close() {
        ZoneScoped;
		state = ThreadState_Close;
		WakeUp();
	}

	//waits for the thread to return, this means you must know the thread is active to begin with!
	void CloseAndJoin(){
        ZoneScoped;
		state = ThreadState_Close;
		WakeUp();
		if(me.joinable()) me.join();
	}

    //sets the function that the thread calls 
    //TODO maybe theres a way around closing and reopening the thread? probably not with templating
	template<typename FuncToRun, typename... FuncArgs>
	void SetFunction(FuncToRun f, FuncArgs...args) {
        ZoneScoped;
        Close(); state = ThreadState_Initializing;
		me = std::thread(threadfunc<FuncToRun, FuncArgs...>, this, f, args...);
	}
    //sets the function that the thread calls and waits until its done initializing
	template<typename FuncToRun, typename... FuncArgs>
	void SetFunctionAndWait(FuncToRun f, FuncArgs...args) {
        ZoneScoped;
		Close(); state = ThreadState_Initializing;
		me = std::thread(threadfunc<FuncToRun, FuncArgs...>, this, f, args...);
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
    ZoneScoped;
    tracy::SetThreadName(me->comment.str);
	ThreadDebugPrint(me, " has been created.");
	while (me->state != ThreadState_Close) {
		ZoneScopedN("Thread is awake");
		if(me->state==ThreadState_CallFunction){
			ZoneScopedN("Thread calls function");
            ThreadDebugPrint(me, " is calling its function ",  me->functioncalls, " times"); 
            while (me->functioncalls--) { f(deref_if_pointer(args)...); }
        }
        ThreadDebugPrint(me, " is going to sleep");
		me->state = ThreadState_Sleep;
		me->CallingThreadCondition.notify_all();
		std::unique_lock<std::mutex> lock(me->waiting);
		while(me->state == ThreadState_Sleep) 
			me->ThreadCondition.wait(lock);
		ThreadDebugPrint(me, " has woken up");
	}
	ThreadDebugPrint(me, " is now closing");
	me->CallingThreadCondition.notify_all();
	std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

struct ThreadManager{
    set<Thread*> threads;
    Arena thread_arena;

    Thread* MakeNewThread(const string& comment = ""){
        ZoneScoped;
        if(!thread_arena.data) thread_arena.init(sizeof(Thread)*max_threads);
        if(threads.count >= max_threads) return 0;
        Thread* nu = (Thread*)thread_arena.add(Thread());
        if(comment.count) nu->comment = comment;
        threads.add(nu,nu);
        return nu;
    }

    void StopAndDeleteThread(Thread* thread){
        ZoneScoped;
        if(Thread** tcheck = threads.at(thread); !tcheck) return; //maybe assert here?
        thread->Close();
        threads.remove(thread);
    }

	
    void StopAndDeleteThreadAndWait(Thread* thread){
        ZoneScoped;
        if(Thread** tcheck = threads.at(thread); !tcheck) return; //maybe assert here?
        thread->Close();
		thread->Wait();
        threads.remove(thread);
    }

	  void StopAndDeleteAllThreads(){
        ZoneScoped;
        for(Thread* t : threads){
        	t->Close();
		}
		threads.clear();
    }

	
    void StopAndDeleteAllThreadsAndWait(){
        ZoneScoped;
       	for(Thread* t : threads){
        	t->CloseAndJoin();
		}
		threads.clear();
    }


    void DeleteThread(Thread* thread){
        ZoneScoped;
        threads.remove(thread); //no need to check if it exists here
    }

	void CloseAllThreads(){
		for(Thread* t : threads) t->Close();
	}

    void WaitForAllThreadsToFinish(u64 timeout = 0){
        ZoneScoped;
        for(Thread* t : threads) t->Wait(timeout);
    }

    ~ThreadManager(){
        
    }

} tmanager;
