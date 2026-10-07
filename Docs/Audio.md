# Sound and menu radio

Existing navigation, confirmation, archetype-dependent launch/impact/rim sounds,
eliminations, turn changes, round wins and match wins are preserved. New cues cover
switch activation, divider movement, BOB pocket scoring, your own actionable turn
and incoming team pings. Own-turn eligibility uses the actual selection rules;
spectators and other seats do not receive that private cue. Ping sound runs only
on recipients of the existing team-only feed.

## Mix and controls

Settings > Sound has Master, Physics Effects, Interface and Music sliders.
Interface includes your-turn and team-ping notifications. Music controls both
the menu radio and replay bed, independently of physics effects. Settings save
automatically. Stream Safe > Mute All Music suppresses both music sources; the
old replay-mute config key is retained for backward compatibility.

The home screen and Sound settings show the radio's title, progress and
Previous / Pause / Next controls. Pause preserves the current position and is
remembered as a radio preference. Music fades away when entering gameplay,
including match pause/settings, and resumes when returning to the frontend.
It does not compete with final replays or the winning showcase. After fading
out, the radio releases its procedural voice. Returning to the menu creates a
fresh voice from the cached PCM at the saved song position; it does not rely on
Unreal keeping a silent/paused procedural buffer alive throughout a match.

The starting station contains three original, generated stereo instrumentals:
After Hours (synthwave), Soft Reset (downtempo), Break Point (electronic).
Each has a 24-bar intro/main/breakdown/outro arrangement. These are synthesized
music, not commercial recordings. There are no external downloads or dependencies.

## Implementation and tuning

- `FlickSoundSynthesis`: deterministic 48 kHz mono one-shot cues. Original cases
  are unchanged; each new case has its own envelope and tone.
- `FlickAudioDirector`: server broadcasts compact cue descriptions. Important
  events are reliable; frequent impacts use unreliable multicast. Each client
  synthesizes locally and applies its own mix. Dedicated servers render no PCM.
- `FlickRadioSynthesis`: pure 24 kHz stereo generator. BPM, key, melody and
  arrangement live here; no simulation random state is touched.
- `FlickMenuRadioComponent`: local controller-owned playback, asynchronous
  generation with no UObject captured by workers, lazy per-track PCM cache,
  short transition fades and explicit teardown on travel. At most three tracks
  are cached (about 16 MB of PCM total, plus the active audio queue).

Gameplay cue duration, pitch, volume and spatial attenuation remain easy to tune
in the director. Music has a conservative 45% default volume. No physics values,
colliders, puck stats or timing rules are changed.

## Review

Run `FLICK.Audio.Synthesis`, `FLICK.Audio.RadioSynthesis`,
`FLICK.Audio.RadioContext`, `FLICK.Audio.RadioResume` and `FLICK.Match.TeamPings`. Add
`-FlickExportSoundReview` to export cue WAVs and complete radio tracks to
`Saved/AudioReview`. Automated tests cover deterministic generation, sample
length, stereo width, silent edges, headroom, menu eligibility and track wrapping.
Listening in-game remains the final check for subjective mix and musical taste.
For a real-device radio/UI check, run
`Tools/Capture-FlickUI.ps1 -Screen Home -EnableAudio -CaptureDelaySeconds 15`.
Ordinary captures remain silent. Successful audio startup logs `MENU_RADIO_STARTED`.
