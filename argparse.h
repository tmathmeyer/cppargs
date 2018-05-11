
#ifndef argparse_h
#define argparse_h

#include <iostream>
#include <string>
#include <sys/types.h>
#include <sys/types.h>
#include <tuple>
#include <utility>
#include <vector>

#include "exception.h"
#include "fs.h"
#include "index.h"


// Macro for defining flags:
// Flag(Example, "--flag", "-f", "long description")
struct _FLAG_ {};
#define Flag(name, _full, _simple, _desc) struct _F_##name : _FLAG_ { \
  const std::string full = _full; \
  const std::string simple = _simple; \
  const std::string desc = _desc; \
}


// Macro for defining an Argument parser:
// Arg(Example, std::string, std::string)
// Arg(Other, std::string, Example)
#define Arg(name, ...) \
class name : public Argument_<_F_##name, __VA_ARGS__> {};


// Macros for generating incomplete types for 
// recursive specialiazation.
#define RecursiveType(sname) \
template<typename... X_s> \
struct sname;

#define DefaultHandler(sname, V) \
template<typename V> \
struct sname<V>

#define RecursiveHandler(sname, V) \
template<typename V, typename... V##s> \
struct sname<V, V##s...>


// Forward declare some recursively specialized types.
RecursiveType(ShowType);
RecursiveType(PrintHelp);
RecursiveType(GroupParse);


// Some simple typedefs to keep code more sane.
using strings = std::vector<std::string>;
using nullarg_t = std::tuple<nullptr_t>;
template<typename X>
using converted = std::tuple<X, strings>;


// A macro for defining an integral converter / parser.
#define convert_type(TYPE) \
template<> \
struct convert<TYPE> { \
	static converted<TYPE> c(strings vec) { \
		strings rest(vec.begin()+1, vec.end()); \
		try { \
			return std::make_tuple(std::stoi(vec[0]), rest); \
		} catch(...) { \
			EXCEPT("Could not convert \"" + vec[0] + "\" to a " + #TYPE); \
		} \
	} \
	static std::string Stringify() { return #TYPE; } \
};


// Default integral parser, can handle any type
// which has a functor: Name()->std::string
// and a functor:       parse(std::vector<std::string>)->*
template<typename X>
struct convert {
	static converted<X> c(strings vec) {
		X argparser;
		argparser.parse(vec);
		return std::make_tuple(argparser, argparser.args_);
	}
	static std::string Stringify() {
		return X().Name();
	}
};

// Converter for strings.
template<>
struct convert<std::string> {
	static converted<std::string> c(strings vec) {
		strings rest(vec.begin()+1, vec.end());
		return std::make_tuple(vec[0], rest);
	}
	static std::string Stringify() {
		return "string";
	}
};

// Converter for path elements
template<>
struct convert<fs::path> {
	static converted<fs::path> c(strings vec) {
		strings rest(vec.begin()+1, vec.end());
		return std::make_tuple(fs::path(vec[0]), rest);
	}
	static std::string Stringify() {
		return "path";
	}
};

// Converter for empty argument.
template<>
struct convert<nullptr_t> {
	static converted<nullptr_t> c(strings vec) {
		return std::make_tuple(nullptr, vec);
	}
	static std::string Stringify() {
		return "";
	}
};

// Converter for any std::optional<T> where T can
// also be handled by some converter.
template<typename X>
struct convert<std::optional<X>> {
	static converted<std::optional<X>> c(strings vec) {
		if (vec.size() == 0) {
			return std::make_tuple(std::nullopt, vec);
		}
		try {
			converted<X> c = convert<X>::c(vec);
			return std::make_tuple(
				std::optional<X>(std::get<0>(c)),
				std::get<1>(c));
		} catch(TraceException _) {
			return std::make_tuple(std::nullopt, vec);
		}
	}
	static std::string Stringify() {
		return "[" + convert<X>::Stringify() + "]";
	}
};

// Converter for tuples.
template<typename F, typename ...R>
struct convert<std::tuple<F, R...>> {
	static converted<std::tuple<F, R...>> c(strings vec) {
		converted<F> first = convert<F>::c(vec);
		converted<std::tuple<R...>> rest =
		  convert<std::tuple<R...>>::c(std::get<1>(first));
		std::tuple<F, R...> all = std::tuple_cat(
			std::make_tuple(std::get<0>(first)),
			std::get<0>(rest));
		return std::make_tuple(all, std::get<1>(rest));
	}
	static std::string Stringify() {
		return ShowType<F, R...>::NameTypes();
	}
};

template<typename F>
struct convert<std::tuple<F>> {
	static converted<std::tuple<F>> c(strings vec) {
		converted<F> f = convert<F>::c(vec);
		return std::make_tuple(
			std::make_tuple(std::get<0>(f)),
			std::get<1>(f));
	}
	static std::string Stringify() {
		return convert<F>::Stringify();
	}
};

// AnyOrder definition
template<typename ...F>
class AnyOrder {
public:
	std::tuple<F...> wrapped;

	template<typename X>
	X get(size_t index) {
		if (index >= std::tuple_size<decltype(wrapped)>::value) {
			EXCEPT("Get out of bounds!");
		}
		return std::get<index>(wrapped);
	}
	
	template <std::size_t I>
	decltype(auto) operator[](tuple_index::index<I>) {
		return std::get<I>(this->wrapped);
	}
};


template<typename F, typename ...R>
struct TMagic {
	static F last(std::tuple<R..., F> in) {
		return std::get<std::tuple_size<decltype(in)>::value - 1>(in);
	}

	template<size_t ...I>
	static std::tuple<R...> all_but_last(std::tuple<R..., F> in, std::index_sequence<I...>) {
		return std::make_tuple(std::get<I>(in)...);
	}

	static std::tuple<F, R...> back_cycle(std::tuple<R..., F> in) {
		const size_t tuple_size = std::tuple_size<decltype(in)>::value;
		auto indicies = std::make_index_sequence<tuple_size -1>{};
		std::tuple<R...> rest = TMagic<F, R...>::all_but_last(in, indicies);
		F first = TMagic<F, R...>::last(in);
		return std::tuple_cat(std::make_tuple(first), rest);
	}
};

// Converter for AnyOrder
template<typename F, typename ...R>
struct convert<AnyOrder<F, R...>> {
	static converted<AnyOrder<F, R...>> c(strings vec) {
		return convert<AnyOrder<F, R...>>::c_limited(vec, 0);
	}

	static converted<AnyOrder<F, R...>> c_limited(strings vec, size_t itrs) {
		AnyOrder<F, R...> ret;
		strings res;

		try {
			converted<AnyOrder<F>> first = convert<AnyOrder<F>>::c(vec);
			strings restvec = std::get<1>(first);
			converted<AnyOrder<R...>> rest = convert<AnyOrder<R...>>::c(restvec);
			ret.wrapped = tuple_cat(std::get<0>(first).wrapped, std::get<0>(rest).wrapped);
			res = std::get<1>(rest);
		} catch(char const *_) {
			const size_t elements = std::tuple_size<std::tuple<F, R...>>::value;
			if (itrs <= elements) {
				converted<AnyOrder<R..., F>> cycle = convert<AnyOrder<R..., F>>::c_limited(vec, itrs + 1);
				ret.wrapped = TMagic<F, R...>::back_cycle(std::get<0>(cycle).wrapped);
				res = std::get<1>(cycle);
			}
		}

		return std::make_tuple(ret, res);
	}

	static std::string Stringify() {
		return ShowType<F, R...>::NameTypes();
	}
};

template<typename F>
struct convert<AnyOrder<F>> {
	static converted<AnyOrder<F>> c(strings vec) {
		converted<F> converted = convert<F>::c(vec);
		AnyOrder<F> ret;
		ret.wrapped = std::make_tuple(std::get<0>(converted));
		return std::make_tuple(ret, std::get<1>(converted));
	}
	static std::string Stringify() {
		return convert<F>::Stringify();
	}
};

template<typename F>
struct convert<AnyOrder<std::optional<F>>> {
	using AOF = AnyOrder<std::optional<F>>;
	static converted<AOF> c(strings vec) {
		converted<std::optional<F>> maybe = convert<std::optional<F>>::c(vec);
		std::optional<F> f_ = std::get<0>(maybe);
		if (f_) {
			AOF ret;
			ret.wrapped = std::make_tuple(f_);
			return std::make_tuple(ret, std::get<1>(maybe));
		}

		throw "nopt";
	}
	static std::string Stringify() {
		return convert<F>::Stringify();
	}
};

// Create converters for a selection of integer types.
// non-exhaustive, as I am lazy.
convert_type(uint32_t);
convert_type(uint64_t);
convert_type(uint16_t);
convert_type(int);
convert_type(long);


// Create a recursive type parser.
// TODO: switch this to the RecursiveType(...) macro
template<typename F, typename... R>
struct _parser_ {
	static converted<std::tuple<F, R...>> parse(strings args) {
		if (args.size() == 0) {
			EXCEPT("Missing arguments");
		}
		converted<F> c;
		try {
			c = convert<F>::c(args);
		} catch(TraceException e) {
			EXCEPT_CHAIN(e, "Could not convert type " + convert<F>::Stringify());
		} catch(...) {
			EXCEPT("Could not convert type " + convert<F>::Stringify());
		}
		converted<std::tuple<R...>> rest = _parser_<R...>::parse(std::get<1>(c));
		return std::make_tuple(
			std::tuple_cat(std::make_tuple(std::get<0>(c)),
										   std::get<0>(rest)),
			std::get<1>(rest));
	}
};

// Base case for recursive type parser.
template<typename F>
struct _parser_<F> {
	static converted<std::tuple<F>> parse(strings args) {
		converted<F> c = convert<F>::c(args);
		return std::make_tuple(
			std::make_tuple(std::get<0>(c)),
			std::get<1>(c));
	}
};


// The untemplated Argument base class used for type erasure.
class Argument {
public:
	virtual ~Argument() = default;
	virtual void EnsureNoRemainingArguments() = 0;
};

// A templated subclass of Argument from which all manually
// declared classes inherit. Supplies the parse and DisplayHelp
// methods.
template<typename T, typename... P>
class Argument_ : public Argument {
public:
	std::tuple<P...> parsed_;
	strings args_;
	std::tuple<P...> parse(strings args) {
		try {
			if (args.size() > 0 && (args[0] == T().full || args[0] == T().simple)) {
				strings rest(args.begin()+1, args.end());
				converted<std::tuple<P...>> x = _parser_<P...>::parse(rest);
				parsed_ = std::get<0>(x);
				args_ = std::get<1>(x);
				return parsed_;
			}
		} catch(TraceException err) {
			EXCEPT_CHAIN(err, "Parsing flag " + T().full + " failed.");
		}
		if (args.size() == 0) {
			EXCEPT("Could not parse empty flags");
		} else {
			EXCEPT("Could not parse flag: " + args[0]);
		}
	}

	void DisplayHelp() {
		std::cout << T().full << ", " << T().simple << " ";
		std::cout << ShowType<P...>::NameTypes() << std::endl;
		std::cout << T().desc << std::endl << std::endl;
	}

	void EnsureNoRemainingArguments() override {
		if (args_.size()) {
			EXCEPT("Argument " + args_[0] + " not parsed.");
		}
	}

	std::string Name() {
		return T().full;
	}

	template <std::size_t I>
	decltype(auto) operator[](tuple_index::index<I>) {
		return std::get<I>(this->parsed_);
	}
};


// Recursive specialization converting type sequences
// to strings.
DefaultHandler(ShowType, T) {
	static std::string NameTypes() {
		return convert<T>::Stringify();
	}
};
RecursiveHandler(ShowType, T) {
	static std::string NameTypes() {
		return convert<T>::Stringify()
		       + ", "
		       + ShowType<Ts...>::NameTypes();
	}
};


// Recursive specialization for finding the best
// Argument subtype to parse.
DefaultHandler(GroupParse, T) {
	static Argument *parse(strings args) {
		T *group = new T();
		group->parse(args);
		return group;
	}
};
RecursiveHandler(GroupParse, T) {
	static Argument *parse(strings args) {
		T *group = new T();
		try {
			group->parse(args);
			return group;
		} catch(std::string err) {
			delete group;
			std::cout << err << std::endl;
			return GroupParse<Ts...>::parse(args);
		} catch(...) {
			delete group;
			return GroupParse<Ts...>::parse(args);
		}
	}
};

// Recursive specialization for printing the help text
// for a given set of Argument subtypes.
DefaultHandler(PrintHelp, T) {
	static void PrintTypeHelp() { T().DisplayHelp(); }
};
RecursiveHandler(PrintHelp, T) {
	static void PrintTypeHelp() {
		T().DisplayHelp();
		PrintHelp<Ts...>::PrintTypeHelp();
	}
};


// Print function for an argument type.
template<typename T, typename... P>
std::ostream& operator<<(std::ostream& os, const Argument_<T, P...>& obj) {
	os << obj.parsed_;
	return os;
}

// Entry point function to parse args.
template<typename ...Args>
Argument *ParseArgs(int argc, char **argv) {
	strings arguments(argv + 1, argv + argc);
	Argument *result = GroupParse<Args...>::parse(arguments);
	result->EnsureNoRemainingArguments();
	return result;
}

// Entry point function to display help text.
template<typename ...Types>
void DisplayHelp() {
	PrintHelp<Types...>::PrintTypeHelp();
}


#endif