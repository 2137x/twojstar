#include "DeNoiseDsp.h"

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

using Travny::Audio::DeNoiseDsp;

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

struct NoiseSource
{
    std::uint32_t state = 0x12345678u;
    double next() noexcept
    {
        state = state * 1664525u + 1013904223u;
        const double unit = static_cast<double>(state) / static_cast<double>(UINT32_MAX);
        return unit * 2.0 - 1.0;
    }
};

double rms(const std::vector<double>& values, std::size_t start)
{
    double energy = 0.0;
    std::size_t count = 0;
    for (std::size_t i = start; i < values.size(); ++i)
    {
        energy += values[i] * values[i];
        ++count;
    }
    return std::sqrt(energy / static_cast<double>(count));
}

void testQuietBroadbandNoiseIsAttenuated()
{
    DeNoiseDsp dsp;
    dsp.configure(kSampleRate);
    NoiseSource noise;
    std::vector<double> input;
    std::vector<double> output;
    input.reserve(96000);
    output.reserve(96000);

    for (std::size_t i = 0; i < 96000; ++i)
    {
        const double sample = 0.0025 * noise.next();
        input.push_back(sample);
        output.push_back(dsp.processSample(sample));
    }

    const auto inputRms = rms(input, 24000);
    const auto outputRms = rms(output, 24000);
    require(outputRms < inputRms * 0.55, "quiet broadband noise was not attenuated enough");
    require(outputRms > inputRms * 0.10, "noise suppression collapsed to a hard mute");
}

void testNormalSignalIsPreserved()
{
    DeNoiseDsp dsp;
    dsp.configure(kSampleRate);
    std::vector<double> input;
    std::vector<double> output;
    input.reserve(48000);
    output.reserve(48000);
    for (std::size_t i = 0; i < 48000; ++i)
    {
        const double sample = 0.15 * std::sin(2.0 * kPi * 1000.0 * static_cast<double>(i) / kSampleRate);
        input.push_back(sample);
        output.push_back(dsp.processSample(sample));
    }

    const auto ratio = rms(output, 4800) / rms(input, 4800);
    require(ratio > 0.985, "normal signal was attenuated by the denoise core");
    require(ratio < 1.015, "denoise core amplified normal signal unexpectedly");
}

void testGateReopensForSignal()
{
    DeNoiseDsp dsp;
    dsp.configure(kSampleRate);
    NoiseSource noise;

    for (std::size_t i = 0; i < 48000; ++i)
    {
        (void)dsp.processSample(0.002 * noise.next());
    }

    double inputEnergy = 0.0;
    double outputEnergy = 0.0;
    for (std::size_t i = 0; i < 4800; ++i)
    {
        const double input = 0.12 * std::sin(2.0 * kPi * 440.0 * static_cast<double>(i) / kSampleRate);
        const double output = dsp.processSample(input);
        if (i >= 960)
        {
            inputEnergy += input * input;
            outputEnergy += output * output;
        }
    }

    const double ratio = std::sqrt(outputEnergy / inputEnergy);
    require(ratio > 0.97, "denoise gate reopened too slowly for wanted signal");
}

void testResetIsDeterministic()
{
    DeNoiseDsp dsp;
    dsp.configure(kSampleRate);
    NoiseSource noise;
    std::vector<double> input(8192);
    for (auto& sample : input)
    {
        sample = 0.002 * noise.next();
    }

    std::vector<double> first;
    std::vector<double> second;
    first.reserve(input.size());
    second.reserve(input.size());
    for (double sample : input)
    {
        first.push_back(dsp.processSample(sample));
    }

    dsp.reset();
    for (double sample : input)
    {
        second.push_back(dsp.processSample(sample));
    }

    require(first == second, "reset did not restore deterministic denoise state");
}

void testNonFiniteSampleDoesNotPoisonState()
{
    DeNoiseDsp dsp;
    dsp.configure(kSampleRate);
    (void)dsp.processSample(0.001);
    const double nan = std::numeric_limits<double>::quiet_NaN();
    require(std::isnan(dsp.processSample(nan)), "non-finite denoise input should pass through locally");
    require(std::isfinite(dsp.processSample(0.0)), "non-finite denoise input poisoned state");
}

} // namespace

int main()
{
    try
    {
        testQuietBroadbandNoiseIsAttenuated();
        testNormalSignalIsPreserved();
        testGateReopensForSignal();
        testResetIsDeterministic();
        testNonFiniteSampleDoesNotPoisonState();
        std::cout << "DeNoise DSP tests passed\n";
        return EXIT_SUCCESS;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
