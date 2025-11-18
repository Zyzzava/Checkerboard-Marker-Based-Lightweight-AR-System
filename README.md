# Checkerboard / Marker-Based Lightweight AR Starter

This repository proves out a minimal C++ program that opens your webcam via OpenCV and forms the baseline for overlaying virtual objects on printed markers or checkerboards. It keeps the pipeline small so you can rapidly experiment with pose estimation and rendering layers (e.g., OpenGL or a UI overlay).

## Requirements

- C++17-compatible compiler
- [OpenCV](https://opencv.org/) 4.x (core, highgui, videoio, imgproc modules) installed system-wide
- CMake 3.16+ and a build directory

On macOS you can install the dependency with Homebrew:

```bash
brew install opencv cmake
```

## Build

```bash
cmake -S . -B build
cmake --build build
```

The generated executable will be at `build/lightweight_ar`.

## Run

Make sure your camera is free and run:

```bash
./build/lightweight_ar
```

Press `ESC` or `q` to stop the capture. From here you can extend the program by detecting checkerboard corners with `cv::findChessboardCorners`, solving the extrinsic pose with `cv::solvePnP`, and passing the resulting rotation/translation into OpenGL or another renderer for a virtual cube or axis overlay. The helper module (`capture_helpers.cpp`/`.hpp`) keeps initialization and the capture loop factored out so `main()` only coordinates the high-level workflow.

On phones that default to a zoomed-in video mode, `ar::initializeCapture` now requests a 1280×720 stream before showing frames, and it prints the actual size so you can tell if the sensor fell back to a narrower resolution. Modify the width/height arguments in `capture_helpers.hpp` if you need a different field of view or if your device only supports certain modes.

## Next steps

- Measure latency by timestamping frame capture, pose solve, and render submission.
- Track pose jitter by logging frame-to-frame deltas on rotation and translation.
- Evaluate under harsh lighting and oblique viewing angles while tuning detector parameters and smoothing filters.
