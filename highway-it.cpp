#undef HWY_TARGET_INCLUDE
#define HWY_TARGET_INCLUDE "highway-it.cpp"
#include "hwy/foreach_target.h"
#include "hwy/highway.h"

HWY_BEFORE_NAMESPACE();
namespace project {
namespace HWY_NAMESPACE {

namespace hn = hwy::HWY_NAMESPACE;

void AddVectorsImpl(const float* HWY_RESTRICT a, const float* HWY_RESTRICT b, 
                    float* HWY_RESTRICT out, size_t count) {
    const hn::ScalableTag<float> d;
    const size_t N = hn::Lanes(d);

    size_t i = 0;
    for (; i + N <= count; i += N) {
        const auto vec_a = hn::LoadU(d, a + i);
        const auto vec_b = hn::LoadU(d, b + i);
        const auto vec_out = hn::Add(vec_a, vec_b);
        hn::StoreU(vec_out, d, out + i);
    }

    // Handle remaining elements
    for (; i < count; ++i) {
        out[i] = a[i] + b[i];
    }
}

}  // namespace HWY_NAMESPACE
}  // namespace project
HWY_AFTER_NAMESPACE();

#if HWY_ONCE
namespace project {

HWY_EXPORT(AddVectorsImpl);

void AddVectors(const float* a, const float* b, float* out, size_t count) {
    HWY_DYNAMIC_DISPATCH(AddVectorsImpl)(a, b, out, count);
}

}  // namespace project
#endif