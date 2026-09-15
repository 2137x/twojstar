#include "AutoDeclipController.h"
#include "AutoDeclipParams.h"
#include "AutoDeclipProcessor.h"
#include "public.sdk/source/common/memorystream.h"

#include <cstdlib>
#include <iostream>

int main()
{
    Travny::Vst3::AutoDeclipProcessor source;
    if (source.initialize(nullptr) != Steinberg::kResultOk) return EXIT_FAILURE;

    Steinberg::MemoryStream enabledState;
    Steinberg::int32 enabled = 1;
    if (enabledState.write(&enabled, sizeof(enabled), nullptr) != Steinberg::kResultOk) return EXIT_FAILURE;
    enabledState.seek(0, Steinberg::IBStream::kIBSeekSet, nullptr);
    if (source.setState(&enabledState) != Steinberg::kResultOk) return EXIT_FAILURE;

    Steinberg::MemoryStream savedState;
    if (source.getState(&savedState) != Steinberg::kResultOk) return EXIT_FAILURE;
    source.terminate();

    savedState.seek(0, Steinberg::IBStream::kIBSeekSet, nullptr);
    Travny::Vst3::AutoDeclipProcessor restored;
    if (restored.initialize(nullptr) != Steinberg::kResultOk || restored.setState(&savedState) != Steinberg::kResultOk) return EXIT_FAILURE;

    Steinberg::MemoryStream restoredState;
    if (restored.getState(&restoredState) != Steinberg::kResultOk) return EXIT_FAILURE;
    restored.terminate();
    restoredState.seek(0, Steinberg::IBStream::kIBSeekSet, nullptr);

    Travny::Vst3::AutoDeclipController controller;
    if (controller.initialize(nullptr) != Steinberg::kResultOk || controller.setComponentState(&restoredState) != Steinberg::kResultOk) return EXIT_FAILURE;
    const auto normalized = controller.getParamNormalized(Travny::Vst3::kDenoiseEnabledId);
    controller.terminate();

    if (normalized < 0.5) {
        std::cerr << "Denoise On did not survive processor state round-trip into controller\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
