#ifndef __CUSTOM_VARIANT__HEADER_FILE
#define __CUSTOM_VARIANT__HEADER_FILE
#include <typeindex>
#include <utility>

template <template <typename T> class F, typename T, typename... args>
struct max_value
{
	enum { value = static_cast<int>(F<T>::value) > static_cast<int>(max_value<F, args...>::value) ?
		static_cast<int>(F<T>::value) : static_cast<int>(max_value<F, args...>::value) };
};

template <template <typename T> class F, typename T>
struct max_value<F, T>
{
	enum { value = F<T>::value };
};

template <typename T, typename U, typename... args>
struct contains
{
	enum { value = std::is_same<T, U>::value ?
		true : contains<T, args...>::value };
};

template <typename T, typename U>
struct contains<T, U>
{
	enum { value = std::is_same<T, U>::value };
};

template <typename T>
struct size_of
{
	enum { value = sizeof(T) };
};

template <typename T>
struct align_of
{
	enum { value = std::alignment_of<T>::value };
};

template <template <typename T> class Hash, typename T, typename... args>
struct variant_helper
{
	static void destroy(void* p, typename Hash<T>::value_type t)
	{
		if (t == Hash<T>::value)
			reinterpret_cast<T*>(p)->~T();
		else
			variant_helper<Hash, args...>::destroy(p, t);
	}
	// copy ctor
	static void copy(void* dest, const void* src, typename Hash<T>::value_type t)
	{
		if (t ==  Hash<T>::value)
			new(dest) T(*reinterpret_cast<const T*>(src));
		else
			variant_helper<Hash, args...>::copy(dest, src, t);
	}
	// move ctor
	static void move(void* dest, void* src, typename Hash<T>::value_type t)
	{
		if (t == Hash<T>::value)
			new(dest) T(std::move(*reinterpret_cast<T*>(src)));
		else
			variant_helper<Hash, args...>::move(dest, src, t);
	}
	// copy assign operator
	static void copy_assign(void* dest, const void* src, typename Hash<T>::value_type t)
	{
		if (t == Hash<T>::value)
			*reinterpret_cast<T*>(dest) = *reinterpret_cast<const T*>(src);
		else
			variant_helper<Hash, args...>::copy_assign(dest, src, t);
	}
	// move assign operator
	static void move_assign(void* dest, void* src, typename Hash<T>::value_type t)
	{
		if (t == Hash<T>::value)
			*reinterpret_cast<T*>(dest) = std::move(*reinterpret_cast<T*>(src));
		else
			variant_helper<Hash, args...>::move_assign(dest, src, t);
	}
};

template <template <typename T> class Hash, typename T>
struct variant_helper<Hash, T>
{
	static void destroy(void* p, typename Hash<T>::value_type)
	{
		reinterpret_cast<T*>(p)->~T();
	}
	static void copy(void* dest, const void* src, typename Hash<T>::value_type t)
	{
		new(dest) T(*reinterpret_cast<const T*>(src));
	}
	static void move(void* dest, void* src, typename Hash<T>::value_type)
	{
		new(dest) T(std::move(*reinterpret_cast<T*>(src)));
	}
	static void copy_assign(void* dest, const void* src, typename Hash<T>::value_type)
	{	
		*reinterpret_cast<T*>(dest) = *reinterpret_cast<const T*>(src);
	}
	static void move_assign(void* dest, void* src, typename Hash<T>::value_type)
	{
		*reinterpret_cast<T*>(dest) = std::move(*reinterpret_cast<T*>(src));
	}
};

template <template <typename T> class Hash, template <typename U> class Fail, typename... args>
class basic_variant final
{
	using helper = variant_helper<Hash, args...>;
public:
	basic_variant(): m_type(Hash<void>::value) {}
	~basic_variant()
	{
		if (m_type != Hash<void>::value) helper::destroy(&buffer, m_type);
	}
	// copy/move ctor (std::forward)
	template <typename T, typename = typename std::enable_if<contains<
		typename std::remove_reference<T>::type, args...>::value>::type>
	basic_variant(T&& v): m_type(Hash<T>::value)
	{
		using U = typename std::remove_reference<T>::type;
		new(&buffer) U(std::forward<T>(v));
	}
	basic_variant(basic_variant&& v): m_type(v.m_type)
	{
		helper::move(&buffer, &v.buffer, m_type);
	}
	basic_variant(const basic_variant& v): m_type(v.m_type)
	{
		helper::copy(&buffer, &v.buffer, m_type);
	}
	template <typename T, typename = typename std::enable_if<contains<
		typename std::remove_reference<T>::type, args...>::value>::type>
	basic_variant& operator=(T&& v)
	{
		using U = typename std::remove_reference<T>::type;
		if (m_type == Hash<U>::value)
		{	// v and this are of the same type, call the corresponding assign operator
			reinterpret_cast<U&>(buffer) = std::forward<T>(v);
		}
		else
		{	// destroy the old variant, and call the corresponding ctor
			if (m_type != Hash<void>::value) helper::destroy(&buffer, m_type);
			new(&buffer) U(std::forward<T>(v));
			m_type = Hash<U>::value;
		}
		return *this;
	}
	basic_variant& operator=(const basic_variant& v)
	{
		if (v.m_type == m_type)
		{	// v and this are of the same type, use copy assign operator
			helper::copy_assign(&buffer, &v.buffer, m_type);
		}
		else
		{	// type not same, call this dtor and use copy ctor
			if (m_type != Hash<void>::value) helper::destroy(&buffer, m_type);
			helper::copy(&buffer, &v.buffer, v.m_type);
			m_type = v.m_type;
		}
		return *this;
	}
	basic_variant& operator=(basic_variant&& v)
	{
		if (v.m_type == m_type)
		{	// v and this are of the same type, use move assign operator
			helper::move_assign(&buffer, &v.buffer, m_type);
		}
		else
		{	// type not same, call this dtor and use move ctor
			if (m_type != Hash<void>::value) helper::destroy(&buffer, m_type);
			helper::move(&buffer, &v.buffer, v.m_type);
			m_type = v.m_type;
		}
		return *this;
	}
	template<typename T> bool is()
	{
		return (m_type == Hash<T>::value);
	}
	template<typename T> T& get()
	{
		if (is<T>()) return reinterpret_cast<T&>(buffer);
		else Fail<T>::execute();
	}
	typename Hash<void>::value_type type() { return m_type; }
private:
	typename std::aligned_storage<max_value<size_of, args...>::value,
		max_value<align_of, args...>::value>::type buffer;
	typename Hash<void>::value_type m_type;
};

template <class T>
struct type_hash
{
	using value_type = std::type_index;
	static const value_type value;
};
template <class T>
const typename type_hash<T>::value_type type_hash<T>::value = std::type_index(typeid(T));
template <class T>
struct err_handler
{
	static void execute()
	{
		throw std::bad_cast();
	}
};

template <class... Args>
using variant = basic_variant<type_hash, err_handler, Args...>;
template <template <class T> class Handler, class... Args>
using callback_variant = basic_variant<type_hash, Handler, Args...>;

#endif
#include "unique_variant.h"
