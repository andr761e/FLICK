# FLICK sound palette

All nine existing cues use code-generated 48 kHz mono PCM. The palette combines
short tactile transients, damped resonances and a shared melodic vocabulary.

- Navigate: quiet, short mechanical tick.
- Confirm: two overlapping plucks rising by a fifth.
- Launch: falling low body, sharp release and a brief air layer; power and piece
  archetype control weight and brightness.
- Puck impact: solid low body and a short contact transient, with mass, strength
  and both piece archetypes shaping the result.
- Rim impact: separate metallic resonances with a short decaying tail.
- Ring-out: downward sweep with a soft air tail.
- Turn: restrained two-note signal, with a different root for each team.
- Round win: three rising plucks.
- Match win: a longer four-note resolution with a low final reinforcement.

`FlickSoundSynthesis.cpp` renders PCM without touching gameplay state or its random
streams. Notes have individual envelopes and phase origins; sweeps integrate
frequency continuously. Edge fades and a soft limiter prevent abrupt endpoints
and leave headroom. `FlickAudioDirector` owns event timing, mix levels, archetype
mapping and positional playback. Master, effects and interface settings still apply.

`FLICK.Audio.Synthesis` checks all cues at nine timbre/intensity combinations for
non-silence, peak headroom, silent endpoints and repeatability. Run automation with
`-FlickExportSoundReview` to export the actual renderer output to
`Saved/AudioReview/*.wav`. These are dry, full-intensity review samples before
the director's mix gain, pitch and spatial attenuation.

Waveform checks do not assess perceived loudness or replace listening in a match.

Validation: Unreal 5.6 Development Editor build succeeded; all 23 FLICK tests
passed with zero warnings or failures. Review exports were verified as 48 kHz,
16-bit mono WAVs and copied from the isolated automation user directory into
`Saved/AudioReview`, alongside `index.html` for listening. No listening-based
assessment of the in-game mix was performed.
