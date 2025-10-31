-- Basic OrangeSodium synthesizer script


print("Basic script loaded successfully!")


os_version = get_osodium_version()
print("OrangeSodium Version: " .. os_version)
sawtooth_waveform = create_sawtooth_waveform()

master_audio_buffer = add_audio_buffer(2)


add_buffer_to_master(master_audio_buffer)

function get_num_voices()
    return 8
end

function build_voice()
    print("Building voice...")
    audio_buf = add_voice_audio_buffer(2)
    vfx_buf= add_voice_audio_buffer(2)
    vfx_chain = add_voice_effect_chain(2, audio_buf, vfx_buf)
    
    print("Added audio buffer with ID: " .. audio_buf)

    amp_env = add_basic_envelope(0.0, 0.1, 1.0, 0.4)
    waveform_osc = add_waveform_osc(2, sawtooth_waveform, 0.0, audio_buf)
    print("Added waveform oscillator with ID: " .. waveform_osc)
    filter_env = add_basic_envelope(0.0, 0.4, 0.4, 0.4)
    filter_fx = add_filter_effect(vfx_chain, "ZDF", 100.0, 0.7)

    add_modulation(amp_env, "output", waveform_osc, "amplitude", 0.4)


    set_voice_rand_detune(0.07)

    add_voice_output(vfx_buf, master_audio_buffer)
end