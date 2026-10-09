# Practice packs

Play > Training > Practice Packs offers Precision, Knockouts, Switch Control and BOB Pockets. Each has Foundation, Skilled and Expert levels with five fixed, one-shot challenges. Passes and misses both advance after a short result display. A completed run saves its best score per category/difficulty in `FLICK.PracticeScores.v1` in GameUserSettings.ini. A 5/5 earns mastery; there are 12 packs to perfect.

Restart Run (or the configured Restart key, R by default) restarts all five challenges. Practice Packs returns to difficulty selection; Back then returns to categories. Leaving an incomplete run does not save its partial score. Free Play undo/editing and bot matches are unchanged, and the original guided Tutorial remains available.

The component uses normal GameMode spawning, selection, launches, Chaos collision, pocket detection and shot resolution. Difficulty changes layouts and objectives, never puck physics. Precision requires the entire puck footprint inside the circle: centre distance plus gameplay radius must not exceed the target radius. Friendly losses invalidate knockout attempts. Pocketing the BOB striker invalidates a pocket challenge. Expert Switch Control requires exactly the marked switch. Markers have no collision, overlap, navigation or shadows.

Code: `FlickPracticeCatalog` owns stable pack IDs and best-score persistence; `UFlickPracticeComponent` owns the current run and challenge setups; `FlickGameLayerPractice.cpp` owns the selection and scoring UI. Component defaults expose zone radii, result-display time and the repeatable switch-layout seed. Layout distances are grouped in `SetupChallenge` for tuning.

Verification: automation `FLICK.Training.PracticePacks` checks scoring, restrictions, persistence, restarts and all sixty setups through actual Chaos launches with a bounded power search. Test scores use a separate INI and never change real mastery. UI capture `Tools/Capture-FlickUI.ps1 -Screen PracticePacks,Practice` with optional `-PracticeCategory 0..3` and `-PracticeDifficulty 0..2`. Category -1 shows the category hub. These preview flags are non-shipping and the capture script isolates the user profile.
