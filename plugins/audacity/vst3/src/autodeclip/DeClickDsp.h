#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace Travny::Audio {

class DeClickDsp final
{
public:
    static constexpr std::size_t kLatencySamples = 2;
    static constexpr double kImpulseResidual = 0.30;
    static constexpr double kContextDelta = 0.12;

    DeClickDsp() noexcept { reset(); }

    void reset() noexcept;

    template <typename Sample>
    Sample processSample(Sample input) noexcept
    {
        return static_cast<Sample>(processSampleImpl(static_cast<double>(input)));
    }

private:
    static constexpr std::size_t kBufferSize = 8;
    double processSampleImpl(double input) noexcept;
    void repairCandidate(std::uint64_t index) noexcept;
    double sampleAt(std::uint64_t index) const noexcept;
    double& sampleAt(std::uint64_t index) noexcept;

    std::array<double, kBufferSize> buffer_{};
    std::uint64_t nextIndex_ = 0;
};

} // namespace Travny::Audio
