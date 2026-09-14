#pragma once

namespace Travny::Audio {

inline bool denoiseEnabledFromNormalized(double value) noexcept
{
    return value >= 0.5;
}

inline double denoiseEnabledToNormalized(bool enabled) noexcept
{
    return enabled ? 1.0 : 0.0;
}

} // namespace Travny::Audio
