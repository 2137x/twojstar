#pragma once

#include <array>
#include <cstddef>

namespace Travny::Audio {

class DeHumDsp final
{
public:
    static constexpr std::size_t kNotchCount = 6;
    static constexpr double kQuality = 35.0;

    DeHumDsp() noexcept { configure(48000.0); }

    void configure(double sampleRate) noexcept;
    void reset() noexcept;

    template <typename Sample>
    Sample processSample(Sample input) noexcept
    {
        return static_cast<Sample>(processSampleImpl(static_cast<double>(input)));
    }

private:
    struct Biquad
    {
        double b0 = 1.0;
        double b1 = 0.0;
        double b2 = 0.0;
        double a1 = 0.0;
        double a2 = 0.0;
        double z1 = 0.0;
        double z2 = 0.0;

        void configureNotch(double sampleRate, double frequency, double quality) noexcept;
        void reset() noexcept;
        double process(double input) noexcept;
    };

    double processSampleImpl(double input) noexcept;

    std::array<Biquad, kNotchCount> filters_{};
    double sampleRate_ = 48000.0;
};

} // namespace Travny::Audio
