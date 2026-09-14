#include "DeNoiseDsp.h"

#include <algorithm>
#include <cmath>

namespace Travny::Audio {

double DeNoiseDsp::smoothingCoefficient(double sampleRate, double seconds) noexcept
{
    const double safeSampleRate = std::isfinite(sampleRate) && sampleRate > 1000.0 ? sampleRate : 48000.0;
    const double safeSeconds = std::max(seconds, 1e-6);
    return std::exp(-1.0 / (safeSampleRate * safeSeconds));
}

void DeNoiseDsp::configure(double sampleRate) noexcept
{
    envelopeAttack_ = smoothingCoefficient(sampleRate, kEnvelopeAttackSeconds);
    envelopeRelease_ = smoothingCoefficient(sampleRate, kEnvelopeReleaseSeconds);
    gainAttack_ = smoothingCoefficient(sampleRate, kGainAttackSeconds);
    gainRelease_ = smoothingCoefficient(sampleRate, kGainReleaseSeconds);
    reset();
}

void DeNoiseDsp::reset() noexcept
{
    envelope_ = 0.0;
    gain_ = kFloorGain;
}
double DeNoiseDsp::targetGain(double envelope) noexcept
{
    const double kneeStart = kThreshold * 0.5;
    if (envelope <= kneeStart)
    {
        return kFloorGain;
    }
    if (envelope >= kThreshold)
    {
        return 1.0;
    }

    const double x = (envelope - kneeStart) / (kThreshold - kneeStart);
    const double smooth = x * x * (3.0 - 2.0 * x);
    return kFloorGain + (1.0 - kFloorGain) * smooth;
}

double DeNoiseDsp::processSampleImpl(double input) noexcept
{
    if (!std::isfinite(input))
    {
        reset();
        return input;
    }

    const double magnitude = std::abs(input);
    const double envelopeCoeff = magnitude > envelope_ ? envelopeAttack_ : envelopeRelease_;
    envelope_ = envelopeCoeff * envelope_ + (1.0 - envelopeCoeff) * magnitude;

    const double wantedGain = targetGain(envelope_);
    const double gainCoeff = wantedGain > gain_ ? gainAttack_ : gainRelease_;
    gain_ = gainCoeff * gain_ + (1.0 - gainCoeff) * wantedGain;

    return input * gain_;
}

} // namespace Travny::Audio
