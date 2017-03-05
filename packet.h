#ifndef PACKET__HEADER_FILE
#define PACKET__HEADER_FILE
#include "variant.h"

template <unsigned I, class T, class U, class... Left>
	struct get_index: get_index<I + 1, T, Left...>
		{};
template <unsigned I, class T, class... Left>
	struct get_index<I, T, T, Left...>
		{ static constexpr unsigned value = I; };

template <unsigned I, class T, class... Args>
	struct get_type_by_index: get_type_by_index<I - 1, Args...>
		{};
template <class T, class... Args>
	struct get_type_by_index<0, T, Args...>
		{ using type = T; };

template <class KeyTy, unsigned I, KeyTy X, KeyTy Y, KeyTy... Args>
	struct index_of_helper: index_of_helper<KeyTy, I + 1, X, Args...>
		{};
template <class KeyTy, unsigned I, KeyTy X, KeyTy... Args>
	struct index_of_helper<KeyTy, I, X, X, Args...>
		{ static constexpr unsigned value = I; };
template <class KeyTy, KeyTy X, KeyTy... Args>
	struct index_of: index_of_helper<KeyTy, 0, X, Args...>
		{};

template <template <typename T> class Hash,
		template <typename U, typename... Left> class Fail,
		class KeyTy,
		class... T>
	class basic_packet;
template <template <typename T> class Hash,
		template <typename U, typename... Left> class Fail,
		class KeyTy,
		template <class, KeyTy> class F,
		class... T,
		KeyTy... K>
	class basic_packet<Hash, Fail, KeyTy, F<T, K>...>
	{
		using check_valid = typename std::enable_if<make_and<
				!std::is_function<T>::value...,
				!std::is_void<T>::value...,
				!std::is_array<T>::value...,
				!std::is_reference<T>::value...
			>::value>::type;
		template <class X, class... Args>
		friend struct packet_iterator;
		template <class U, KeyTy I>
			struct unique
				{ U value; };
		template <KeyTy X, KeyTy... Y>
			struct is_unique
			{ static constexpr bool value = make_and<(X != Y)...>::value
					&& is_unique<Y...>::value; };
		template <KeyTy X>
			struct is_unique<X>
			{ static constexpr bool value = true; };
		template <KeyTy X, KeyTy Y, KeyTy... Args>
			struct contains
			{ static constexpr bool value = X == Y ?
					true : contains<X, Args...>::value; };
		template <KeyTy X, KeyTy Y>
			struct contains<X, Y>
			{ static constexpr bool value = X == Y; };
		template <KeyTy X>
			struct get_map
			{ using type = typename get_type_by_index<index_of<KeyTy, X, K...>::value, T...>::type; };
		
		using variant_type = basic_variant<Hash, Fail, unique<T, K>...>;
		template <KeyTy X, KeyTy... Args>
			struct packet_iterator
			{
				static bool make_match(variant_type& var,
					std::function<void(typename get_map<X>::type&)>&& func,
					std::function<void(typename get_map<Args>::type&)>&&... call_back)
				{
					if (var.is<typename get_map<X>::type>())
					{
						func(var.get<unique<typename get_map<X>::type, X>>().value);
						return true;
					}
					return packet_iterator<Args...>::make_match(var,
						std::forward<std::function<void(typename get_map<Args>::type&)>>(call_back)...);
				}
			};
		template <KeyTy X>
			struct packet_iterator<X>
			{
				static bool make_match(variant_type& var,
					std::function<void(typename get_map<X>::type&)>&& func)
				{
					if (var.is<typename get_map<X>::type>())
					{
						func(var.get<unique<typename get_map<X>::type, X>>().value);
						return true;
					}
					return false;
				}
			};
		variant_type var;
	public:
		template <KeyTy X>
			bool is() const
			{ return var.is<unique<typename get_map<X>::type, X>>(); }
		template <KeyTy X>
			typename std::add_lvalue_reference<
				typename get_map<X>::type>::type get()
			{ if (is<X>()) return var.get<unique<typename get_map<X>::type, X>>().value;
				Fail<typename get_map<X>::type>::execute(); }
		template <KeyTy... X, typename = typename
				std::enable_if<make_and<
					is_unique<X...>::value,
					contains<X, K...>::value...
				>::value>::type>
			void make_match(std::function<void(
					typename get_map<X>::type&)>&&... call_back)
			{ if (!packet_iterator<X...>::make_match(var,
					std::forward<std::function<void(typename get_map<X>::type&)>>(call_back)...))
				Fail<typename get_map<X>::type...>::execute(); }
	public:
		template <KeyTy X, class... Args>
			static basic_packet create(Args&&... args)
			{
				using V = typename get_type_by_index<index_of<KeyTy, X, K...>::value, T...>::type;
				basic_packet result;
				result.var = unique<V, X>({V(std::forward<Args>(args)...)});
				return result;
			}
	};
	
template <class K, class... T>
	using custom_packet = basic_packet<type_hash, err_handler, K, T...>;
template <class... T>
	using packet = custom_packet<unsigned, T...>;
template <class T, unsigned K>
	struct binding
		{};

#endif
