/*

	Test bed for setting up dependency based blocking.

	In amu Code objects may need to block on other Code objects which I originally wanted to solve using
	coroutines, but since C++ 20 coroutines are too complicated and there aren't any good libs wrapping 
	it (that I can build successfully) yet, we have to settle with using actual threads. Which is 
	find since amu will be multithreaded later.

	The example here involves parsing a small table language, where you can have tables like:
		name: {
			elem0: 1;
			elem1: 2;
			...
		}
	and these tables can use values from other tables
		other_name: {
			elem0: 1;
			elem1: name.elem0;
			...
		}
	and it doesn't matter where the table it's referencing is. The file is lexed and table locations are marked,
	then we start a parser on each table. Tables store whether they're finished or not and so if the parser
	comes across a reference to an unfinished table then it saves the table it depends on and waits on that table's 
	future. 

	When a table is finished it fulfills its promise, waking all of the tables dependent on it which must go through 
	the semaphore to actually continue.

	When a dependency is added to a table we also check for dependency cycles using Floyd's cycle finding algorithm.
	
	The code here is only meant to be a demonstration and, if amu's implementation of Code dependencies stays true to
	this, a reference for how Code dependencies works.

*/


#include <semaphore>
#include <thread>
#include <future>
#include <type_traits>

#include "Common.h"
#include "basic/Memory.h"
#include "basic/Node.h"
#include "storage/String.h"
#include "storage/DString.h"
#include "storage/Array.h"
#include "storage/Pool.h"
#include "storage/Map.h"

#include "basic/Memory.cpp"
#include "basic/Node.cpp"
#include "storage/String.cpp"
#include "storage/DString.cpp"
#include "storage/Array.cpp"
#include "storage/Pool.cpp"
#include "storage/Map.cpp"

using namespace amu;

template<typename R, typename F, typename... Args> void
wrapper(std::promise<R> p, F f, Args... args);

struct Man {
	Array<std::thread> threads;
	std::counting_semaphore<1> sema; 
	
	template<typename F, typename... Args> std::future<std::invoke_result_t<F, Args...>>
	start(F f, Args... args) {
		using R = std::invoke_result_t<F, Args...>;
		auto p = std::promise<R>();
		auto fut = p.get_future();
		*threads.push() = std::thread(wrapper<R, F, Args...>, std::move(p), f, args...);
		return fut;
	}

	void
	end() {
		sema.release();
	}

	template<typename T> T
	wait_for_dep(std::future<T>& fut) {
		sema.release();
		auto val = fut.get();
		sema.acquire();
		return val;
	}

	Man() : sema(1) {}
};

Man man;

template<typename R, typename F, typename... Args> void
wrapper(std::promise<R> p, F f, Args... args) {
	man.sema.acquire();
	if constexpr(std::is_void<R>::value) {
		f(args...);
		p.set_value();
	} else {
		p.set_value(f(args...));
	}
	man.sema.release();
}

struct Tok {
	enum class Kind {
		Colon,
		OpenBrace,
		CloseBrace,
		Dot,
		Semicolon,
		Literal,
		Word,
	};

	Kind kind;

	union {
		u64 literal;
		String word;
	};
};

struct KV {
	String key;
	u64 val;
};

struct Table {
	String name;
	Array<KV> elements;
	Array<std::promise<b32>> deps;
	Tok* start;
	b32 finished;

	// the Table this one depends on, if any
	Table* dependency;

	// promise kept when this table is finished processing
	std::promise<b32>* promise;
	std::shared_future<b32> fut;

	Table() {
		promise = new std::promise<b32>();
		fut = promise->get_future().share();
	}

	// makes the given Dependency dependent on this one
	void
	make_dependent(Table* dep) {
		dep->dependency = this;
		// check for a dependency cycle 
		if(!dependency) return;
		Table* slow = this;
		Table* fast = this->dependency;
		while(1) {
			if(!fast->dependency) return;
			if(!fast->dependency->dependency) return;
			slow = slow->dependency;
			fast = fast->dependency->dependency;
			if(slow == fast) {
				util::println("dependency cycle detected");
				slow = slow->dependency;
				while(slow != fast) {
					util::print(slow->name);
					util::println(" ->");
					slow = slow->dependency;
				}
				util::println(slow->name);
				Assert(0);
			}
		}
	}

	// wait for the table we depend on to finish
	b32 
	wait() {
		if(!dependency) return true;
		man.sema.release();
		auto val = dependency->fut.get();
		man.sema.acquire();
		return val;
	}

	void
	satisfy(b32 b) {
		promise->set_value(b);
		delete promise;
	}
};

Pool<Table> tables = pool::init<Table>(8);
Map<String, Table*> table_map = map::init<String, Table*>();

Table*
find_table(String name) {
	auto [idx, found] = map::find(table_map, name);
	if(!found) return 0;
	return table_map.values.read(idx);
}

u64
table_find(Table* tbl, String name) {
	forI(tbl->elements.count) {
		auto elem = tbl->elements.read(i);
		if(name.equal(elem.key)) return elem.val;
	}
	return -1;
}

struct LexRet {
	Array<Tok> tokens;
	Array<u64> labels;
};

LexRet
lex(String input) {
	LexRet out;
	out.tokens = Array<Tok>::create();
	out.labels = Array<u64>::create();
	u64 scope = 0;
	while(input) {
		switch(*input.str) {
			case ':': {
				out.tokens.push(Tok{.kind=Tok::Kind::Colon});
				if(!scope && out.tokens.read(-2).kind == Tok::Kind::Word) {
					out.labels.push(out.tokens.count-1);
				} 
			} break;
			case '{': {
				out.tokens.push(Tok{.kind=Tok::Kind::OpenBrace});
				scope++;
			} break;
			case '}': {
				out.tokens.push(Tok{.kind=Tok::Kind::CloseBrace});
				scope--;
			} break;
			case '.': {
				out.tokens.push(Tok{.kind=Tok::Kind::Dot});
			} break;
			case ';': {
				out.tokens.push(Tok{.kind=Tok::Kind::Semicolon});
			} break;
			default: {
				if(string::isdigit(*input.str)) {
					auto i = input.eat_int();
					input.advance(i.count);
					out.tokens.push(Tok{.kind=Tok::Kind::Literal, .literal = (u64)i.to_s64()});
				} else if(string::isalnum(*input.str)) {
					auto s = input.eat_word();
					input.advance(s.count);
					out.tokens.push(Tok{.kind=Tok::Kind::Word, .word = s});
				} else if(string::isspace(*input.str)) {
					input = input.skip_whitespace();
				}
				continue;
			} break;
		}
		input.advance();
	}	
	return out;
}

Tok*
parse_key_val(Table* tbl, Tok* start) {
	Tok* curt = start;

	if(curt->kind != Tok::Kind::Word) {
		util::println("parse error: expected key");
		return 0;
	}

	auto key = curt->word;

	curt++;
	if(curt->kind != Tok::Kind::Colon) {
		util::println("parse error: expected colon after key");
		return 0;
	}

	curt++;
	if(curt->kind == Tok::Kind::Literal) {
		auto kv = tbl->elements.push();
		kv->key = key;
		kv->val = curt->literal;
	} else {
		auto ref = find_table(curt->word);
		if(!ref) {
			util::println(DString::create("parse error: ref to unknown table '", curt->word, "'"));
			return 0;
		}

		if(!ref->finished) {
			util::println(DString::create(tbl->name, " waiting for dependency ", ref->name));
			ref->make_dependent(tbl);
			if(!tbl->wait()) return 0;
			util::println(DString::create(tbl->name, " done waiting for dependency ", ref->name));
		}

		curt++;
		if(curt->kind != Tok::Kind::Dot) {
			util::println("parse error: expected access of ref");
			return 0;
		}

		curt++;
		if(curt->kind != Tok::Kind::Word) {
			util::println("parse error: expected member name");
			return 0;
		}

		auto kv = tbl->elements.push();
		kv->key = key;
		kv->val = table_find(ref, curt->word);
	}
	
	curt++;
	if(curt->kind != Tok::Kind::Semicolon) {
		util::println("parse error: expected semicolon after key/value");
		return 0;
	}

	return curt;
}

void
parse(Table* tbl, Tok* start) {
	Tok* curt = start;

	if(curt->kind != Tok::Kind::Word) {
		util::println("parse error: expected word to name table");
		return;
	}

	tbl->name = curt->word;
	
	util::println(DString::create(tbl->name, ": starting parse"));

	curt++;
	if(curt->kind != Tok::Kind::Colon) {
		util::println("parse error: expected colon after table name");
		return;
	}

	curt++;
	if(curt->kind != Tok::Kind::OpenBrace) {
		util::println("parse error: expected '{' to start table");
		return;
	}

	while(1) {
		curt++;
		if(curt->kind == Tok::Kind::CloseBrace) {
			break;
		}
		curt = parse_key_val(tbl, curt);
		if(!curt) return;
	}

	util::println(DString::create("finished parsing ", tbl->name));

	tbl->finished = true;

	tbl->satisfy(true);

	man.end();
}

void test() {
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	util::println("ohhh boy");
}


int main() {
	auto file = fopen("misc/coinput", "r");
	fseek(file, 0, SEEK_END);
	u64 sz = ftell(file);
	fseek(file, 0, SEEK_SET);
	
	String buf;
	buf.str = (u8*)memory::allocate(sz);
	buf.count = sz;
	fread(buf.str, sz, 1, file);

	auto lexed = lex(buf);

	forI(lexed.labels.count) {
		auto t = pool::add(tables);
		t->elements = Array<KV>::create();
		t->name = lexed.tokens.read(lexed.labels.read(i)-1).word;
		t->start = lexed.tokens.readptr(lexed.labels.read(i)-1);
		map::add(table_map, t->name, t);
	}


	auto futures = Array<std::future<void>>::create();
	std::promise<void> sig;
	std::shared_future sf = sig.get_future().share();
	
	auto iter = pool::iterator(tables);
	auto guy = pool::next(iter);
	while(guy) {
		*futures.push() = man.start([&](Table* ptr, Tok* start) {
			sf.wait();
			parse(ptr, start);
		}, guy, guy->start);
		guy = pool::next(iter);
	}
	sig.set_value();

	forI(futures.count) {
		futures.readptr(i)->get();
	}
}
