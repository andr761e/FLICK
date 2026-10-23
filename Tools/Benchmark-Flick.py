"""Repeatable Epic GPU comparisons using isolated profiles and real Unreal views.

Build FLICKEditor first. Example (Python 3, no packages needed):
  python Tools/Benchmark-Flick.py --label Epic-After
  python Tools/Benchmark-Flick.py --label Epic-Control --full-reflections
This is a settled-board rendering benchmark, not a moving-shot/network benchmark.
"""
import argparse
import csv
import json
import os
from pathlib import Path
import re
import statistics
import subprocess


def summarize(path, frame_count, sample_count):
    # Unreal appends new stat columns during capture, then writes the complete
    # header at the end. Earlier rows legitimately have fewer columns.
    csv.field_size_limit(10_000_000)
    with path.open(encoding="utf-8-sig", newline="") as source:
        rows = list(csv.reader(source))
    headers = [i for i, row in enumerate(rows)
               if row and row[0] == "EVENTS" and "FrameTime" in row]
    if len(headers) < 2:
        raise RuntimeError(f"Incomplete Unreal CSV: {path}")
    header = rows[headers[-1]]
    frames = rows[headers[0] + 1:headers[-1]]
    if len(frames) != frame_count:
        raise RuntimeError(f"Expected {frame_count} frames, got {len(frames)}: {path}")
    samples = frames[-sample_count:]
    result = {"frames": len(frames), "measured_frames": len(samples)}
    for name in ("FrameTime", "GPUTime", "GameThreadTime", "RenderThreadTime",
                 "GPU/LumenReflections", "GPU/TemporalSuperResolution"):
        column = header.index(name)
        values = [float(row[column]) for row in samples]
        ordered = sorted(values)
        result[name] = {"mean_ms": statistics.mean(values),
                        "median_ms": statistics.median(values),
                        "p95_ms": ordered[int((len(ordered) - 1) * .95)]}
    result["fps"] = 1000 / result["FrameTime"]["mean_ms"]
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--label", required=True)
    parser.add_argument("--modes", nargs="+", choices=("BOB", "1v1", "2v2", "3v3"),
                        default=["BOB", "1v1", "2v2", "3v3"])
    parser.add_argument("--engine-root", default=os.environ.get(
        "FLICK_UNREAL_ENGINE_ROOT", "C:/Program Files/Epic Games/UE_5.8"))
    parser.add_argument("--full-reflections", action="store_true",
                        help="Control run: override only reflection tracing to the former full grid")
    parser.add_argument("--previous-budget", action="store_true",
                        help="Control run: former full grid AND eight Knockout reconstruction samples")
    parser.add_argument("--console", action="append", default=[],
                        help="Additional numeric console assignment for isolated experiments")
    args = parser.parse_args()
    if not re.fullmatch(r"[A-Za-z0-9_-]+", args.label):
        parser.error("Use letters, numbers, underscores or hyphens for --label")
    for assignment in args.console:
        if not re.fullmatch(r"[A-Za-z0-9_.]+ [-+0-9.]+", assignment):
            parser.error("--console must be a console variable name and numeric value")
    root = Path(__file__).resolve().parent.parent
    editor = Path(args.engine_root) / "Engine/Binaries/Win64/UnrealEditor.exe"
    if not editor.is_file():
        parser.error(f"UnrealEditor.exe not found: {editor}")
    output = root / "Saved/Performance" / args.label
    # Never mix a previous run's CSV/profile with this run.
    output.mkdir(parents=True, exist_ok=False)
    results = {}
    for mode in args.modes:
        profile = output / mode
        profile.mkdir()
        commands = ["t.MaxFPS 0", "r.VSync 0"] + args.console
        if args.full_reflections or args.previous_budget:
            commands += ["r.Lumen.Reflections.DownsampleFactor 1",
                         "r.Lumen.Reflections.DownsampleCheckerboard 0"]
        if args.previous_budget:
            # Old camera quality 1.5 * Epic's five samples rounds to eight.
            # Current quality 1.0 * eight is the identical shader sample count.
            commands += ["r.Lumen.Reflections.ScreenSpaceReconstruction.NumSamples "
                         + ("5" if mode == "BOB" else "8")]
        commands += ["r.ScreenPercentage", "r.Lumen.Reflections.DownsampleFactor",
                     "r.Lumen.Reflections.DownsampleCheckerboard",
                     "r.Lumen.Reflections.ScreenSpaceReconstruction.NumSamples"]
        command = [str(editor), str(root / "FLICK.uproject"), "/Engine/Maps/Entry",
                   "-game", "-windowed", "-RenderOffscreen", "-unattended", "-nosplash",
                   "-ddc=InstalledNoZenLocalFallback", "-nosteam", "-nosound",
                   "-NoScreenMessages", "-ForceRes", "-ResX=1920", "-ResY=1080",
                   "-FlickSkipIntro", "-FixedSeed", "-FlickPerformancePreview",
                   "-FlickCameraView=0", "-FlickTestArenaSeed=1337",
                   "-FlickBobPreview" if mode == "BOB" else "-FlickTestArenaPreview",
                   f"-FlickPlayersPerTeam={1 if mode == 'BOB' else mode[0]}",
                   "-ini:GameUserSettings:[FLICK.GameplaySettings]:GameplayCameraDistance=1.25",
                   "-ini:GameUserSettings:[FLICK.GameplaySettings]:ControlOverviewEnabled=False",
                   "-csvCaptureFrames=1500", "-csvGpuStats", "-ExitAfterCsvProfiling",
                   "-csvCompression=0", "-csvExecCmds=" + ",".join(f"300:{c}" for c in commands),
                   f"-UserDir={profile.as_posix()}/", f"-abslog={(profile / 'run.log').as_posix()}"]
        print(f"Benchmarking {mode}...", flush=True)
        subprocess.run(command, cwd=root, check=True, timeout=240,
                       creationflags=subprocess.CREATE_NO_WINDOW)
        captures = list(profile.rglob("*.csv"))
        if len(captures) != 1:
            raise RuntimeError(f"Expected one CSV, found {len(captures)} in {profile}")
        result = summarize(captures[0], 1500, 600)
        result["csv"] = str(captures[0].relative_to(root))
        log = (profile / "run.log").read_text(encoding="utf-8-sig", errors="replace")
        if any(marker in log for marker in ("Simultaneous kickoff released", "locked kickoff Piece",
                                            " flicked Piece ")):
            raise RuntimeError(f"A shot changed the benchmark scene; discard this run: {profile}")
        result["hardware"] = [line for line in log.splitlines()
                              if "LogCsvProfiler: Display: Metadata set" in line
                              and any(key in line for key in ('gpu="', 'cpu="'))]
        result["output_resolution"] = [1920, 1080]
        result["render_settings"] = [line for line in log.splitlines()
                                     if re.search(r'(r.ScreenPercentage|r.Lumen.Reflections.(Downsample\w*|ScreenSpaceReconstruction.NumSamples)) = "', line)]
        for group in ("ViewDistance", "AntiAliasing", "Shadow", "GlobalIllumination",
                      "Reflection", "PostProcess", "Texture", "Effects", "Shading"):
            if f"[{group}Quality@3]" not in log:
                raise RuntimeError(f"Epic {group} quality not confirmed in {profile / 'run.log'}")
        results[mode] = result
        print(f"{mode}: {result['fps']:.1f} FPS, GPU {result['GPUTime']['mean_ms']:.3f} ms", flush=True)
        (output / "summary.json").write_text(json.dumps(results, indent=2), encoding="utf-8")
    print(f"Results: {output / 'summary.json'}")


if __name__ == "__main__":
    main()
