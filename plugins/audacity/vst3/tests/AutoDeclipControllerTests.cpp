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

    bool visibleAutomatableToggle = false;
    for (Steinberg::int32 i = 0; i < controller.getParameterCount(); ++i)
    {
        Steinberg::Vst::ParameterInfo info{};
        if (controller.getParameterInfo(i, info) == Steinberg::kResultOk &&
            info.id == Travny::Vst3::kDenoiseEnabledId)
        {
            visibleAutomatableToggle =
                info.stepCount == 1 &&
                (info.flags & Steinberg::Vst::ParameterInfo::kCanAutomate) != 0 &&
                (info.flags & Steinberg::Vst::ParameterInfo::kIsHidden) == 0;
            break;
        }
    }

    controller.terminate();
    if (!visibleAutomatableToggle)
    {
        std::cerr << "Denoise must be an automatable visible VST3 toggle\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
