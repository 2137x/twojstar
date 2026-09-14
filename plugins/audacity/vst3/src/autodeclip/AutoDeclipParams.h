#pragma once

#include "AutoDeclipParameterState.h"
#include "pluginterfaces/vst/vsttypes.h"

namespace Travny::Vst3 {

enum AutoDeclipParams : Steinberg::Vst::ParamID
{
    kDenoiseEnabledId = 1000,
};

} // namespace Travny::Vst3
