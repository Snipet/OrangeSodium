-- Basic OrangeSodium synthesizer script

-- Configure synthesizer parameters
sample_rate = 44100
buffer_size = 512


print("Basic script loaded successfully!")

os_version = get_osodium_version()
print("OrangeSodium Version: " .. os_version)
