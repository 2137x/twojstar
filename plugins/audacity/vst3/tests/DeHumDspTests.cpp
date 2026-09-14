#include "DeHumDsp.h"

#include <cmath>
#include <cstdlib>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

using Travny::Audio::DeHumDsp;

namespace {
constexpr double kPi = 3.14159265358979323846;
constexpr double kSampleRate = 48000.0;

void require(bool condition, const char* message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

double renderToneRatio(double frequency)
{
    DeHumDsp dsp;
    dsp.configure(kSampleRate);

    constexpr std::size_t totalSamples = std::size_t{3} * 48000;
    constexpr std::size_t measureStart = std::size_t{2} * 48000;
    double outputEnergy = 0.0;
    std::size_t measured = 0;

    for (std::size_t i = 0; i < totalSamples; ++i)
    {
        const double input = 0.5 * std::sin(2.0 * kPi * frequency * static_cast<double>(i) / kSampleRate);
        const double output = dsp.processSample(input);
        if (i >= measureStart)
        {
            outputEnergy += output * output;
            ++measured;
        }
    }

    const double outputRms = std::sqrt(outputEnergy / static_cast<double>(measured));
    const double inputRms = 0.5 / std::sqrt(2.0);
    return outputRms / inputRms;
}

void testMainsFamiliesAreSuppressed()
{
    require(renderToneRatio(50.0) < 0.02, "50 Hz hum was not suppressed");
    require(renderToneRatio(60.0) < 0.02, "60 Hz hum was not suppressed");
    require(renderToneRatio(100.0) < 0.02, "100 Hz harmonic was not suppressed");
    require(renderToneRatio(120.0) < 0.02, "120 Hz harmonic was not suppressed");
    require(renderToneRatio(150.0) < 0.02, "150 Hz harmonic was not suppressed");
    require(renderToneRatio(180.0) < 0.02, "180 Hz harmonic was not suppressed");
}

void testMusicBandIsPreserved()
{
    require(renderToneRatio(440.0) > 0.995, "440 Hz tone was changed too much");
    require(renderToneRatio(1000.0) > 0.995, "1 kHz tone was changed too much");
}

void testNonFiniteSampleDoesNotPoisonState()
{
    DeHumDsp dsp;
    dsp.configure(kSampleRate);
    for (std::size_t i = 0; i < 4096; ++i)
    {
        (void)dsp.processSample(0.2 * std::sin(2.0 * kPi * 50.0 * static_cast<double>(i) / kSampleRate));
    }

    const double nan = std::numeric_limits<double>::quiet_NaN();
    require(std::isnan(dsp.processSample(nan)), "non-finite input should pass through locally");
    require(std::isfinite(dsp.processSample(0.0)), "non-finite input poisoned filter state");
}

void testResetIsDeterministic()
{
    DeHumDsp dsp;
    dsp.configure(kSampleRate);
    std::vector<double> first;
    std::vector<double> second;
    first.reserve(4096);
    second.reserve(4096);

    for (std::size_t i = 0; i < 4096; ++i)
    {
        const double input = 0.25 * std::sin(2.0 * kPi * 60.0 * static_cast<double>(i) / kSampleRate);
        first.push_back(dsp.processSample(input));
    }

    dsp.reset();
    for (std::size_t i = 0; i < 4096; ++i)
    {
        const double input = 0.25 * std::sin(2.0 * kPi * 60.0 * static_cast<double>(i) / kSampleRate);
        second.push_back(dsp.processSample(input));
    }

    require(first == second, "reset did not restore deterministic filter state");
}

void testReportedTailCoversFilterDecay()
{
    DeHumDsp dsp;
    dsp.configure(kSampleRate);
    (void)dsp.processSample(1.0);

    const auto tailSamples = DeHumDsp::tailSamplesForRate(kSampleRate);
    double output = 0.0;
    for (std::uint32_t i = 0; i < tailSamples; ++i)
    {
        output = dsp.processSample(0.0);
    }

    require(std::abs(output) < 1e-7, "reported de-hum tail ended before filter decay was negligible");
}

void testInvalidSampleRateFallsBackSafely()
{
    DeHumDsp dsp;
    dsp.configure(std::numeric_limits<double>::quiet_NaN());
    require(std::isfinite(dsp.processSample(0.25)), "invalid sample rate produced non-finite output");
}

} // namespace

int main()
{
    try
    {
        testMainsFamiliesAreSuppressed();
        testMusicBandIsPreserved();
        testNonFiniteSampleDoesNotPoisonState();
        testResetIsDeterministic();
        testReportedTailCoversFilterDecay();
        testInvalidSampleRateFallsBackSafely();
        std::cout << "DeHum DSP tests passed\n";
        return EXIT_SUCCESS;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
