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

        operator const char* () const { return std::data(buffer); }
        operator std::string() const { return std::string(std::data(buffer), N - 1); }
        constexpr operator std::string_view() const { return std::string_view(std::data(buffer), N - 1); }
    };

    template<size_t descType> struct decl {
        static constexpr size_t desc_type = descType;
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

    template<typename Type> using dynamic_resolver = size_t (*) (const Type& parent);
    template<typename Parent> using array_size_resolver = size_t (*) (const Parent& parent);

    static constexpr size_t DESCTYPE_DYNAMIC = 0x44594E414D494330;
    template<typename Base, typename Parent, dynamic_resolver<Parent> resolver, typename... Types> struct dynamic : decl<DESCTYPE_DYNAMIC> {
        static constexpr dynamic_resolver<Parent> resolver = resolver;
        using types = std::tuple<Types...>;
    };

    static constexpr size_t DESCTYPE_DYNAMIC_SELF = 0x44594E414D494353;
    template<typename Base, dynamic_resolver<Base> resolver, typename... Types> struct dynamic_self : decl<DESCTYPE_DYNAMIC_SELF> {
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

    template<size_t alignment, typename Type> struct aligned {
        static constexpr size_t alignment = alignment;
        using type = resolve_decl_t<Type>;
    };

    // Some simple canonical implementations.
    //template<typename T, size_t Len> struct canonical<T[Len]> { using type = static_carray<T, Len>; };
    template<typename T> struct canonical<T, std::enable_if_t<std::is_fundamental_v<T>>> { using type = primitive<T>; };
    template<typename T> struct canonical<T*> { using type = pointer<canonical_t<T>>; };
    template<> struct canonical<const char*> { using type = primitive<const char*>; };
    template<> struct canonical<void*> { using type = primitive<void*>; };

    template<typename Type> struct desugar { using type = Type; };
    template<size_t alignment, typename Type> struct desugar<aligned<alignment, Type>> { using type = typename desugar<typename aligned<alignment, Type>::type>::type; };
    template<typename Type> using desugar_t = desugar<Type>::type;

    template<typename Type> struct representation_of_type { static_assert("Attempt to get representation_of_type of invalid reflection description."); };
    template<typename Type> using representation_of_type_t = typename representation_of_type<Type>::type;

    template<typename Type> struct representation { using type = representation_of_type_t<desugar_t<Type>>; };
    template<typename Type> using representation_t = typename representation<Type>::type;

    template<typename Repr> struct representation_of_type<primitive<Repr>> { using type = Repr; };
    template<typename Type> struct representation_of_type<pointer<Type>> { using type = typename representation_t<Type>*; };
    template<typename Base, typename Parent, dynamic_resolver<Parent> resolver, typename... Types> struct representation_of_type<dynamic<Base, Parent, resolver, Types...>> { using type = Base; };
    template<typename Base, dynamic_resolver<Base> resolver, typename... Types> struct representation_of_type<dynamic_self<Base, resolver, Types...>> { using type = Base; };
    template<typename Type, typename Parent, array_size_resolver<Parent> resolver> struct representation_of_type<dynamic_carray<Type, Parent, resolver>> { using type = Type[]; };
    template<typename Type, size_t size> struct representation_of_type<static_carray<Type, size>> { using type = Type[size]; };
    template<typename Repr, strlit name, typename Base, typename... Fields> struct representation_of_type<structure<Repr, name, Base, Fields...>> { using type = Repr; };

    template<typename Type> struct size_of { static constexpr size_t value = sizeof(representation_t<Type>); };
    template<typename Type, typename Parent, array_size_resolver<Parent> resolver> struct size_of<dynamic_carray<Type, Parent, resolver>> { static_assert("Cannot take size of a dynamic array!"); };
    template<typename Type> constexpr size_t size_of_v = size_of<Type>::value;

    template<typename Type> struct align_of { static constexpr size_t value = alignof(representation_t<Type>); };
    template<size_t alignment, typename Type> struct align_of<aligned<alignment, Type>> { static constexpr size_t value = alignment; };
    template<typename Type, typename Parent, array_size_resolver<Parent> resolver> struct align_of<dynamic_carray<Type, Parent, resolver>> {
        static constexpr size_t value = align_of<typename dynamic_carray<Type, Parent, resolver>::type>::value;
    };
    template<typename Type> constexpr size_t align_of_v = align_of<Type>::value;

    template<typename Type>
    size_t dynamic_size_of(void* parent, void* self) {
        if constexpr (desugar_t<Type>::desc_type == DESCTYPE_DYNAMIC_CARRAY)
            return desugar_t<Type>::resolver(*(typename desugar_t<Type>::parent*)parent) * dynamic_size_of<typename desugar_t<Type>::type>(parent, self);
        else
            return size_of_v<Type>;
    }
}
