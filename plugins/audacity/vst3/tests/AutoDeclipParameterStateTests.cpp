#include "AutoDeclipParameterState.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>

using Travny::Audio::denoiseEnabledFromNormalized;

namespace {
void require(bool condition, const char* message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

void testDenoiseDefaultsToOffThreshold()
{
    require(!denoiseEnabledFromNormalized(0.0), "zero should keep denoise off");
    require(!denoiseEnabledFromNormalized(0.49), "values below midpoint should keep denoise off");
}

void testDenoiseTurnsOnAtMidpoint()
{
    require(denoiseEnabledFromNormalized(0.5), "midpoint should enable denoise");
    require(denoiseEnabledFromNormalized(1.0), "one should enable denoise");
}

} // namespace

int main()
{
    try
    {
        testDenoiseDefaultsToOffThreshold();
        testDenoiseTurnsOnAtMidpoint();
        std::cout << "Auto Declip parameter-state tests passed\n";
        return EXIT_SUCCESS;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
