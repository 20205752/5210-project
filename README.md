# DV-STAR reproduction

**Description**: This is the reproduction of DV-STAR

## Environment Requirements

This project is developed and tested on Windows system. Required tools:
- CMake version 3.25 or higher
- C++ compiler supporting C++17 standard
- Visual Studio or MinGW with C++ support

## Build Instructions

1. Open terminal in the project root directory
2. Run CMake build commands:
   ```
   mkdir build
   cd build
   cmake ..
   cmake --build .
   ```
3. The executable `DynamicStrClu_ours.exe` will be generated in the build directory

## Running Instructions

Run the program with the following command format:
```
DynamicStrClu_ours.exe -graph [graph file path] -update [update file path] -rho [rho value]
```

Example:
```
DynamicStrClu_ours.exe -graph ../datasets/graphs/soc -update ../datasets/updates/d_r/soc -rho 0.5
```

## File Structure

- main.cpp: Main entry point that parses command line arguments and executes the dynamic graph clustering algorithm
- CMakeLists.txt: CMake configuration file for building the project
- dt/: Directory containing DT data structures for managing edge similarity updates
  - DTBucket.cpp/h: Bucket structure implementation for organizing DT instances
  - DTInstance.cpp/h: DT instance class for tracking edge update tolerance
  - DTManager.cpp/h: Manager class for all DT instances
- MyLib/: Utility library directory
- Tessil_robin_map/: Third-party hash map library for efficient key-value storage
- evalution.py: Python script for evaluating algorithm results
- VD-STAR_tr.pdf: Original paper describing the VD-STAR algorithm

## Auxiliary Functions

The `evalution.py` script evaluates clustering results by comparing ground truth and predicted cluster files. Run it with:
```
python evalution.py
```
Modify the file paths in the script to point to your ground truth and prediction result files. The script outputs ARI and MLR metrics to measure clustering quality.

