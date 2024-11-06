#include <simple-reflection/simple-reflection.h>

using namespace simplerfl;
using namespace std;

using AlignedInt = aligned<16, int>;
using AlignedIntInner = AlignedInt::type;
using O = resolve_decl_t<int>;
using Rep = representation_t<AlignedInt>;

using Test1 = structure<"Test1", void,
	field<float, "foo">,
	field<AlignedInt, "bar">
>;

static_assert(is_same_v<canonical_t<char>, primitive<char>>);
static_assert(is_same_v<canonical_t<unsigned char>, primitive<unsigned char>>);
static_assert(is_same_v<canonical_t<short>, primitive<short>>);
static_assert(is_same_v<canonical_t<unsigned short>, primitive<unsigned short>>);
static_assert(is_same_v<canonical_t<int>, primitive<int>>);
static_assert(is_same_v<canonical_t<unsigned int>, primitive<unsigned int>>);
static_assert(is_same_v<canonical_t<float>, primitive<float>>);
static_assert(is_same_v<canonical_t<double>, primitive<double>>);

static_assert(is_same_v<canonical_t<const char*>, primitive<const char*>>);
static_assert(is_same_v<canonical_t<void*>, primitive<void*>>);
static_assert(is_same_v<canonical_t<Test1>, Test1>);

static_assert(is_same_v<desugar_t<AlignedInt>, primitive<int>>);
static_assert(is_same_v<representation_t<primitive<int>>, int>);
static_assert(is_same_v<representation_t<AlignedInt>, int>);
static_assert(is_same_v<representation_t<primitive<void*>>, void*>);

static_assert(align_of_v<AlignedInt> == 16);
static_assert(size_of_v<primitive<int>> == 4);
static_assert(size_of_v<AlignedInt> == 4);


int main() {

}