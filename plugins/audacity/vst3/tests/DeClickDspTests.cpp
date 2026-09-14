#include "DeClickDsp.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

using Travny::Audio::DeClickDsp;

namespace {

void require(bool condition, const char* message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

std::vector<double> render(const std::vector<double>& input)
{
    DeClickDsp dsp;
    std::vector<double> output;
    output.reserve(input.size() + DeClickDsp::kLatencySamples);

    for (double sample : input)
    {
        output.push_back(dsp.processSample(sample));
    }
    for (std::size_t i = 0; i < DeClickDsp::kLatencySamples; ++i)
    {
        output.push_back(dsp.processSample(0.0));
    }
    return output;
}

std::vector<double> aligned(const std::vector<double>& rendered, std::size_t originalSize)
{
    return {rendered.begin() + static_cast<std::ptrdiff_t>(DeClickDsp::kLatencySamples),
            rendered.begin() + static_cast<std::ptrdiff_t>(DeClickDsp::kLatencySamples + originalSize)};
}

void testCleanPassThrough()
{
    std::vector<double> input(512);
    for (std::size_t i = 0; i < input.size(); ++i)
    {
        input[i] = 0.55 * std::sin(static_cast<double>(i) * 0.031);
    }

    const auto output = aligned(render(input), input.size());
    for (std::size_t i = 0; i < input.size(); ++i)
    {
        require(std::abs(output[i] - input[i]) < 1e-12, "clean audio changed");
    }
}

void testPositiveImpulseIsRepaired()
{
    std::vector<double> input(220, 0.10);
    input[110] = 0.92;

    const auto output = aligned(render(input), input.size());
    require(std::abs(output[110] - 0.10) < 1e-12, "positive impulse was not repaired");
    require(output[109] == input[109] && output[111] == input[111], "clean neighbors changed");
}

void testNegativeImpulseIsRepaired()
{
    std::vector<double> input(220, 0.18);
    input[110] = -0.82;

    const auto output = aligned(render(input), input.size());
    require(std::abs(output[110] - 0.18) < 1e-12, "negative impulse was not repaired");
}

void testLowAmplitudeVariationIsPreserved()
{
    std::vector<double> input(220, 0.10);
    input[110] = 0.34;

    const auto output = aligned(render(input), input.size());
    require(output[110] == input[110], "small local variation should be preserved");
}

void testStepTransientIsPreserved()
{
    std::vector<double> input(220, 0.0);
    for (std::size_t i = 110; i < input.size(); ++i)
    {
        input[i] = 0.75;
    }

    const auto output = aligned(render(input), input.size());
    require(output[109] == input[109] && output[110] == input[110],
            "real step transient should be preserved");
}

void testMultiSampleTransientIsPreserved()
{
    std::vector<double> input(220, 0.0);
    input[108] = 0.10;
    input[109] = 0.55;
    input[110] = 0.95;
    input[111] = 0.55;
    input[112] = 0.10;

    const auto output = aligned(render(input), input.size());
    require(output[110] == input[110], "multi-sample transient peak should be preserved");
}

void testNonFiniteContextDoesNotSpread()
{
    std::vector<double> input(220, 0.10);
    input[109] = std::numeric_limits<double>::quiet_NaN();
    input[110] = 0.95;
    const auto output = aligned(render(input), input.size());
    require(output[110] == input[110], "non-finite context must disable click repair");
    require(std::isnan(output[109]), "non-finite sample should remain local");
}

void testResetStartsFreshStream()
{
    DeClickDsp dsp;
    (void)dsp.processSample(0.1);
    (void)dsp.processSample(0.9);
    dsp.reset();

    require(dsp.processSample(0.2) == 0.0, "reset did not restore initial latency state");
    require(dsp.processSample(0.2) == 0.0, "reset retained buffered samples");
    require(std::abs(dsp.processSample(0.2) - 0.2) < 1e-12, "fresh stream output is wrong");
}

} // namespace

int main()
{
    try
    {
        testCleanPassThrough();
        testPositiveImpulseIsRepaired();
        testNegativeImpulseIsRepaired();
        testLowAmplitudeVariationIsPreserved();
        testStepTransientIsPreserved();
        testMultiSampleTransientIsPreserved();
        testNonFiniteContextDoesNotSpread();
        testResetStartsFreshStream();
        std::cout << "DeClick DSP tests passed\n";
        return EXIT_SUCCESS;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
