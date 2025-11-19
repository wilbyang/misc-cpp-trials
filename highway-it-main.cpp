#include <iostream>
#include <vector>

namespace project {
void AddVectors(const float* a, const float* b, float* out, size_t count);
void alpha_blend_inplace(const uint8_t* src_rgba, uint8_t* dest_rgb_in_out, int width, int height);
}

int main() {
    const size_t N = 16;
    std::vector<float> a(N);
    std::vector<float> b(N);
    std::vector<float> result(N);

    // Initialize test data
    for (size_t i = 0; i < N; ++i) {
        a[i] = static_cast<float>(i);
        b[i] = static_cast<float>(i * 2);
    }

    // Call vectorized addition
    project::AddVectors(a.data(), b.data(), result.data(), N);

    // Print results
    std::cout << "Highway SIMD Vector Addition Test:\n";
    for (size_t i = 0; i < N; ++i) {
        std::cout << a[i] << " + " << b[i] << " = " << result[i] << "\n";
    }

    std::cout << "\nTest completed successfully!\n";
    return 0;
}
