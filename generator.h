#pragma once
#include <functional>
#include <experimental/optional>

using std::experimental::optional;

namespace qflow {
template<typename T>
struct generator
{
	using yield_value = optional<T>*;
	generator(std::function<void(yield_value)> func) : _f(func)
	{

	}
	struct iterator
	{
		iterator(std::function<void(yield_value)> func, bool done = false) : _f(func), _done(done)
		{

		}
		iterator &operator++() {
			optional<T> val;
			 _f(&val);
			 if(val) _current = val.value();
			 else _done = true;
			return *this;
		}
		bool operator==(iterator const &other) const {
			return _done == other._done;
		}
		bool operator!=(iterator const &other) const { return !(*this == other); }
		T const &operator*() const 
		{ 
			return _current; 
		}
		std::function<void(yield_value)> _f;
		T _current;
		bool _done;
	};
	iterator begin() {
		iterator i{_f};
		return ++i;
	}
	iterator end() {
		return{ _f, true };
	}
	std::function<void(yield_value)> _f;
};
}
