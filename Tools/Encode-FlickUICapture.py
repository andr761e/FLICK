"""Encode an isolated Capture-FlickUI fixed-time PNG sequence as a MJPEG AVI.

Uses Windows' native JPEG encoder and Python's standard library; no extra packages.
Example: python Tools/Encode-FlickUICapture.py <screenshots-directory> <output.avi> --fps 30
"""
import argparse
import os
import struct
import subprocess
import tempfile
from pathlib import Path


def chunk(tag, data):
    return tag + struct.pack('<I', len(data)) + data + (b'\0' if len(data) & 1 else b'')


def encode(directory, destination, fps):
    files = sorted(Path(directory).glob('*.png'))
    if not files:
        raise ValueError(f'No PNG frames in {directory}')
    frames = []
    width = height = 0
    for file in files:
        header = file.read_bytes()[:24]
        if header[:8] != b'\x89PNG\r\n\x1a\n':
            raise ValueError(f'Not a PNG frame: {file}')
        size = struct.unpack('>II', header[16:24])
        if width and size != (width, height):
            raise ValueError(f'Frame size changed: {file}')
        width, height = size
    # Pass paths as environment values, never interpolated PowerShell code.
    with tempfile.TemporaryDirectory(prefix='flick-video-') as temporary:
        environment = os.environ.copy()
        environment.update(FLICK_VIDEO_FRAME_DIR=str(Path(directory).resolve()), FLICK_VIDEO_JPEG_DIR=temporary)
        script = r'''
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
$codec = [System.Drawing.Imaging.ImageCodecInfo]::GetImageEncoders() | Where-Object MimeType -eq 'image/jpeg'
$options = New-Object System.Drawing.Imaging.EncoderParameters 1
$options.Param[0] = New-Object System.Drawing.Imaging.EncoderParameter ([System.Drawing.Imaging.Encoder]::Quality), ([long]90)
foreach ($file in Get-ChildItem -LiteralPath $env:FLICK_VIDEO_FRAME_DIR -Filter '*.png' -File) {
    $image = [System.Drawing.Image]::FromFile($file.FullName)
    try { $image.Save((Join-Path $env:FLICK_VIDEO_JPEG_DIR ($file.BaseName + '.jpg')), $codec, $options) }
    finally { $image.Dispose() }
}
$options.Dispose()
'''
        # A script block ensures stdin's multiline loop executes as one command.
        command = '& {\n' + script + '\n}\n'
        result = subprocess.run(['powershell', '-NoProfile', '-NonInteractive', '-Command', command],
                                env=environment, capture_output=True, text=True)
        if result.returncode:
            raise RuntimeError(result.stderr)
        frames = [(Path(temporary) / (file.stem + '.jpg')).read_bytes() for file in files]
    maximum = max(map(len, frames))
    avih = struct.pack('<14I', round(1_000_000 / fps), maximum * fps, 0, 0x10,
                       len(frames), 0, 1, maximum, width, height, 0, 0, 0, 0)
    strh = struct.pack('<4s4sIHH8I4h', b'vids', b'MJPG', 0, 0, 0,
                       0, 1, fps, 0, len(frames), maximum, 0xFFFFFFFF, 0,
                       0, 0, width, height)
    strf = struct.pack('<IiiHH4sIiiII', 40, width, height, 1, 24, b'MJPG',
                       width * height * 3, 0, 0, 0, 0)
    hdrl = chunk(b'LIST', b'hdrl' + chunk(b'avih', avih) +
                 chunk(b'LIST', b'strl' + chunk(b'strh', strh) + chunk(b'strf', strf)))
    movie = bytearray(b'movi')
    index = bytearray()
    for frame in frames:
        index += struct.pack('<4sIII', b'00dc', 0x10, len(movie), len(frame))
        movie += chunk(b'00dc', frame)
    payload = b'AVI ' + hdrl + chunk(b'LIST', movie) + chunk(b'idx1', index)
    destination = Path(destination)
    destination.parent.mkdir(parents=True, exist_ok=True)
    destination.write_bytes(chunk(b'RIFF', payload))
    print(f'{destination}: {len(frames)} frames, {width}x{height}, {fps} FPS, {len(frames) / fps:.2f} seconds')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('directory')
    parser.add_argument('destination')
    parser.add_argument('--fps', type=int, default=30, choices=range(1, 61))
    args = parser.parse_args()
    encode(args.directory, args.destination, args.fps)
