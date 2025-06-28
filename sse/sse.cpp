#include "sse.h"

#include <memory>
#include <immintrin.h>

struct OptimizedModel {
    std::vector<int> indices;
    std::vector<float> thresholds;
    std::vector<float> values;
    size_t size;
};

std::shared_ptr<OptimizedModel> Optimize(const Model& model) {
    auto optimized = std::make_shared<OptimizedModel>();
    optimized->size = ((model.size() + 1) / 4) * 4;
    for (size_t i = 0; i < model.size(); ++i) {
        optimized->indices.push_back(model[i].index);
        optimized->thresholds.push_back(static_cast<float>(model[i].threshold));
        optimized->values.push_back(static_cast<float>(model[i].value));
    }
    for (size_t i = model.size(); i < optimized->size; ++i) {
        optimized->indices.push_back(0);
        optimized->thresholds.push_back(-1.0f);
        optimized->values.push_back(0.0f);
    }
    return optimized;
}

double ApplyOptimizedModel(const OptimizedModel& model, const std::vector<float>& features) {
    __m128 result = _mm_setzero_ps();
    for (size_t i = 0; i < model.size; i += 4) {
        __m128i indices = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&model.indices[i]));
        __m128 thresholds = _mm_loadu_ps(&model.thresholds[i]);
        __m128 values = _mm_loadu_ps(&model.values[i]);
        __m128 feature_values = _mm_set_ps(
                features[_mm_extract_epi32(indices, 3)],
                features[_mm_extract_epi32(indices, 2)],
                features[_mm_extract_epi32(indices, 1)],
                features[_mm_extract_epi32(indices, 0)]
        );
        __m128 mask = _mm_cmpgt_ps(feature_values, thresholds);
        __m128 masked_values = _mm_and_ps(values, mask);
        result = _mm_add_ps(result, masked_values);
    }
    __m128 tmp = _mm_add_ps(result, _mm_movehl_ps(result, result));
    __m128 sum = _mm_add_ss(tmp, _mm_shuffle_ps(tmp, tmp, 1));
    return _mm_cvtss_f32(sum);
}
