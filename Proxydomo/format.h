#ifndef SHAND_FORMAT_INCLUDE
#define SHAND_FORMAT_INCLUDE

#include <boost/format.hpp>

namespace shand {

	template <class CharT,
	class Traits = std::char_traits<CharT>,
	class Alloc = std::allocator<CharT> >
	class basic_formatter {
	public:
		typedef boost::basic_format<CharT, Traits, Alloc> format_type;
		typedef typename format_type::string_type         string_type;
		typedef basic_formatter&                          result_type;

		basic_formatter(const CharT* fmt) : fmt_(fmt) {}
		basic_formatter(const string_type& fmt) : fmt_(fmt) {}

		template <class T>
		basic_formatter& operator()(const T& arg)
		{
			fmt_ % arg;
			return *this;
		}

		string_type str() const
		{
			return fmt_.str();
		}

		template <class CharT2, class Traits2, class Alloc2>
		friend std::basic_ostream<CharT2, Traits2>&
			operator<<(std::basic_ostream<CharT2, Traits2>& os,
			const basic_formatter<CharT2, Traits2, Alloc2>& fmt);

		template <class T>
		static basic_formatter& bind(basic_formatter& fmt, const T& x)
		{
			return fmt(x);
		}

		template <class Head, class... Tail>
		static basic_formatter& bind(basic_formatter& fmt, const Head& x, const Tail&... xs)
		{
			return bind(fmt(x), xs...);
		}

	private:
		format_type fmt_;
	};

	typedef basic_formatter<char>       formatter;
	typedef basic_formatter<wchar_t>    wformatter;

	template <class CharT2, class Traits2, class Alloc2>
	inline std::basic_ostream<CharT2, Traits2>&
		operator<<(std::basic_ostream<CharT2, Traits2>& os,
		const basic_formatter<CharT2, Traits2, Alloc2>& fmt)
	{
		return os << fmt.fmt_;
	}


	template <class CharT, class Traits, class Alloc>
	inline basic_formatter<CharT, Traits, Alloc>
		format(const std::basic_string<CharT, Traits, Alloc>& fmt)
	{
		return basic_formatter<CharT, Traits, Alloc>(fmt);
	}

	template <class CharT, class Traits, class Alloc, class... Args>
	inline basic_formatter<CharT, Traits, Alloc>
		format(const std::basic_string<CharT, Traits, Alloc>& fmt, const Args&... args)
	{
		typedef basic_formatter<CharT, Traits, Alloc> format_type;
		format_type f(fmt);
		return format_type::bind(f, args...);
	}

	template <class CharT>
	inline basic_formatter<CharT> format(const CharT* fmt)
	{
		return basic_formatter<CharT>(fmt);
	}

	template <class CharT, class... Args>
	inline basic_formatter<CharT> format(const CharT* fmt, const Args&... args)
	{
		return format<CharT, std::char_traits<CharT>, std::allocator<CharT>, Args...>(fmt, args...);
	}

	template <class... Args>
	inline void print(const std::string& fmt, const Args&... args)
	{
		std::cout << format(fmt, args...);
	}

	template <class... Args>
	inline void print(const std::wstring& fmt, const Args&... args)
	{
		std::wcout << format(fmt, args...);
	}

	template <class CharT, class... Args>
	inline void print(const CharT* fmt, const Args&... args)
	{
		print(std::basic_string<CharT>(fmt), args...);
	}

	template <class... Args>
	inline void println(const std::string& fmt, const Args&... args)
	{
		std::cout << format(fmt, args...) << std::endl;
	}

	template <class... Args>
	inline void println(const std::wstring& fmt, const Args&... args)
	{
		std::wcout << format(fmt, args...) << std::endl;
	}

	template <class CharT, class... Args>
	inline void println(const CharT* fmt, const Args&... args)
	{
		println(std::basic_string<CharT>(fmt), args...);
	}

} // namespace shand

#endif // SHAND_FORMAT_INCLUDE
