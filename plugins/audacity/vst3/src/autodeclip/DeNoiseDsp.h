#pragma once

#include <cmath>

namespace Travny::Audio {

class DeNoiseDsp final
{
public:
    static constexpr double kThreshold = 0.004;
    static constexpr double kFloorGain = 0.25;
    static constexpr double kEnvelopeAttackSeconds = 0.002;
    static constexpr double kEnvelopeReleaseSeconds = 0.080;
    static constexpr double kGainAttackSeconds = 0.002;
    static constexpr double kGainReleaseSeconds = 0.120;

    DeNoiseDsp() noexcept { configure(48000.0); }

    void configure(double sampleRate) noexcept;
    void reset() noexcept;

    template <typename Sample>
    Sample processSample(Sample input) noexcept
    {
        return static_cast<Sample>(processSampleImpl(static_cast<double>(input)));
    }

private:
    static double smoothingCoefficient(double sampleRate, double seconds) noexcept;
    static double targetGain(double envelope) noexcept;
    double processSampleImpl(double input) noexcept;

    double envelopeAttack_ = 0.0;
    double envelopeRelease_ = 0.0;
    double gainAttack_ = 0.0;
    double gainRelease_ = 0.0;
    double envelope_ = 0.0;
    double gain_ = kFloorGain;
};

} // namespace Travny::Audio
