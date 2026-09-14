#include "AutoDeclipController.h"

#include "AutoDeclipParams.h"
#include "base/source/fstreamer.h"

namespace Travny::Vst3 {

Steinberg::tresult PLUGIN_API AutoDeclipController::initialize(Steinberg::FUnknown* context)
{
    const auto result = EditController::initialize(context);
    if (result != Steinberg::kResultOk)
    {
        return result;
    }

    parameters.addParameter(
        STR16("Denoise"),
        nullptr,
        1,
        0.0,
        0,
        kDenoiseEnabledId);
    return Steinberg::kResultOk;
}

Steinberg::tresult PLUGIN_API AutoDeclipController::setComponentState(Steinberg::IBStream* state)
{
    if (!state)
    {
        return Steinberg::kResultFalse;
    }
    Steinberg::IBStreamer streamer(state, Steinberg::kLittleEndian);
    Steinberg::int32 savedDenoiseEnabled = 0;
    if (!streamer.readInt32(savedDenoiseEnabled))
    {
        savedDenoiseEnabled = 0;
    }

    if (auto* parameter = parameters.getParameter(kDenoiseEnabledId))
    {
        parameter->setNormalized(
            Travny::Audio::denoiseEnabledToNormalized(savedDenoiseEnabled != 0));
    }
    return Steinberg::kResultOk;
}

} // namespace Travny::Vst3
