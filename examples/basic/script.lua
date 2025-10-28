-- Basic OrangeSodium synthesizer script


print("Basic script loaded successfully!")

config_master_output_default()

os_version = get_osodium_version()
print("OrangeSodium Version: " .. os_version)
sawtooth_waveform = create_sawtooth_waveform()

function build_voice()
    print("Building voice...")
    audio_buf = add_voice_audio_buffer(2)
    print("Added audio buffer with ID: " .. audio_buf)

    obj_type_buf = get_object_type(audio_buf)
    print("Object type of ID " .. audio_buf .. " is: " .. tostring(obj_type_buf))

    print("Created sawtooth waveform with Resource ID: " .. sawtooth_waveform)

    waveform_osc = add_waveform_osc(2, sawtooth_waveform, 0.0, audio_buf)

    voice_effects_out_buf = add_voice_audio_buffer(2)
    print("Added voice effects output buffer with ID: " .. voice_effects_out_buf)
    configure_voice_effects_io(audio_buf, voice_effects_out_buf) -- Input is audio_buf, output is voice_effects_out_buf
    print("Configured voice effects I/O with input buffer ID: " .. audio_buf .. " and output buffer ID: " .. voice_effects_out_buf)
    

    print("Added waveform oscillator with ID: " .. waveform_osc)

    amp_env = add_basic_envelope(0.0, 0.1, 1.0, 0.4)
    pitch_env = add_basic_envelope(0.0, 0.005, 0.0, 0.005)
    add_modulation(amp_env, "output", waveform_osc, "amplitude", 0.8)

    -- Add a filter effect to the voice
    filter_effect = add_voice_effect_filter("ZDF", 2, 0.0, 0.7)
    print("Added filter effect with ID: " .. filter_effect)

    filter_env = add_basic_envelope(0.2, 0.45, 1.0, 0.4)
    add_modulation(filter_env, "output", filter_effect, "cutoff", 0.90)
    --add_modulation(pitch_env, "output", waveform_osc, "pitch", 48.0)

    -- Set voice detune
    set_voice_rand_detune(0.05)

    set_voice_output(voice_effects_out_buf)
end

