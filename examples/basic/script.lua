-- Basic OrangeSodium synthesizer script


print("Basic script loaded successfully!")

os_version = get_osodium_version()
print("OrangeSodium Version: " .. os_version)

sine_osc = add_sine_osc(2)
print("Added sine oscillator with ID: " .. sine_osc)

obj_type = get_object_type(sine_osc)
print("Object type of ID " .. sine_osc .. " is: " .. tostring(obj_type))