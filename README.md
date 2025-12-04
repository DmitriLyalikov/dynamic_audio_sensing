# Dynamic Audio Sensing with Scene Classification on ESP32
## Overview
> This project implements a real-time audio sensing and classification system on the ESP32 platform. It captures ambient audio using a digital MEMS microphone, performs low-power signal analysis (RMS, spectral centroid) and classifies acoustic scenes into categories such as `quiet`, `speech`, `background noise`. 

>> The system dynamically adjusts gain or equalization settings based on classified scene and optionally transmits audio data wireleslly using ESP-NOW to another ESP32, which plays it back through a speaker.

## Goals

* Monitor ambient audio with low computational and power cost

* Extract features: Root Mean Square (RMS), Spectral Centroid

* Classify scenes dynamically in real time

* Adjust digital gain or filtering based on acoustic context
* Optionally transmit processed audio or metadata to another ESP32 device

* Enable future extensions such as EQ, ML classification, or power management

## Hardware Requirements
1. ESP32 DevKit (e.g., ESP32-WROOM-32)

2. INMP441 Digital MEMS microphone (I2S interface)

3. Speaker (optional for playback)

4. Optional: External I2S amplifier (e.g., MAX98357A)

## Development Environment
* ESP-IDF v5.5.1

* ESP-DSP v1.3.2 (installed via idf.py add-dependency)

* Toolchain: xtensa-esp32-elf (v14.2.0 or later)

* Platform: Windows

## File Structure
```
dynamic_audio_sensing/
├── CMakeLists.txt
├── main/
│   ├── CMakeLists.txt
│   └── dynamic_audio_sensing.c          # Main application logic
├── components/
│   ├── mic_input/
│   │   ├── CMakeLists.txt
│   │   ├── mic_input.c                  # I2S microphone interface
│   │   └── mic_input.h
│   ├── dsp/
│   │   ├── CMakeLists.txt
│   │   ├── dsp_features.c               # RMS, spectral centroid, gain control
│   │   └── dsp_features.h
├── managed_components/
│   └── espressif__esp-dsp/              # Auto-managed DSP component (via IDF)
```

## Features & Functionality
### Signal Acquisition
* Uses I2S interface to receive 24-bit audio samples from the INMP441 microphone
* Samples at 16 kHz with 512-sample buffers (~32 ms window)

### Feature Extraction
* RMS Energy: Measures average signal power
* Spectral Centroid: Calculates center of spectral mass using real FFT
* Implemented using the ESP-DSP library for performance

### Scene Classification
* quiet: Low RMS energy
* speech: Mid-level energy and centroid between 300–3500 Hz
* background noise: High energy or wideband centroid
Real-Time Gain Adjustment

### Real-Time Gain Adjustment
* Applies digital gain scaling to mic input before further use or transmission

#### Gain levels:
* Quiet: +3.0x

* Speech: 1.0x

* Noise: 0.5x
Wireless Transmission (Optional)

### Wireless Transmission (Optional)
* Scene metadata or audio samples can be transmitted over:

* ESP-NOW (preferred for low-latency peer-to-peer)

* BLE (suitable for metadata or light streaming)

`On the receiving ESP32, audio can be:`
1. Played back via I2S DAC

2. Adjusted based on transmitted scene classification

### Techniques Used

* ESP-DSP FFT (dsps_fft2r) for spectral analysis

* Windowing (Hanning) for FFT accuracy

* Real-time signal processing on microcontroller without OS delay

* Dynamic parameter control based on audio classification

* Modular ESP-IDF component structure