#include "DeHumDsp.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace Travny::Audio {
namespace {
constexpr double kPi = 3.14159265358979323846;
constexpr std::array<double, DeHumDsp::kNotchCount> kHumFrequencies{
    50.0, 60.0, 100.0, 120.0, 150.0, 180.0};
}

void DeHumDsp::Biquad::configureNotch(double sampleRate, double frequency, double quality) noexcept
{
    const double omega = 2.0 * kPi * frequency / sampleRate;
    const double cosine = std::cos(omega);
    const double alpha = std::sin(omega) / (2.0 * quality);
    const double a0 = 1.0 + alpha;

    b0 = 1.0 / a0;
    b1 = (-2.0 * cosine) / a0;
    b2 = 1.0 / a0;
    a1 = (-2.0 * cosine) / a0;
    a2 = (1.0 - alpha) / a0;
    reset();
}

void DeHumDsp::Biquad::reset() noexcept
{
    z1 = 0.0;
    z2 = 0.0;
}

double DeHumDsp::Biquad::process(double input) noexcept
{
    const double output = b0 * input + z1;
    z1 = b1 * input - a1 * output + z2;
    z2 = b2 * input - a2 * output;
    return output;
}

void DeHumDsp::configure(double sampleRate) noexcept
{
    sampleRate_ = std::isfinite(sampleRate) && sampleRate > 1000.0 ? sampleRate : 48000.0;
    const double nyquist = sampleRate_ * 0.5;

    for (std::size_t i = 0; i < filters_.size(); ++i)
    {
        const double frequency = std::min(kHumFrequencies[i], nyquist * 0.45);
        filters_[i].configureNotch(sampleRate_, frequency, kQuality);
    }
}

void DeHumDsp::reset() noexcept
{
    for (auto& filter : filters_)
    {
        filter.reset();
    }
}

double DeHumDsp::processSampleImpl(double input) noexcept
{
    if (!std::isfinite(input))
    {
        reset();
        return input;
    }

    double output = input;
    for (auto& filter : filters_)
    {
        output = filter.process(output);
    }
    return output;
}

} // namespace Travny::Audio
