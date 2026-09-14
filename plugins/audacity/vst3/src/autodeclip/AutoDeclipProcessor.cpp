#include "AutoDeclipProcessor.h"

#include "AutoDeclipParams.h"
#include "base/source/fstreamer.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/vst/vstspeaker.h"

#include <algorithm>

namespace Travny::Vst3 {

AutoDeclipProcessor::AutoDeclipProcessor()
{
    setControllerClass(kAutoDeclipControllerUid);
}

Steinberg::tresult PLUGIN_API AutoDeclipProcessor::initialize(Steinberg::FUnknown* context)
{
    const auto result = AudioEffect::initialize(context);
    if (result != Steinberg::kResultOk)
    {
        return result;
    }

    addAudioInput(STR16("Input"), Steinberg::Vst::SpeakerArr::kStereo);
    addAudioOutput(STR16("Output"), Steinberg::Vst::SpeakerArr::kStereo);
    return Steinberg::kResultOk;
}

void AutoDeclipProcessor::resetDsp() noexcept
{
    for (auto& channel : declipDsp_)
    {
        channel.reset();
    }
    for (auto& channel : deClickDsp_)
    {
        channel.reset();
    }
    for (auto& channel : deHumDsp_)
    {
        channel.reset();
    }
    for (auto& channel : deNoiseDsp_)
    {
        channel.reset();
    }
}

void AutoDeclipProcessor::setDenoiseEnabled(bool enabled) noexcept
{
    if (denoiseEnabled_ == enabled)
    {
        return;
    }

    denoiseEnabled_ = enabled;
    for (auto& channel : deNoiseDsp_)
    {
        channel.reset();
    }
}

void AutoDeclipProcessor::applyParameterChanges(Steinberg::Vst::IParameterChanges* changes) noexcept
{
    if (!changes)
    {
        return;
    }

    const auto parameterCount = changes->getParameterCount();
    for (Steinberg::int32 index = 0; index < parameterCount; ++index)
    {
        auto* queue = changes->getParameterData(index);
        if (!queue || queue->getParameterId() != kDenoiseEnabledId || queue->getPointCount() <= 0)
        {
            continue;
        }

        Steinberg::int32 sampleOffset = 0;
        Steinberg::Vst::ParamValue value = 0.0;
        if (queue->getPoint(queue->getPointCount() - 1, sampleOffset, value) == Steinberg::kResultTrue)
        {
            setDenoiseEnabled(Travny::Audio::denoiseEnabledFromNormalized(value));
        }
    }
}

Steinberg::tresult PLUGIN_API AutoDeclipProcessor::setState(Steinberg::IBStream* state)
{
    if (!state)
    {
        return Steinberg::kResultFalse;
    }

    Steinberg::IBStreamer streamer(state, Steinberg::kLittleEndian);
    Steinberg::int32 savedDenoiseEnabled = 0;
    if (!streamer.readInt32(savedDenoiseEnabled))
    {
        setDenoiseEnabled(false);
        return Steinberg::kResultOk;
    }
    setDenoiseEnabled(savedDenoiseEnabled != 0);
    return Steinberg::kResultOk;
}

Steinberg::tresult PLUGIN_API AutoDeclipProcessor::getState(Steinberg::IBStream* state)
{
    if (!state)
    {
        return Steinberg::kResultFalse;
    }

    Steinberg::IBStreamer streamer(state, Steinberg::kLittleEndian);
    return streamer.writeInt32(denoiseEnabled_ ? 1 : 0)
        ? Steinberg::kResultOk
        : Steinberg::kResultFalse;
}
Steinberg::tresult PLUGIN_API AutoDeclipProcessor::setActive(Steinberg::TBool state)
{
    resetDsp();
    return AudioEffect::setActive(state);
}

Steinberg::tresult PLUGIN_API AutoDeclipProcessor::setupProcessing(Steinberg::Vst::ProcessSetup& setup)
{
    const auto result = AudioEffect::setupProcessing(setup);
    if (result == Steinberg::kResultOk)
    {
        resetDsp();
        sampleRate_ = setup.sampleRate;
        for (auto& channel : deHumDsp_)
        {
            channel.configure(sampleRate_);
        }
        for (auto& channel : deNoiseDsp_)
        {
            channel.configure(sampleRate_);
        }
    }
    return result;
}

Steinberg::tresult PLUGIN_API AutoDeclipProcessor::setBusArrangements(
    Steinberg::Vst::SpeakerArrangement* inputs,
    Steinberg::int32 numIns,
    Steinberg::Vst::SpeakerArrangement* outputs,
    Steinberg::int32 numOuts)
{
    if (numIns != 1 || numOuts != 1)
    {
        return Steinberg::kResultFalse;
    }

    const auto inputChannels = Steinberg::Vst::SpeakerArr::getChannelCount(inputs[0]);
    const auto outputChannels = Steinberg::Vst::SpeakerArr::getChannelCount(outputs[0]);
    if (inputChannels != outputChannels || (inputChannels != 1 && inputChannels != 2))
    {
        return Steinberg::kResultFalse;
    }

    getAudioInput(0)->setArrangement(inputs[0]);
    getAudioOutput(0)->setArrangement(outputs[0]);
    resetDsp();
    return Steinberg::kResultOk;
}

Steinberg::tresult PLUGIN_API AutoDeclipProcessor::canProcessSampleSize(Steinberg::int32 symbolicSampleSize)
{
    return symbolicSampleSize == Steinberg::Vst::kSample32 || symbolicSampleSize == Steinberg::Vst::kSample64
        ? Steinberg::kResultTrue
        : Steinberg::kResultFalse;
}

Steinberg::uint32 PLUGIN_API AutoDeclipProcessor::getLatencySamples()
{
    return static_cast<Steinberg::uint32>(Travny::Audio::AutoDeclipDsp::kLatencySamples + Travny::Audio::DeClickDsp::kLatencySamples);
}

Steinberg::uint32 PLUGIN_API AutoDeclipProcessor::getTailSamples()
{
    const auto pipelineLatency = static_cast<Steinberg::uint64>(
        Travny::Audio::AutoDeclipDsp::kLatencySamples + Travny::Audio::DeClickDsp::kLatencySamples);
    const auto deHumTail = static_cast<Steinberg::uint64>(Travny::Audio::DeHumDsp::tailSamplesForRate(sampleRate_));
    const auto totalTail = pipelineLatency + deHumTail;
    return totalTail >= Steinberg::Vst::kInfiniteTail
        ? Steinberg::Vst::kInfiniteTail
        : static_cast<Steinberg::uint32>(totalTail);
}

template <typename Sample>
bool AutoDeclipProcessor::processBlock(
    Sample** input,
    Sample** output,
    Steinberg::int32 channels,
    Steinberg::int32 samples) noexcept
{
    bool allSilent = true;
    const auto channelCount = std::min<Steinberg::int32>(channels, static_cast<Steinberg::int32>(declipDsp_.size()));
    for (Steinberg::int32 channel = 0; channel < channelCount; ++channel)
    {
        auto* in = input[channel];
        auto* out = output[channel];
        for (Steinberg::int32 sample = 0; sample < samples; ++sample)
        {
            const auto channelIndex = static_cast<std::size_t>(channel);
            const auto declipped = declipDsp_[channelIndex].processSample(in[sample]);
            const auto deClicked = deClickDsp_[channelIndex].processSample(declipped);
            const auto deHummed = deHumDsp_[channelIndex].processSample(deClicked);
            const auto value = denoiseEnabled_
                ? deNoiseDsp_[channelIndex].processSample(deHummed)
                : deHummed;
            out[sample] = value;
            allSilent = allSilent && value == static_cast<Sample>(0);
        }
    }
    return allSilent;
}

Steinberg::tresult PLUGIN_API AutoDeclipProcessor::process(Steinberg::Vst::ProcessData& data)
{
    applyParameterChanges(data.inputParameterChanges);

    if (data.numInputs == 0 || data.numOutputs == 0 || data.numSamples <= 0)
    {
        return Steinberg::kResultOk;
    }

    const auto channels = data.inputs[0].numChannels;
    if (channels < 1 || channels > 2 || data.outputs[0].numChannels != channels)
    {
        return Steinberg::kResultFalse;
    }

    bool allSilent = false;
    if (data.symbolicSampleSize == Steinberg::Vst::kSample32)
    {
        allSilent = processBlock(data.inputs[0].channelBuffers32, data.outputs[0].channelBuffers32, channels, data.numSamples);
    }
    else if (data.symbolicSampleSize == Steinberg::Vst::kSample64)
    {
        allSilent = processBlock(data.inputs[0].channelBuffers64, data.outputs[0].channelBuffers64, channels, data.numSamples);
    }
    else
    {
        return Steinberg::kResultFalse;
    }

    data.outputs[0].silenceFlags = allSilent
        ? ((Steinberg::uint64{1} << static_cast<Steinberg::uint32>(channels)) - 1)
        : 0;
    return Steinberg::kResultOk;
}

} // namespace Travny::Vst3
