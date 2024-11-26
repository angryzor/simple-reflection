#pragma once
#include <tuple>
#include <algorithm>
#include <array>
#include <string>
#include <string_view>

namespace simplerfl {
    template <size_t N>
    struct strlit {
        std::array<char, N> buffer{};

        constexpr strlit(const auto... characters) : buffer{ characters..., '\0' } {}
        constexpr strlit(const std::array<char, N> buffer) : buffer{ buffer } {}
        constexpr strlit(const char(&str)[N]) { std::copy_n(str, N, std::data(buffer)); }

        constexpr operator const char* () const { return std::data(buffer); }
        constexpr operator std::string() const { return std::string(std::data(buffer), N - 1); }
        constexpr operator std::string_view() const { return std::string_view(std::data(buffer), N - 1); }
    };

    template<size_t descType> struct decl {
        static constexpr size_t desc_type = descType;
    };
    struct mod_base {};
    template<size_t modId> struct mod : mod_base {
        static constexpr size_t mod_id = modId;
    };

    static constexpr size_t DESCTYPE_PRIMITIVE = 0x5052494D54495645;
    template<typename Repr> struct primitive : decl<DESCTYPE_PRIMITIVE> {
        using repr = Repr;
    };

    // Canonical declarations are the default declaration, used if the type is just passed as an argument.
    template<typename T, typename = void>
    struct canonical {
        using type = T;
    };
    template<typename T> using canonical_t = typename canonical<T>::type;

    // Handles friendly use of just specifying the type instead of wrapping everything.
    // If a canonical declaration exists, use that. Otherwise assume it's a declaration.
    template<typename Type> struct resolve_decl { using type = canonical_t<Type>; };
    template<typename Type> using resolve_decl_t = resolve_decl<Type>::type;

    template<typename Type, strlit name> struct field {
        using type = resolve_decl_t<Type>;
        static constexpr strlit name = name;
    };

    static constexpr size_t DESCTYPE_STRUCTURE = 0x5354525543545245;
    template<typename Repr, strlit name, typename Base, typename... Fields> struct structure : decl<DESCTYPE_STRUCTURE> {
        using repr = Repr;
        static constexpr strlit name = name;
        using base = resolve_decl_t<Base>;
        using fields = std::tuple<Fields...>;
    };

    template<typename Parent> using union_resolver = size_t (*) (const Parent& parent);
    static constexpr size_t DESCTYPE_UNION = 0x554e494f4e303030;
    template<typename Repr, strlit name, typename Parent, union_resolver<Parent> resolver, typename... Fields> struct unionof : decl<DESCTYPE_UNION> {
        using repr = Repr;
        static constexpr strlit name = name;
        using parent = Parent;
        static constexpr union_resolver<Parent> resolver = resolver;
        using fields = std::tuple<Fields...>;
    };

    template<strlit name> struct option {
        static constexpr strlit name = name;
    };

    template<strlit name, long long value> struct fixed_option {
        static constexpr strlit name = name;
        static constexpr long long value = value;
    };

    static constexpr size_t DESCTYPE_ENUMERATION = 0x454e554d5241544e;
    template<typename Repr, strlit name, typename Underlying, typename... Options> struct enumeration : decl<DESCTYPE_ENUMERATION> {
        using repr = Repr;
        static constexpr strlit name = name;
        using underlying = Underlying;
        using options = std::tuple<Options...>;
    };

    template<typename Type> using dynamic_resolver = size_t (*) (const Type& parent);
    template<typename Parent> using array_size_resolver = size_t (*) (const Parent& parent);

    static constexpr size_t DESCTYPE_DYNAMIC = 0x44594E414D494330;
    template<typename Base, typename Parent, dynamic_resolver<Parent> resolver, typename... Types> struct dynamic : decl<DESCTYPE_DYNAMIC> {
        using base = Base;
        using parent = Parent;
        static constexpr dynamic_resolver<Parent> resolver = resolver;
        using types = std::tuple<Types...>;
    };

    static constexpr size_t DESCTYPE_DYNAMIC_SELF = 0x44594E414D494353;
    template<typename Base, dynamic_resolver<Base> resolver, typename... Types> struct dynamic_self : decl<DESCTYPE_DYNAMIC_SELF> {
        using base = Base;
        static constexpr dynamic_resolver<Base> resolver = resolver;
        using types = std::tuple<Types...>;
    };

    static constexpr size_t DESCTYPE_POINTER = 0x504F494E54455230;
    template<typename Type> struct pointer : decl<DESCTYPE_POINTER> {
        using target = resolve_decl_t<Type>;
    };

    static constexpr size_t DESCTYPE_DYNAMIC_CARRAY = 0x44594E4143415252;
    template<typename Type, typename Parent, array_size_resolver<Parent> resolver> struct dynamic_carray : decl<DESCTYPE_DYNAMIC_CARRAY> {
        using type = resolve_decl_t<Type>;
        using parent = Parent;
        static constexpr array_size_resolver<Parent> resolver = resolver;
    };

    static constexpr size_t DESCTYPE_STATIC_CARRAY = 0x5354415443415252;
    template<typename Type, size_t size> struct static_carray : decl<DESCTYPE_STATIC_CARRAY> {
        using type = resolve_decl_t<Type>;
        static constexpr size_t size = size;
    };

    static constexpr size_t MOD_ALIGNED = 0x414c49474e454430;
    template<size_t alignment, typename Type> struct aligned : mod<MOD_ALIGNED> {
        static constexpr size_t alignment = alignment;
        using type = resolve_decl_t<Type>;
    };

    template<typename Type> struct is_realigned { static constexpr bool value = is_realigned<Type::type>::value; };
    template<typename T> struct is_realigned<simplerfl::primitive<T>> { static constexpr bool value = false; };
    template<typename Repr, simplerfl::strlit name, typename Underlying, typename... Options> struct is_realigned<simplerfl::enumeration<Repr, name, Underlying, Options...>> { static constexpr bool value = false; };
    template<typename T> struct is_realigned<simplerfl::pointer<T>> { static constexpr bool value = false; };
    template<typename Repr, simplerfl::strlit name, typename Base, typename... Fields> struct is_realigned<simplerfl::structure<Repr, name, Base, Fields...>> { static constexpr bool value = false; };
    template<typename Repr, simplerfl::strlit name, typename Parent, simplerfl::union_resolver<Parent> resolver, typename... Fields> struct is_realigned<simplerfl::unionof<Repr, name, Parent, resolver, Fields...>> { static constexpr bool value = false; };
    template<size_t alignment, typename Type> struct is_realigned<aligned<alignment, Type>> { static constexpr bool value = true; };
    template<typename Type> static constexpr bool is_realigned_v = is_realigned<Type>::value;

    // Some simple canonical implementations.
    //template<typename T, size_t Len> struct canonical<T[Len]> { using type = static_carray<T, Len>; };
    template<typename T> struct canonical<T, std::enable_if_t<std::is_fundamental_v<T>>> { using type = primitive<T>; };
    template<typename T> struct canonical<T*> { using type = pointer<canonical_t<T>>; };
    template<> struct canonical<const char*> { using type = primitive<const char*>; };
    template<> struct canonical<void*> { using type = primitive<void*>; };
    template<typename T, size_t length> struct canonical<T[length]> { using type = static_carray<T, length>; };

    template<typename Type> static constexpr bool is_modifier_v = std::is_base_of_v<mod_base, Type>;

    template<size_t mod_id, typename Type, bool = is_modifier_v<Type>> struct has_modifier;
    template<size_t mod_id, typename Type> struct has_modifier<mod_id, Type, true> { static constexpr bool value = Type::mod_id == mod_id || has_modifier<mod_id, typename Type::type>::value; };
    template<size_t mod_id, typename Type> struct has_modifier<mod_id, Type, false> { static constexpr bool value = false; };
    template<size_t mod_id, typename Type> static constexpr bool has_modifier_v = has_modifier<mod_id, Type>::value;

    //template<size_t mod_id, typename Type, bool = is_modifier_v<Type>> struct get_modifier;
    //template<size_t mod_id, typename Type> struct get_modifier<mod_id, Type, true> { using type = std::conditional_t<Type::mod_id == mod_id, >; };
    //template<size_t mod_id, typename Type> struct get_modifier<mod_id, Type, false> { using type = void; };
    //template<size_t mod_id, typename Type> using get_modifier_t = get_modifier<mod_id, Type>::type;

    template<typename Type, bool = is_modifier_v<Type>> struct desugar;
    template<typename Type> struct desugar<Type, true> { using type = typename desugar<typename Type::type>::type; };
    template<typename Type> struct desugar<Type, false> { using type = Type; };
    template<typename Type> using desugar_t = desugar<Type>::type;

    template<typename Type> struct representation;
    template<typename Type> using representation_t = typename representation<Type>::type;

    template<typename Repr> struct representation<primitive<Repr>> { using type = typename primitive<Repr>::repr; };
    template<typename Repr, strlit name, typename Underlying, typename... Options> struct representation<enumeration<Repr, name, Underlying, Options...>> { using type = Repr; };
    template<typename Type> struct representation<pointer<Type>> { using type = typename representation_t<typename pointer<Type>::target>*; };
    template<typename Base, typename Parent, dynamic_resolver<Parent> resolver, typename... Types> struct representation<dynamic<Base, Parent, resolver, Types...>> { using type = typename dynamic<Base, Parent, resolver, Types...>::base; };
    template<typename Base, dynamic_resolver<Base> resolver, typename... Types> struct representation<dynamic_self<Base, resolver, Types...>> { using type = typename dynamic_self<Base, resolver, Types...>::base; };
    template<typename Type, typename Parent, array_size_resolver<Parent> resolver> struct representation<dynamic_carray<Type, Parent, resolver>> { using type = typename dynamic_carray<Type, Parent, resolver>::type[]; };
    template<typename Type, size_t size> struct representation<static_carray<Type, size>> { using type = typename static_carray<Type, size>::type[size]; };
    template<typename Repr, strlit name, typename Base, typename... Fields> struct representation<structure<Repr, name, Base, Fields...>> { using type = Repr; };
    template<typename Repr, strlit name, typename Parent, union_resolver<Parent> resolver, typename... Fields> struct representation<unionof<Repr, name, Parent, resolver, Fields...>> { using type = Repr; };

    template<typename Type> struct size_of { static constexpr size_t value = sizeof(representation_t<desugar_t<Type>>); };
    template<typename Type, typename Parent, array_size_resolver<Parent> resolver> struct size_of<dynamic_carray<Type, Parent, resolver>> { static_assert("Cannot take size of a dynamic array!"); };
    template<typename Type> constexpr size_t size_of_v = size_of<Type>::value;

    template<typename Type> struct align_of { static constexpr size_t value = alignof(representation_t<desugar_t<Type>>); };
    template<size_t alignment, typename Type> struct align_of<aligned<alignment, Type>> { static constexpr size_t value = alignment; };
    template<typename Type, typename Parent, array_size_resolver<Parent> resolver> struct align_of<dynamic_carray<Type, Parent, resolver>> { static constexpr size_t value = align_of<typename dynamic_carray<Type, Parent, resolver>::type>::value; }; // TODO: Fix this. currently only works when top level.
    template<typename Type> constexpr size_t align_of_v = align_of<Type>::value;

    template<typename Type>
    size_t dynamic_size_of(void* parent, void* self) {
        if constexpr (desugar_t<Type>::desc_type == DESCTYPE_DYNAMIC_CARRAY)
            return desugar_t<Type>::resolver(*(typename desugar_t<Type>::parent*)parent) * dynamic_size_of<typename desugar_t<Type>::type>(parent, self);
        else
            return size_of_v<Type>;
    }
}
