# Checkerboard vs. NFT Marker-Based AR System

This repository implements a lightweight Augmented Reality system using OpenCV. It provides a comparative framework between two tracking methods:
1. **Checkerboard Tracking:** Uses standard `cv::findChessboardCorners` and `cv::solvePnP`.
2. **Natural Feature Tracking (NFT):** Uses feature matching (ORB/AKAZE) to track arbitrary planar images.

The system is designed to run automated experiments to evaluate **Pose Stability**, **Detection Robustness** (under occlusion, lighting changes, and extreme angles), and **Computational Performance**.

## Requirements

### C++ Core
- C++17-compatible compiler
- [OpenCV](https://opencv.org/) 4.x (core, highgui, videoio, imgproc, calib3d, features2d)
- CMake 3.16+

On macOS:
```bash
brew install opencv cmake
```

### Python Analysis Tools
To generate the performance graphs, you need Python 3 installed with the following libraries:
```bash
pip install pandas matplotlib seaborn
```

## Build

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

The generated executable will be at `build/lightweight_ar`.

## Usage

### 1. Configuration
The system is currently configured via `main.cpp`. You can toggle between tracking modes and select the experiment type before compiling:

```cpp
// In main.cpp:

// 1. Select Tracking Method
bool useNft = false; // Set true for NFT, false for Checkerboard

// 2. Select Experiment
// Arguments: capture, useNft, patternSize, squareSize, ExperimentCategory, TestName
// Categories: "pose_stability", "detection_robustness"
// TestNames: "static", "angle", "lighting", "occlusion"

augmentLoop(capture, useNft, patternSize, squareSize, "detection_robustness", "occlusion");
```

### 2. Running the AR System
```bash
./build/lightweight_ar
```

- **Calibration:** On the first run, the system will automatically perform camera calibration. Follow the on-screen instructions.
- **NFT Reference:** If using NFT, the system will ask to capture a reference image if one doesn't exist in `data/reference/`.
- **Data Collection:** The system records per-frame statistics (pose jitter, frame time, tracking success) and saves them to JSON files in `data/statistics/`.

### 3. Generating Analysis Plots
Once you have collected data for your experiments (Checkerboard and NFT runs for Angle, Lighting, Occlusion, and Static stability), run the Python script to generate comparative graphs:

```bash
python main.py
```

This will read the JSON logs and generate the following figures in `data/figures/`:
1. **Performance:** Frame time comparison across all tests.
2. **Stability:** Translational and Rotational jitter comparison (NFT vs. Checkerboard).
3. **Robustness:** Gantt charts showing tracking intervals during stress tests.

#### 3.1 Table Generation
You can also generate the table plot, by doing:
```bash
python create_summary_table.py
```

## Data Structure
The system organizes data as follows:
- `data/calibration/`: Camera intrinsics.
- `data/reference/`: Reference image for NFT.
- `data/statistics/`: JSON logs separated by method (Checkerboard/NFT) and experiment type.
- `data/figures/`: Generated analysis plots.

If no calibration folder exists, you'll be prompted to make new calibration pictures, take pictures on space, vary the motives, angle it. 15 is required for checkerboard and a single texture photo, taken from birds-eye-view for NFT. Once done, they will automatically be saved and used for calibrating the AR pipeline. 