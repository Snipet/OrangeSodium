OrangeSodium Codebase Report (excluding third_party)

Critical Build/Compile Issues
- src/signal_buffer.h:24
  Stray backslash at end of `setBufferId` line causes a syntax error. Remove the trailing `\`.

- src/filter.cpp:3
  Opens `namespace OrangeSodium{` but never closes it; file ends at line 9 without a closing brace. Add `}` to close the namespace (and consider `} // namespace OrangeSodium`).

- src/effect.h:11
  Declares `~Effect();` but no definition exists anywhere. Also destructor should be virtual for safe polymorphic deletion. Provide a definition (even empty) in a `.cpp` and mark it `virtual ~Effect() = default;` or define it inline.

- src/simd.h:55,58
  SSE branch macros are malformed (missing closing parentheses): `OS_SIMD_CMPGT` and `OS_SIMD_CMPLE`. Fix the macro definitions. Also, several SSE/FMA intrinsics (`_mm_permute_ps`, `_mm_fmadd_ps`, etc.) are not universally available in plain SSE; guard with appropriate feature macros (SSE3/SSSE3/SSE4.1/FMA) or provide alternatives.

Memory/Ownership Issues
- src/synthesizer.cpp:135–137
  `prepare()` calls `setMasterOutputBufferInfoCallback(n_channels)` which allocates a new `SignalBuffer` and assigns it to `master_output_buffer`, then immediately reassigns `master_output_buffer` with `new SignalBuffer(...)`, leaking the one allocated in the callback. Remove one of the allocations; prefer using only the callback or only the direct allocation.

- src/voice.cpp:11–28
  `Voice` destructor does not delete `voice_master_audio_buffer` allocated in the constructor (src/voice.cpp:8) and via `setMasterAudioBufferInfo`. Add `delete voice_master_audio_buffer; voice_master_audio_buffer = nullptr;`.

- src/orange_sodium.cpp:10
  `Context* context = new Context();` is never freed. `Synthesizer` does not own/delete the context (src/synthesizer.cpp:26). Decide on ownership: either `Synthesizer` should own and delete the `Context`, or `createSynthesizerFromScript` should manage it with a smart pointer and pass ownership.

- src/program.cpp:477,443–447
  On Lua error, `execute()` calls `lua_close(L)` but does not set `L=nullptr`. The destructor later calls `lua_close(L)` again, causing a double-close. After closing, set `L = nullptr` or restructure to avoid closing in `execute()` and instead clear the error without destroying the state.

Audio/MIDI Behavior Issues
- src/synthesizer.cpp:140–149
  `processMidiEvent` ignores note-off; voices can latch forever (no release logic). At least add a way to deactivate the voice on note-off or implement a basic envelope to avoid stuck notes.

- src/voice.cpp:155–161
  When copying from source to master buffer, uses `m_context->n_frames` instead of the `n_audio_frames` provided to `processVoice`. If `n_audio_frames < m_context->n_frames`, this may overread. Use the current block size parameter when copying.

- src/synthesizer.cpp:77–100
  The variant `processBlock(size_t n_audio_frames)` accumulates into `master_output_buffer` without clearing it. This is fine only if always preceded by a clear (as in the `processBlock(float** ...)` path). Document or enforce clearing to avoid subtle accumulation bugs if API usage changes.

Performance/Real‑Time Concerns
- OrangeSodiumTestingPlayground/Source/PluginProcessor.cpp:84
  The plugin calls `synth->prepare(nCh, nSmps)` every audio block. This can reallocate buffers each block, which is not real-time safe. Call `prepare` only when channel count or buffer size changes.

- src/synthesizer.cpp:131–134
  `prepare()` iterates voices and resizes buffers on every call. Ensure the host buffer size or channel count actually changed before doing heavy work, and avoid any heap allocations on the audio thread.

Headers/Includes and Portability
- include/orange_sodium.h:4
  Includes `lua/lua.h` in a public header used by the JUCE plugin. This leaks a third‑party dependency into consumers and can break plugin builds if include paths aren’t configured. Prefer removing the direct Lua include from the public header and keeping it internal to implementation files.

- src/synthesizer.cpp:111 and src/voice.cpp:160
  Use of `std::memcpy` without including `<cstring>` in those translation units may rely on incidental includes. Add `#include <cstring>` where used.

- File name case consistency
  Mixed case references exist (e.g., editor shows `src/Oscillator.h` while the file is `src/oscillator.h`). On case‑sensitive filesystems this will fail. Standardize include paths/cases across the project.

API/Design Observations
- src/signal_buffer.h:29
  `assignExistingBuffer` takes ownership of `data` (deleted in destructor). Document ownership semantics clearly to avoid double‑free if callers pass externally managed pointers.

- src/utilities.h:5 and src/signal_buffer.h:8
  `ObjectID` is defined twice in the same namespace (`typedef` and `using`). While identical re‑declarations are generally tolerated, this duplication can confuse maintenance. Consolidate to a single declaration in one common header.

- src/program.cpp (general)
  The `Program` singleton lacks thread safety. Access from audio and message threads could race. Consider guarding with a mutex or restricting usage to the message thread.

- src/simd.h:3,10–11
  Hard‑defines `OS_AVX`. This will break on non‑AVX builds and some CI hosts. Detect features via compiler flags or CMake checks, and gate intrinsics accordingly.

DSP/Quality Notes (non‑blocking)
- src/oscillators/sine_osc.cpp:15–27
  The oscillator computes phase from time within the block without accumulating across blocks; this causes discontinuities at block boundaries. Maintain a per‑channel phase accumulator for continuous output.

- src/oscillators/sine_osc.cpp:20–26
  Fetches `pitch_buffer[i / pitch_buffer_divisions]` without bounds checks if modulation buffer length/division mismatch occurs. Current setup uses constant buffers sized to `n_frames`, but add guards for robustness.

JUCE Plugin Integration Notes
- OrangeSodiumTestingPlayground/Source/PluginProcessor.cpp:139–167
  `findDefaultScript()` attempts several paths; in packaged builds these may fail. Consider adding a user‑selectable script path or embedding a default script resource.

- OrangeSodiumTestingPlayground/Source/PluginProcessor.cpp:118–136
  MIDI handling forwards only note number; velocity is ignored. Consider using velocity for amplitude.

Suggested Fixes Summary
- Remove stray `\` (src/signal_buffer.h:24), close namespace (src/filter.cpp), and fix SIMD macros (src/simd.h) to restore clean builds.
- Fix ownership: free `voice_master_audio_buffer`, avoid double allocation in `Synthesizer::prepare`, and decide `Context` ownership.
- Make `Effect` destructor virtual and define it; add missing includes for `std::memcpy`.
- Handle note‑off properly; use the current block size when copying buffers; avoid heavy allocations on the audio thread by gating `prepare`.
- Reduce public header dependencies (remove Lua includes from `orange_sodium.h`), and standardize filename casing.

