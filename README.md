# PICAM
Driving software for camera sensors manufactured by PI(Princeton Instruments).
Requirements - PICam 5.x (SDK)

1. Test.cpp
Testing basic exposure settings.
  - Sensor temperature (in degC)
  - Analog gain (High / Medium / Low)
  - Image Quality (Low Noise / High Capacity)
  - Exposure time (in milliseconds)
  - Number of frames

Output file in .txt format, includes exposure information.

Supporting subsequential exposures with fixed time interval.
2. Dark.cpp 
  - only for dark frames
3. Gain.cpp
  - getting both bias frames (0s exposure) and light frames for each exposure times.





