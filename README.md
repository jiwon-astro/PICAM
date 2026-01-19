# PICAM

Driving software for camera sensors manufactured by PI (Princeton Instruments).

## Requirements

- PICam 5.x (SDK)

### Test.cpp

Testing basic exposure settings:
- Sensor temperature (in °C)
- Analog gain (High / Medium / Low)
- Image Quality (Low Noise / High Capacity)
- Exposure time (in milliseconds)
- Number of frames

The output file is in .txt format and includes exposure information. Supports subsequent exposures with a fixed time interval.

### Dark.cpp

- Used only for dark frames

### Gain.cpp

- Obtains both bias frames (0s exposure) and light frames for each exposure time.


<img width="2165" height="2101" alt="ProgramOperationStructure" src="https://github.com/user-attachments/assets/a91ac067-fd59-465d-aab0-e95b1984e471" />

