#include "DeClickDsp.h"

#include <algorithm>
#include <cmath>

namespace Travny::Audio {

void DeClickDsp::reset() noexcept
{
    buffer_.fill(0.0);
    nextIndex_ = 0;
}

double DeClickDsp::sampleAt(std::uint64_t index) const noexcept
{
    return buffer_[static_cast<std::size_t>(index % kBufferSize)];
}

double& DeClickDsp::sampleAt(std::uint64_t index) noexcept
{
    return buffer_[static_cast<std::size_t>(index % kBufferSize)];
}

double DeClickDsp::processSampleImpl(double input) noexcept
{
    const std::uint64_t index = nextIndex_++;
    sampleAt(index) = input;

    if (index >= 4)
    {
        repairCandidate(index - kLatencySamples);
    }

    if (index < kLatencySamples)
    {
        return 0.0;
    }

    return sampleAt(index - kLatencySamples);
}

void DeClickDsp::repairCandidate(std::uint64_t index) noexcept
{
    const double left2 = sampleAt(index - 2);
    const double left1 = sampleAt(index - 1);
    const double center = sampleAt(index);
    const double right1 = sampleAt(index + 1);
    const double right2 = sampleAt(index + 2);

    if (!std::isfinite(left2) || !std::isfinite(left1) || !std::isfinite(center)
        || !std::isfinite(right1) || !std::isfinite(right2))
    {
        return;
    }

    const double leftSlope = left1 - left2;
    const double rightSlope = right2 - right1;
    const double bridge = right1 - left1;
    const bool smoothContext = std::abs(leftSlope) <= kContextDelta
        && std::abs(rightSlope) <= kContextDelta
        && std::abs(bridge) <= kContextDelta;

    if (!smoothContext)
    {
        return;
    }

    const double predicted = 0.5 * (left1 + right1);
    const double residual = std::abs(center - predicted);
    const double edgeResidual = std::min(std::abs(center - left1), std::abs(center - right1));
    const double localMotion = std::max({std::abs(leftSlope), std::abs(rightSlope), std::abs(bridge), 0.02});
    const bool isolatedExtremum = (center - left1) * (center - right1) > 0.0;

    if (!isolatedExtremum || residual < kImpulseResidual || edgeResidual < 0.25
        || residual < 3.0 * localMotion)
    {
        return;
    }

    const double contextMin = std::min({left2, left1, right1, right2});
    const double contextMax = std::max({left2, left1, right1, right2});
    sampleAt(index) = std::clamp(predicted, contextMin, contextMax);
}

} // namespace Travny::Audio
