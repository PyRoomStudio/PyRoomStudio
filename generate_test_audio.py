"""
Generate test audio files for PyRoomStudio.
Creates sample audio files in the sounds/sources/ directory.
"""

import numpy as np
import wave
import struct
import os

def generate_sine_wave(frequency, duration, sample_rate=44100, amplitude=0.5):
    """Generate a sine wave"""
    t = np.linspace(0, duration, int(sample_rate * duration))
    wave_data = amplitude * np.sin(2 * np.pi * frequency * t)
    return wave_data

def generate_chirp(duration, f_start=100, f_end=1000, sample_rate=44100, amplitude=0.5):
    """Generate a frequency chirp (sweep)"""
    t = np.linspace(0, duration, int(sample_rate * duration))
    # Linear chirp
    frequency = f_start + (f_end - f_start) * t / duration
    phase = 2 * np.pi * np.cumsum(frequency) / sample_rate
    wave_data = amplitude * np.sin(phase)
    return wave_data

def save_wav(filename, data, sample_rate=44100):
    """Save audio data to WAV file"""
    # Normalize to 16-bit range
    data_int = np.int16(data * 32767)

    with wave.open(filename, 'w') as wav_file:
        wav_file.setnchannels(1)  # Mono
        wav_file.setsampwidth(2)  # 16-bit
        wav_file.setframerate(sample_rate)
        wav_file.writeframes(data_int.tobytes())

    print(f"Created: {filename}")

def main():
    """Generate test audio files"""
    output_dir = "sounds/sources"
    os.makedirs(output_dir, exist_ok=True)

    print("Generating test audio files...")

    # 1. Pure tone (440 Hz A note) - 2 seconds
    tone_data = generate_sine_wave(440, 2.0)
    save_wav(os.path.join(output_dir, "tone_440hz.wav"), tone_data)

    # 2. Short click/impulse for impulse response
    impulse_data = np.zeros(44100)
    impulse_data[0:100] = 0.8 * np.hanning(100)  # Short pulse with smooth envelope
    save_wav(os.path.join(output_dir, "impulse.wav"), impulse_data)

    # 3. Chirp sweep (100Hz to 1kHz) - 3 seconds
    chirp_data = generate_chirp(3.0, 100, 1000)
    save_wav(os.path.join(output_dir, "chirp.wav"), chirp_data)

    # 4. White noise - 2 seconds
    noise_data = 0.3 * np.random.randn(44100 * 2)
    save_wav(os.path.join(output_dir, "white_noise.wav"), noise_data)

    print("\nTest audio files created successfully!")
    print(f"Location: {os.path.abspath(output_dir)}")
    print("\nYou can now:")
    print("1. Run the GUI with: python main_gui.py")
    print("2. Open the SOUND tab in the Library Panel")
    print("3. Select one of these test files")
    print("4. Click 'Place Sound' to add it to your scene")

if __name__ == "__main__":
    main()
