-- Basic OrangeSodium synthesizer script


print("Basic script loaded successfully!")

config_master_output_default()

os_version = get_osodium_version()
print("OrangeSodium Version: " .. os_version)


audio_buf = add_voice_audio_buffer(2)
print("Added audio buffer with ID: " .. audio_buf)

obj_type_buf = get_object_type(audio_buf)
print("Object type of ID " .. audio_buf .. " is: " .. tostring(obj_type_buf))

sine_osc = add_sine_osc(2, audio_buf)
print("Added sine oscillator with ID: " .. sine_osc)

obj_type = get_object_type(sine_osc)
print("Object type of ID " .. sine_osc .. " is: " .. tostring(obj_type))

connected_buf = get_connected_audio_buffer_for_oscillator(sine_osc)
print("Connected audio buffer for oscillator ID " .. sine_osc .. " is: " .. tostring(connected_buf))

set_voice_output(audio_buf)