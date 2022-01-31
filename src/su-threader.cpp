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

const char* ThreadStateStrs[] = {
    "ThreadState_NotInitialized",
    "ThreadState_NotInitialized",
    "ThreadState_Initializing",
    "ThreadState_Close",
    "ThreadState_CallFunction",
    "ThreadState_Sleep",
};

struct Thread;
Thread* last_returned = 0;
std::condition_variable manager_wait;
std::mutex manager_mute;
template<typename FuncToRun, typename... FuncArgs>
void threadfunc(Thread* me, FuncToRun f, FuncArgs... args);
//persistent thread struct
//the purpose of this is to assign a thread a function it can repeatedly execute without closing
//TODO clean up redundent things in this struct
struct Thread {
	std::thread me;
	std::condition_variable_any ThreadCondition;
	std::condition_variable_any CallingThreadCondition;
	TracyLockable(std::mutex, caller);  //locked by the thread who created this thread while its waiting
	TracyLockable(std::mutex, waiting); //locked by the thread when its waiting
	TracyLockable(std::mutex, statemutex);
    ThreadState state;
    string comment;

	int functioncalls = 0;

    void ChangeState(ThreadState ts){
        std::unique_lock<LockableBase(std::mutex)> lock(statemutex); LockMark(statemutex);
        state = ts;
        lock.unlock();
    }

	void WakeUp() {
        if(state != ThreadState_Sleep) return;
        ZoneScoped;
		ThreadCondition.notify_all();
	}

	//this funciton locks the caller mutex and waits until CallingThreadCondition receieves
	//a signal to unblock
	b32 Wait(u64 timeout = 0) {
        ZoneScoped;
        if(state == ThreadState_Sleep) return true;
        std::unique_lock<LockableBase(std::mutex)> lock(caller);
        LockMark(caller);
        if(timeout) 
            return (CallingThreadCondition.wait_for(lock, std::chrono::milliseconds(timeout), [this]{return state == ThreadState_Sleep;}) ? false : true);
        else CallingThreadCondition.wait(lock,[this]{return state == ThreadState_Sleep;});
        return true;
	}

    //attempts to run the threads function immediately, but returns early if it can't
	void Run(int count = 1) {
        ZoneScoped;
		if (state != ThreadState_Sleep) return; //thread is already running
		functioncalls = count;
		ChangeState(ThreadState_CallFunction);
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
		ChangeState(ThreadState_Close);
		WakeUp();
	}

	//waits for the thread to return, this means you must know the thread is active to begin with!
	void CloseAndJoin(){
        ZoneScoped;
		ChangeState(ThreadState_Close);
		WakeUp();
		if(me.joinable()) me.join();
	}

    //sets the function that the thread calls 
	template<typename FuncToRun, typename... FuncArgs>
	void SetFunction(FuncToRun f, FuncArgs...args) {
        ZoneScoped;
        TracyMessage("Closing previous thread", 23);
        CloseAndJoin(); 
        TracyMessage("Changing state", 14);
        ChangeState(ThreadState_Initializing);
        TracyMessage("Making std::thread", 18);
		me = std::thread(threadfunc<FuncToRun, FuncArgs...>, this, f, args...);
	}
    //sets the function that the thread calls and waits until its done initializing
	template<typename FuncToRun, typename... FuncArgs>
	void SetFunctionAndWait(FuncToRun f, FuncArgs...args) {
        SetFunction(f,args...);
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
#ifdef TRACY_ENABLE
    tracy::SetThreadName(me->comment.str);
#endif
	while (me->state != ThreadState_Close) {
        ZoneScopedN("threadfunc inner loop");
        TracyMessageC("thread loop start", 17, 0x11aa1f);
		if(me->state==ThreadState_CallFunction){
            TracyMessageC("thread calls its function", 25, 0xffaa00);
            while (me->functioncalls--) { f(deref_if_pointer(args)...); }
            TracyMessageC("thread finishes calling its fun", 31, 0xffaa00);
        }
        
        TracyMessageC("thread sends signal to manager", 30, 0x5577ff);
        last_returned = me;
        manager_wait.notify_all();
        
		me->CallingThreadCondition.notify_all();
        TracyMessageC("thread is going to sleep", 20, 0x00aaff);
		std::unique_lock<LockableBase(std::mutex)> lock(me->waiting);
        LockableBase(std::mutex)& ok = me->waiting;
        LockMark(ok);
        TracyMessageC("thread sets sleep state", 23, 0x00aaff);
		me->ChangeState(ThreadState_Sleep);
		while(me->state == ThreadState_Sleep)
			me->ThreadCondition.wait(lock);
        TracyMessageC("thread wakes up", 15, 0xff0000);

	}
    TracyMessageC("thread closes", 15, 0x0000ff);
	me->CallingThreadCondition.notify_all();
}

struct ThreadManager{
    set<Thread*> threads;
    Arena thread_arena;

    Thread* MakeNewThread(const string& comment = ""){
        ZoneScoped;
        if(!thread_arena.data) thread_arena.init(sizeof(Thread)*max_threads);
        if(threads.count >= max_threads) return 0;
        TracyMessage("Arenaing Thread", 15);
        Thread* nu = (Thread*)thread_arena.add(Thread());
        TracyMessage("Setting Comment", 15);
        if(comment.count) nu->comment = comment;
        TracyMessage("Adding Thread to map", 20);
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
        ZoneScoped;
		for(Thread* t : threads) t->Close();
	}

    void WaitForAllThreadsToFinish(u64 timeout = 0){
        ZoneScoped;
        for(Thread* t : threads) t->Wait(timeout);
    }

    Thread* GetFirstAvaliableThread(){
        ZoneScoped;
        for(Thread* t : threads)
            if(match_any(t->state, ThreadState_Sleep, ThreadState_NotInitialized)) return t;
        return 0;
    }
    //TODO add timeout
    Thread* WaitForAvaliableThread(){
        ZoneScoped;
        Thread* try_no_wait = GetFirstAvaliableThread();
        if(try_no_wait) return try_no_wait;
        std::unique_lock<std::mutex> lock(manager_mute);
        manager_wait.wait(lock);
        ZoneText(last_returned->comment.str, last_returned->comment.count)
        TracyMessage("WaitForAvaliable received signal", 33);
        return last_returned;
    }

    ~ThreadManager(){
        
    }

} tmanager;
