
#include <malloc.h>
#include <stdio.h>
#include <string.h>

#include <functional>
#include <vector>
#include <utility>
#include <iostream>

#define ASYNC_AWAIT_GOTO_POITER 1



void* operator new  ( std::size_t count )
{
	printf("**** alloc %d\n", (int)count);
	return malloc(count);
}

void* operator new[]( std::size_t count )
{
	printf("**** alloc[] %d\n", (int)count);
	return malloc(count);
}

void operator delete(void* ptr) noexcept
{
    printf("**** free\n");
    free(ptr);
}

void operator delete[](void* ptr) noexcept
{
    printf("**** free[]\n");
    free(ptr);
}

template< class >
struct myfunction; /* undefined */

int mf_inst = 1;;

template< class R, class... Args >
struct myfunction<R(Args...)>
{
	std::function<R(Args...)> func;
	int instance;
	myfunction() {
		instance = mf_inst;
		mf_inst++;
		printf("#F empty %d\n", instance);
	}
	myfunction(std::function<R(Args...)>&& f) {
		func = std::move(f);
		instance = mf_inst;
		mf_inst++;
		printf("#F construct %d\n", instance);
	}
	myfunction(const myfunction& x) {
		instance = mf_inst;
		mf_inst++;
		func = x.func;
		printf("#F construct %d: copy %d\n", instance, x.instance);
	}
	myfunction(myfunction&& x) {
		instance = x.instance;
		x.instance = -x.instance;
		func = std::move(x.func);
		printf("#F move %d\n", instance);
	}
	void operator=(const myfunction& x) {
		func = x.func;
		printf("#F assign %d = %d\n", instance, x.instance);
	}
	void operator=(myfunction&& x) {
		func = std::move(x.func);
		printf("#F move %d = %d\n", instance, x.instance);
		instance = x.instance;
		x.instance = -x.instance;
	}
	~myfunction() {
		printf("#F destuct %d\n", instance);
	}
	R operator()( Args... args ) const
	{
		return func(std::forward<Args>(args) ...);
	}
	bool empty() { return func == nullptr; }
};


struct AsyncResultValue
{
	template<class T> T get() const {return T();}
	template<class T> T move() const {return T();}
	bool empty() {return true;}
	void clear() {}
};

std::function<void(const AsyncResultValue&)> callbackGlobal;

void write(const std::function<void(const AsyncResultValue&)>& callback, int x)
{
	printf("%d\n", x);
	printf("-- assign callback\n");
	callbackGlobal = callback;
	printf("-- assign callback done\n");
}

template <typename...> struct WhichType;
class Something {};

template <typename ... T>
void TemplatePrint(T ... args )
{
    std::cout << __PRETTY_FUNCTION__ << std::endl;
}


struct myvect
{
	int instance;
	int data[20];
	static int i;
	myvect() {
		instance = i;
		i++;
		printf("construct %d\n", instance);
	}
	myvect(const myvect& x) {
		instance = i;
		i++;
		printf("construct %d: copy %d\n", instance, x.instance);
	}
	myvect(myvect&& x) {
		instance = x.instance;
		x.instance = -x.instance;
		memcpy(data, x.data, sizeof(data));
		printf("move %d\n", instance);
	}
	~myvect() {
		printf("destuct %d\n", instance);
	}
	int& operator[](int x) {return data[x]; }
};

int myvect::i = 1;

/*template<class T0, class T1, class T2, class T3>
void funcInner(void* _resumePoint, T0& callback, T1 add, T2 i, T3& vect)
{
	void *_resumePoint_1_ptr = &&_resumePoint_1;
	if (_resumePoint != NULL)
	{
		goto *_resumePoint;
	}
	vect[0] = 10;
	vect[1] = 11;
	vect[2] = 12;
	vect[3] = 13;
	vect[4] = 14;
	vect[5] = 15;
	vect[6] = 16;
	for (i = i; i < 6; i++)
	{
		printf("-- create lambda\n");
		write(([_resumePoint_1_ptr,
			callback = std::move(callback),
			add,
			i,
			vect = std::move(vect)]() mutable
			{
				printf("-- lambda enter\n");
				printf("-- funcInner\n");
				funcInner<T0, T1, T2, T3>(_resumePoint_1_ptr, callback, add, i, vect);
				printf("-- lambda exit\n");
			}), vect[i] + add);
		printf("-- interrupt\n");
		return;
		_resumePoint_1: while(0) {};
	}
	printf("-- return\n");
	callback();
}*/

struct Storage;

void funcInner(Storage* storage, const AsyncResultValue& __result__);

int sss[sizeof(std::_Nocopy_types::_M_member_pointer)] = { 
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
};

struct Storage {
	std::function<void(const AsyncResultValue&)> __resume__;
	myfunction<void()> __callback__;
#ifdef ASYNC_AWAIT_GOTO_POITER
	void* __resumePoint__;
#else
	int __resumePoint__;
#endif
	AsyncResultValue __result__;
	int add;
	int i;
	myvect vect;
	int arr[10];
	#if this_is_parallel_async
	struct __Resume__rd
	{
		std::function<void(const AsyncResultValue&)> func;
		__Resume__rd() : func(std::reference_wrapper<Storage>(*this)) {}
		void operator() (const AsyncResultValue& r)
		{
			funcInner(GET_PARENT_CONTAINER(this, Storage, __resume__rd), 0, &r);
		}
	};
	__Resume__rd __resume__rd;
	struct __Resume__wr
	{
		std::function<void(const AsyncResultValue&)> func;
		__Resume__wr() : func(std::reference_wrapper<Storage>(*this)) {}
		void operator() (const AsyncResultValue& r)
		{
			funcInner(GET_PARENT_CONTAINER(this, Storage, __resume__wr), 1, &r);
		}
	};
	__Resume__wr __resume__wr;
	#endif
	
	Storage(myfunction<void()>& p0, int p1)
		:__resume__(std::reference_wrapper<Storage>(*this)),
		//__resume__rd(), <-- this_is_parallel_async
		//__resume__wr(),
		__callback__(std::move(p0)),
		__resumePoint__(0),
		__result__(),
		add(p1),
		i(3),
		vect(),
		arr{1,2,3,4,5,6,7,8,9,10}
	{}
	void operator() (const AsyncResultValue& r)
	{
		funcInner(this, &r);
	}
};

void funcInner(Storage* storage, const AsyncResultValue* __result__)
{
	// references to storage
	int& add = storage->add;
	int& i = storage->i;
	myvect& vect = storage->vect;
	int (&arr)[10] = storage->arr;
	const AsyncResultValue* awaitResult;
#ifdef ASYNC_AWAIT_GOTO_POITER
	// jump to resume point if resuming
	static void* const _resumePoint_1_value = &&_resumePoint_1;
	void* __resumePoint__ = storage->__resumePoint__;
	storage->__resumePoint__ = &&_resume_now_;
	if (__resumePoint__ != NULL)
	{
		goto *__resumePoint__;
	}
#else
	// OR switch case as an option for better portability and detection of initialization crossing
	static const int _resumePoint_1_value = 2;
	int __resumePoint__ = storage->__resumePoint__;
	storage->__resumePoint__ = 1;
	switch (__resumePoint__)
	{
		case 0:
			break;
		case 1:
			storage->__result__ = *__result__;
			return;
		case 2:
			goto _resumePoint_1;
	}
#endif
	// start of async function body
	vect[0] = 10;
	vect[1] = 11;
	vect[2] = 12;
	vect[3] = 13;
	vect[4] = 14;
	vect[5] = 15;
	vect[6] = 16;
	for (i = i; i < 6; i++)
	{
		// AWAIT call
		storage->__result__.clear();
		write(storage->__resume__, vect[i] + add + 1000 * arr[i]);
		//write(storage->__resume__wr.func, vect[i] + add + 1000 * arr[i]); 
		if (storage->__result__.empty())
		{
			storage->__resumePoint__ = _resumePoint_1_value;
			return;
			_resumePoint_1: do {} while(0);
		}
		else
		{
			__result__ = &storage->__result__;
		}
		// getting result from last AWAIT call
		int cnt = __result__->get<int>();
		// continue of normal async function body
		if (cnt > 100)
		{
			goto _async_return;
		}
	}

	_async_return:
	storage->__callback__();
	delete storage;
	return;

	_resume_now_:
	storage->__result__ = *__result__;
}

static inline void func(int add, myfunction<void()>&& callback)
{
	printf("-- func (move)\n");
	Storage* s = new Storage(callback, add);
	/*s->__resume__ = std::reference_wrapper<Storage>(*s);
	s->__callback__ = std::move(callback);
	s->__resumePoint__ = NULL;
	s->add = add;
	s->i = 3;*/
	printf("-- funcInner first\n");
	funcInner(s, NULL);
}

/*void func1(int add, myfunction<void()>& callback)
{
	callback();
	int i = 3;
	myvect vect;
	printf("-- funcInner first\n");
	funcInner(NULL, callback, add, i, vect);
}*/

static inline void func(int add, const myfunction<void()>& callback)
{
	printf("-- func (copy)\n");
	func(add, myfunction<void()>(callback));
}

/*

int x;
int x		x

vector<int> x;
const vector<int>& 

*/

#if 0

void func(ASYNC, int PERSISTENT add)
{
	int PERSISTENT i = 3;
	myvect PERSISTENT vect(11);
	int PERSISTENT arr[10];
	for (i = i; i < 10; i++)
	{
		write(AWAIT, i);
		int cnt = AWAIT_RESULT<int>;
		// OR int cnt = AWAIT_RESULT.isError() ? -1 : AWAIT_RESULT.get<int>();
		if (cnt > 100)
		{
			setTimeout(AWAIT, 0.5f);
			ASYNC return;
		}
	}
}

// AND second type of ASYNC function with parallel calls

void echoBytes(PARALLEL_ASYNC, int PERSISTENT size)
{
	int PERSISTENT i;
	char PERSISTENT c;
	bool PERSISTENT next = true;
	for (i = 0; i <= size; i++)
	{
		// read and write in parallel
		if (i > 0)
		{
			write(AWAIT_CALLBACK(wr), c);
		}
		if (i < size)
		{
			read(AWAIT_CALLBACK(rd));
			AWAIT(rd);
			c = AWAIT_RESULT(rd);
		}
		if (i > 0)
		{
			AWAIT(wr);
		}
		sensor1.read(AWAIT_CALLBACK(s1));
		sensor2.read(AWAIT_CALLBACK(s2));
		AWAIT_FOR(s1);
		int PERSITANT value1 = AWAIT_RESULT<int>;
		AWAIT_FOR(s2);
		value2 = AWAIT_RESULT<int>;
		sendReadings(AWAIT, value1, value2);
	}
}

#endif

struct Aaa
{
	int x;
	Aaa(int x) : x(x) { printf("Construct\n");}
	~Aaa() { printf("Destruct\n");}
};


void testf(void** p)
{
	puts("testf");
	if (*p) goto jumpto;
	puts("ret now");
	*p = &&jumpto;
	//return;
	{
		int x;
		printf("%d\n", x + 1);
		jumpto:
		printf("jumpto done %d\n", "a.x");
		//a.x = 123456;
		while (0) {};
	}
	puts("ret NULL");
}

std::function<void()> aaaaaa;

int main()
{
	/*myfunction<void()> f = std::function<void()>([](){ printf("Done!\n"); });
	func(100, f);
	//f();
	while (callbackGlobal != nullptr)
	{
		std::function<void(const AsyncResultValue&)> callback = std::move(callbackGlobal);
		printf("-- call lambda\n");
		callback(AsyncResultValue());
	}
	//f();*/
	void *p = NULL;
	testf(&p);
	testf(&p);
	return 0;
}

/* TOOL:

1. Preprocess the file .cc -> .prep.cc
2. Replace #line/col to spaces .prep.cc -> .no_line.cc
3. Run information extracting tool based on clang AST .no_line.cc -> .info.cc
   - for each async function/method
   	- location
   	- template decl location
   	- ASYNC parameter location
   	- persistent parameters/variables: names and locations, e.t.c.
   	- AWAIT calls locations
   	- e.t.c.
4. Replace special things with standard code .prep.cc --(.info.cc)--> .repl.cc
5. Compile .repl.cc -> .o

*/
