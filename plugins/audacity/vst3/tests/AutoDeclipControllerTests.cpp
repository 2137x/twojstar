#include "AutoDeclipController.h"
#include "AutoDeclipParams.h"

#include <cstdlib>
#include <iostream>

int main()
{
    Travny::Vst3::AutoDeclipController controller;
    if (controller.initialize(nullptr) != Steinberg::kResultOk)
    {
        std::cerr << "controller initialization failed\n";
        return EXIT_FAILURE;
    }

    Steinberg::Vst::ParameterInfo info{};
    const auto result = controller.getParameterInfo(0, info);
    const bool visibleAutomatableToggle =
        result == Steinberg::kResultOk &&
        info.id == Travny::Vst3::kDenoiseEnabledId &&
        info.stepCount == 1 &&
        (info.flags & Steinberg::Vst::ParameterInfo::kCanAutomate) != 0;

    controller.terminate();
    if (!visibleAutomatableToggle)
    {
        std::cerr << "Denoise must be an automatable visible VST3 toggle\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
