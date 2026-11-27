

# --- mscc Zero build (added by rpi/mscc-zero/make-zero-deb.sh) ---------------
# The Zero 2 W can't run local operator audio over its one USB line, so the
# operator lists offer only the two dummy PipeWire devices (the system PortAudio
# that pyaudio uses can't see them; the servers find them by name through MSCC's
# own PortAudio). Real sound cards are not listed.
_mscc_list_audio_devices = list_audio_devices


def list_audio_devices(want_input: bool, *, require_96k: bool = False):
    _devices, err = _mscc_list_audio_devices(want_input, require_96k=require_96k)
    name = "OperatorNullMic" if want_input else "OperatorNull"
    return [
        AudioDevice(
            index=-1,
            name=name,
            host_api="PipeWire",
            channels=1 if want_input else 2,
            is_input=want_input,
            hint="dummy: the Zero has no local audio",
        )
    ], err
