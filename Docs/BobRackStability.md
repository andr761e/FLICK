# BOB soft-break rack stability - 8 November 2026

A 0.28-power opening break reproduced the trailer's continually rocking central
puck: the live-world rack review measured 41 rocking reversals after the initial
collision. The existing 0.82-power review passed and therefore missed this case.
Disabling the self-righting assist for the same soft break produced zero reversals,
isolating the assist rather than the floor collision as the source of energy.

`UpdateBobPieceStability` now checks conservative rotational envelopes for both
coins, using their published actor poses consistently with the tilt check. The
previous narrow-phase query could become clear between contacts and re-enable
the assist while the coin was still constrained by its neighbours. A tunable
`BobSelfRightingNeighbourClearance` (6 cm) reserves a small gap around the two
envelopes. Sleeping pieces remain asleep, and isolated tilted pieces retain the
existing assist. No forced flattening, teleport, freeze, friction change, mass
change or contact-physics change was introduced.

Validation from the rebuilt editor:

- Original 0.28 soft break: failure, 41 reversals.
- Same soft break with assist disabled: success, 0 reversals (diagnosis only).
- Corrected 0.28 soft break with normal assist enabled: success, 1 brief reversal.
- Corrected 0.82 strong break: success, 0 reversals.
- `FLICK.BOB.ResolutionReliability`, `FLICK.BOB.BotTurnIntegration`, and
  `FLICK.Physics.RenderRateIndependence`: all passed, zero test failures/warnings.
- Fresh fixed-step 30 fps rendered footage reviewed through the settled rack.

Evidence: `Saved/TrailerUpgrade20261108`, including the deliberately failing
`bob-soft-break.log`, successful `fixed-soft-break/review.log`,
`fixed-strong-break/review.log`, and `Regression/Report/index.json`.

The existing opt-in live-world test accepts `-FlickBobReviewPower=0.28` (default
0.82). `-FlickBobReviewNoAssist` is a test-only diagnostic control. Launch with
`-game -FlickBobPreview -FlickBobRackReview`, an isolated UserDir, and
`-ExecCmds="Automation RunTests FLICK.BOB.RackMotionReview"`.

For non-Shipping capture runs, `-FlickNetworkAutoShotPower=0.82` overrides the
existing automatic input helper's default 0.28. It retains normal selection and
launch validation. Shipping behaviour and human input are unchanged.

The editor target compiled successfully. A standalone/package rebuild was not
performed for this trailer task.
