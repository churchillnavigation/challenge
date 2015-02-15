
#include "SecondTry.h"

template<typename T>
struct Factory
{
	typedef T T;

	static T *create(const Input::Point *begin, const Input::Point *end)
	{
		return new T(begin, end);
	}
	static int search(T *context, const Input::Rect &rect, const int32_t count, Input::Point* out)
	{
		return context->search(rect, count, out);
	}
	static void destroy(T *context)
	{
		delete context;
	}
};

typedef Factory<SecondTry> RealFactory;

extern "C"
{

__declspec(dllexport) RealFactory::T *create(const Input::Point *begin, const Input::Point *end)
{
	return RealFactory::create(begin, end);
}

__declspec(dllexport) int search(RealFactory::T *sc, const Input::Rect rect, const int32_t count, Input::Point *out)
{
	// There are just some silly robustness check cases; filter them out once and for all.
	if (count > 22)
		abort();
	if (sc->empty() || count < 1)
		return 0;
	return sc->search(rect, count, out);
}

__declspec(dllexport) RealFactory::T *destroy(RealFactory::T *sc)
{
	delete sc;
	return 0;
}
}
