"""
Acoustic Simulation Test Suite for PyRoomStudio.

This script tests the acoustic simulation backend across multiple STL models,
source/listener placements, and model scales. It generates Matplotlib visualizations
and performs audio quality checks to validate simulation correctness.

Usage:
    cd pyroomstudio
    python -m tests.test_acoustic_simulation

Or run directly:
    python tests/test_acoustic_simulation.py
"""

import os
import sys
from pathlib import Path

# Add parent directory to path for imports
script_dir = Path(__file__).parent
pyroomstudio_dir = script_dir.parent
sys.path.insert(0, str(pyroomstudio_dir))

import numpy as np
from stl import mesh
from scipy.io import wavfile
from datetime import datetime
from dataclasses import dataclass, field
from typing import List, Tuple, Optional, Dict
import pyroomacoustics as pra
import matplotlib.pyplot as plt
import matplotlib
matplotlib.use('Agg')  # Non-interactive backend for saving plots

from scene_manager import SceneManager, SoundSource, Listener
from acoustic_simulator import AcousticSimulator


# =============================================================================
# Configuration
# =============================================================================

# Models to test (relative to resources/ directory)
TEST_MODELS = [
    "Pyramid_square.stl",
    "Prism_triangle.stl",
    "Dodecahedron.stl",
]

# Audio files to test (relative to sounds/sources/ directory)
TEST_AUDIO_FILES = [
    "impulse.wav",
    "frequency_sweep.wav",
    "pink_noise.wav",
    "sine_440hz.wav",
]

# Scale factors to test (representing different room sizes)
TEST_SCALES = [1.0, 2.0, 5.0]

# Simulation parameters
MAX_ORDER = 3
N_RAYS = 10000
ENERGY_ABSORPTION = 0.2
SCATTERING = 0.1

# Audio quality thresholds
CLIPPING_THRESHOLD = 0.99  # Peak amplitude above this is considered clipping
SILENCE_THRESHOLD = 0.001  # RMS below this is considered silence
MIN_AMPLITUDE = 0.01  # Minimum peak amplitude expected


# =============================================================================
# Data Classes
# =============================================================================

@dataclass
class PlacementStrategy:
    """Defines a source/listener placement strategy"""
    name: str
    description: str
    # Functions that take (center, dimensions) and return position
    source_offset: np.ndarray  # Offset from center as fraction of dimensions
    listener_offset: np.ndarray


@dataclass
class AudioQualityResult:
    """Results of audio quality checks"""
    peak_amplitude: float
    rms_amplitude: float
    is_clipping: bool
    is_silent: bool
    has_nan: bool
    has_inf: bool
    zero_crossing_rate: float
    duration_seconds: float
    sample_rate: int
    
    @property
    def is_valid(self) -> bool:
        """Check if audio passes all quality checks"""
        return (
            not self.is_clipping and
            not self.is_silent and
            not self.has_nan and
            not self.has_inf and
            self.peak_amplitude >= MIN_AMPLITUDE
        )


@dataclass
class TestResult:
    """Results of a single test case"""
    model_name: str
    audio_file: str
    scale: float
    placement_name: str
    success: bool
    error_message: Optional[str] = None
    output_path: Optional[str] = None
    audio_quality: Optional[AudioQualityResult] = None
    waveform_plot: Optional[str] = None
    spectrogram_plot: Optional[str] = None


# =============================================================================
# Placement Strategies
# =============================================================================

PLACEMENT_STRATEGIES = [
    PlacementStrategy(
        name="center",
        description="Source and listener both near center",
        source_offset=np.array([0.0, 0.0, 0.0]),
        listener_offset=np.array([0.2, 0.2, 0.0]),
    ),
    PlacementStrategy(
        name="corner",
        description="Source in corner, listener opposite",
        source_offset=np.array([-0.3, -0.3, -0.3]),
        listener_offset=np.array([0.3, 0.3, 0.3]),
    ),
    PlacementStrategy(
        name="near_wall",
        description="Source near wall, listener in center",
        source_offset=np.array([0.4, 0.0, 0.0]),
        listener_offset=np.array([0.0, 0.0, 0.0]),
    ),
]


# =============================================================================
# Model Loading and Geometry
# =============================================================================

def load_stl_model(filepath: str) -> mesh.Mesh:
    """Load an STL model from file"""
    return mesh.Mesh.from_file(filepath)


def compute_model_bounds(model: mesh.Mesh) -> Tuple[np.ndarray, np.ndarray, np.ndarray]:
    """
    Compute the bounding box of a model.
    
    Returns:
        Tuple of (center, dimensions, min_corner)
    """
    vertices = model.vectors.reshape(-1, 3)
    min_corner = np.min(vertices, axis=0)
    max_corner = np.max(vertices, axis=0)
    center = (min_corner + max_corner) / 2
    dimensions = max_corner - min_corner
    return center, dimensions, min_corner


def build_walls_from_model(model: mesh.Mesh, scale_factor: float = 1.0) -> Tuple[List[dict], np.ndarray]:
    """
    Build wall information from STL model triangles.
    
    Args:
        model: The STL mesh
        scale_factor: Scale to apply to vertices
        
    Returns:
        Tuple of (walls_list, scaled_vertices)
    """
    # Get vertices and scale them
    vertices = model.vectors.reshape(-1, 3) * scale_factor
    
    # Create wall info - each triangle is a wall
    n_triangles = len(model.vectors)
    walls = []
    for i in range(n_triangles):
        walls.append({'triangles': [i]})
    
    return walls, vertices


def compute_safe_position(center: np.ndarray, dimensions: np.ndarray, 
                          offset_fraction: np.ndarray, margin: float = 0.1) -> np.ndarray:
    """
    Compute a position inside the model bounds with safety margin.
    
    Args:
        center: Center of the model
        dimensions: Dimensions of the model (width, height, depth)
        offset_fraction: Offset from center as fraction of half-dimensions
        margin: Minimum distance from walls as fraction of dimensions
        
    Returns:
        Safe position inside the model
    """
    # Calculate offset in world units
    half_dims = dimensions / 2
    offset = offset_fraction * half_dims * (1 - margin)
    
    return center + offset


# =============================================================================
# Audio Quality Analysis
# =============================================================================

def analyze_audio_quality(signal: np.ndarray, sample_rate: int) -> AudioQualityResult:
    """
    Analyze audio signal for quality issues.
    
    Args:
        signal: Audio signal (normalized to [-1, 1])
        sample_rate: Sample rate in Hz
        
    Returns:
        AudioQualityResult with all metrics
    """
    # Normalize if needed
    if signal.dtype == np.int16:
        signal = signal.astype(np.float32) / 32768.0
    elif signal.dtype == np.int32:
        signal = signal.astype(np.float32) / 2147483648.0
    
    # Basic metrics
    peak_amplitude = np.max(np.abs(signal))
    rms_amplitude = np.sqrt(np.mean(signal ** 2))
    
    # Quality checks
    is_clipping = peak_amplitude > CLIPPING_THRESHOLD
    is_silent = rms_amplitude < SILENCE_THRESHOLD
    has_nan = np.any(np.isnan(signal))
    has_inf = np.any(np.isinf(signal))
    
    # Zero crossing rate (indicator of frequency content)
    zero_crossings = np.sum(np.abs(np.diff(np.sign(signal))) > 0)
    zero_crossing_rate = zero_crossings / len(signal)
    
    duration_seconds = len(signal) / sample_rate
    
    return AudioQualityResult(
        peak_amplitude=peak_amplitude,
        rms_amplitude=rms_amplitude,
        is_clipping=is_clipping,
        is_silent=is_silent,
        has_nan=has_nan,
        has_inf=has_inf,
        zero_crossing_rate=zero_crossing_rate,
        duration_seconds=duration_seconds,
        sample_rate=sample_rate,
    )


# =============================================================================
# Visualization Functions
# =============================================================================

def plot_waveform(signal: np.ndarray, sample_rate: int, title: str, 
                  output_path: str) -> str:
    """
    Plot and save waveform visualization.
    
    Args:
        signal: Audio signal
        sample_rate: Sample rate in Hz
        title: Plot title
        output_path: Path to save the plot
        
    Returns:
        Path to saved plot
    """
    # Normalize if needed
    if signal.dtype == np.int16:
        signal = signal.astype(np.float32) / 32768.0
    
    time = np.arange(len(signal)) / sample_rate
    
    fig, ax = plt.subplots(figsize=(12, 4))
    ax.plot(time, signal, linewidth=0.5, color='#2E86AB')
    ax.set_xlabel('Time (seconds)', fontsize=10)
    ax.set_ylabel('Amplitude', fontsize=10)
    ax.set_title(title, fontsize=12, fontweight='bold')
    ax.set_ylim(-1.1, 1.1)
    ax.grid(True, alpha=0.3)
    ax.axhline(y=0, color='black', linewidth=0.5)
    
    # Add amplitude markers
    peak = np.max(np.abs(signal))
    ax.axhline(y=peak, color='#E94F37', linestyle='--', linewidth=1, alpha=0.7, label=f'Peak: {peak:.3f}')
    ax.axhline(y=-peak, color='#E94F37', linestyle='--', linewidth=1, alpha=0.7)
    ax.legend(loc='upper right')
    
    plt.tight_layout()
    plt.savefig(output_path, dpi=150, bbox_inches='tight')
    plt.close(fig)
    
    return output_path


def plot_spectrogram(signal: np.ndarray, sample_rate: int, title: str,
                     output_path: str) -> str:
    """
    Plot and save spectrogram visualization.
    
    Args:
        signal: Audio signal
        sample_rate: Sample rate in Hz
        title: Plot title
        output_path: Path to save the plot
        
    Returns:
        Path to saved plot
    """
    # Normalize if needed
    if signal.dtype == np.int16:
        signal = signal.astype(np.float32) / 32768.0
    
    fig, ax = plt.subplots(figsize=(12, 6))
    
    # Compute spectrogram
    nperseg = min(1024, len(signal) // 4)
    if nperseg < 64:
        nperseg = 64
    
    spectrum, freqs, times, im = ax.specgram(
        signal, 
        Fs=sample_rate, 
        NFFT=nperseg,
        noverlap=nperseg // 2,
        cmap='viridis',
        scale='dB'
    )
    
    ax.set_xlabel('Time (seconds)', fontsize=10)
    ax.set_ylabel('Frequency (Hz)', fontsize=10)
    ax.set_title(title, fontsize=12, fontweight='bold')
    ax.set_ylim(0, min(sample_rate / 2, 20000))  # Limit to audible range
    
    cbar = plt.colorbar(im, ax=ax)
    cbar.set_label('Power (dB)', fontsize=10)
    
    plt.tight_layout()
    plt.savefig(output_path, dpi=150, bbox_inches='tight')
    plt.close(fig)
    
    return output_path


def plot_comparison(results: List[TestResult], output_path: str):
    """
    Create a comparison plot of all test results.
    
    Args:
        results: List of test results
        output_path: Path to save the plot
    """
    # Filter successful results with audio quality data
    valid_results = [r for r in results if r.success and r.audio_quality]
    
    if not valid_results:
        print("No valid results to plot comparison")
        return
    
    # Group by model
    models = list(set(r.model_name for r in valid_results))
    
    fig, axes = plt.subplots(2, 2, figsize=(14, 10))
    
    # Plot 1: Peak amplitude by model and scale
    ax1 = axes[0, 0]
    for model in models:
        model_results = [r for r in valid_results if r.model_name == model]
        scales = [r.scale for r in model_results]
        peaks = [r.audio_quality.peak_amplitude for r in model_results]
        ax1.scatter(scales, peaks, label=model, s=50, alpha=0.7)
    ax1.set_xlabel('Scale Factor')
    ax1.set_ylabel('Peak Amplitude')
    ax1.set_title('Peak Amplitude vs Scale')
    ax1.legend()
    ax1.grid(True, alpha=0.3)
    
    # Plot 2: RMS amplitude by model
    ax2 = axes[0, 1]
    model_rms = {}
    for model in models:
        model_results = [r for r in valid_results if r.model_name == model]
        model_rms[model] = [r.audio_quality.rms_amplitude for r in model_results]
    ax2.boxplot(model_rms.values(), labels=model_rms.keys())
    ax2.set_ylabel('RMS Amplitude')
    ax2.set_title('RMS Distribution by Model')
    ax2.tick_params(axis='x', rotation=45)
    ax2.grid(True, alpha=0.3, axis='y')
    
    # Plot 3: Pass/Fail by category
    ax3 = axes[1, 0]
    categories = ['Valid', 'Clipping', 'Silent', 'NaN/Inf']
    counts = [
        sum(1 for r in valid_results if r.audio_quality.is_valid),
        sum(1 for r in valid_results if r.audio_quality.is_clipping),
        sum(1 for r in valid_results if r.audio_quality.is_silent),
        sum(1 for r in valid_results if r.audio_quality.has_nan or r.audio_quality.has_inf),
    ]
    colors = ['#2ECC71', '#E74C3C', '#F39C12', '#9B59B6']
    bars = ax3.bar(categories, counts, color=colors)
    ax3.set_ylabel('Count')
    ax3.set_title('Audio Quality Summary')
    for bar, count in zip(bars, counts):
        ax3.text(bar.get_x() + bar.get_width()/2, bar.get_height() + 0.5, 
                str(count), ha='center', va='bottom', fontweight='bold')
    
    # Plot 4: Duration distribution
    ax4 = axes[1, 1]
    durations = [r.audio_quality.duration_seconds for r in valid_results]
    ax4.hist(durations, bins=20, color='#3498DB', edgecolor='black', alpha=0.7)
    ax4.set_xlabel('Duration (seconds)')
    ax4.set_ylabel('Count')
    ax4.set_title('Output Duration Distribution')
    ax4.grid(True, alpha=0.3, axis='y')
    
    plt.tight_layout()
    plt.savefig(output_path, dpi=150, bbox_inches='tight')
    plt.close(fig)
    print(f"Saved comparison plot: {output_path}")


# =============================================================================
# Report Generation
# =============================================================================

def generate_report(results: List[TestResult], output_dir: str) -> str:
    """
    Generate a text report of all test results.
    
    Args:
        results: List of test results
        output_dir: Directory to save the report
        
    Returns:
        Path to the report file
    """
    report_path = os.path.join(output_dir, "report.txt")
    
    total = len(results)
    passed = sum(1 for r in results if r.success and r.audio_quality and r.audio_quality.is_valid)
    failed = total - passed
    
    with open(report_path, 'w') as f:
        f.write("=" * 80 + "\n")
        f.write("PYROOMSTUDIO ACOUSTIC SIMULATION TEST REPORT\n")
        f.write("=" * 80 + "\n\n")
        
        f.write(f"Generated: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n")
        f.write(f"Total Tests: {total}\n")
        f.write(f"Passed: {passed}\n")
        f.write(f"Failed: {failed}\n")
        f.write(f"Pass Rate: {passed/total*100:.1f}%\n\n")
        
        f.write("-" * 80 + "\n")
        f.write("CONFIGURATION\n")
        f.write("-" * 80 + "\n")
        f.write(f"Models: {', '.join(TEST_MODELS)}\n")
        f.write(f"Audio Files: {', '.join(TEST_AUDIO_FILES)}\n")
        f.write(f"Scales: {', '.join(str(s) for s in TEST_SCALES)}\n")
        f.write(f"Placements: {', '.join(p.name for p in PLACEMENT_STRATEGIES)}\n")
        f.write(f"Max Order: {MAX_ORDER}\n")
        f.write(f"N Rays: {N_RAYS}\n")
        f.write(f"Energy Absorption: {ENERGY_ABSORPTION}\n")
        f.write(f"Scattering: {SCATTERING}\n\n")
        
        f.write("-" * 80 + "\n")
        f.write("DETAILED RESULTS\n")
        f.write("-" * 80 + "\n\n")
        
        for i, result in enumerate(results, 1):
            status = "PASS" if (result.success and result.audio_quality and result.audio_quality.is_valid) else "FAIL"
            f.write(f"Test {i}: [{status}]\n")
            f.write(f"  Model: {result.model_name}\n")
            f.write(f"  Audio: {result.audio_file}\n")
            f.write(f"  Scale: {result.scale}x\n")
            f.write(f"  Placement: {result.placement_name}\n")
            
            if result.error_message:
                f.write(f"  Error: {result.error_message}\n")
            
            if result.audio_quality:
                aq = result.audio_quality
                f.write(f"  Audio Quality:\n")
                f.write(f"    Peak Amplitude: {aq.peak_amplitude:.4f}\n")
                f.write(f"    RMS Amplitude: {aq.rms_amplitude:.4f}\n")
                f.write(f"    Duration: {aq.duration_seconds:.2f}s\n")
                f.write(f"    Clipping: {'YES' if aq.is_clipping else 'NO'}\n")
                f.write(f"    Silent: {'YES' if aq.is_silent else 'NO'}\n")
                f.write(f"    Has NaN: {'YES' if aq.has_nan else 'NO'}\n")
                f.write(f"    Has Inf: {'YES' if aq.has_inf else 'NO'}\n")
            
            if result.output_path:
                f.write(f"  Output: {result.output_path}\n")
            if result.waveform_plot:
                f.write(f"  Waveform: {result.waveform_plot}\n")
            if result.spectrogram_plot:
                f.write(f"  Spectrogram: {result.spectrogram_plot}\n")
            
            f.write("\n")
        
        f.write("=" * 80 + "\n")
        f.write("END OF REPORT\n")
        f.write("=" * 80 + "\n")
    
    return report_path


# =============================================================================
# Main Test Runner
# =============================================================================

def run_single_test(
    model_path: str,
    audio_path: str,
    scale: float,
    placement: PlacementStrategy,
    output_dir: str,
    test_index: int,
) -> TestResult:
    """
    Run a single acoustic simulation test.
    
    Args:
        model_path: Path to STL model
        audio_path: Path to source audio file
        scale: Scale factor for the model
        placement: Placement strategy to use
        output_dir: Directory for output files
        test_index: Index for naming output files
        
    Returns:
        TestResult with all information
    """
    model_name = os.path.basename(model_path)
    audio_name = os.path.basename(audio_path)
    
    print(f"\n{'='*60}")
    print(f"Test {test_index}: {model_name} | {audio_name} | {scale}x | {placement.name}")
    print(f"{'='*60}")
    
    try:
        # Load model
        print("Loading model...")
        model = load_stl_model(model_path)
        
        # Compute bounds
        center, dimensions, min_corner = compute_model_bounds(model)
        print(f"Model center: {center}")
        print(f"Model dimensions: {dimensions}")
        
        # Apply scale
        scaled_center = center * scale
        scaled_dimensions = dimensions * scale
        
        # Compute source and listener positions
        source_pos = compute_safe_position(scaled_center, scaled_dimensions, placement.source_offset)
        listener_pos = compute_safe_position(scaled_center, scaled_dimensions, placement.listener_offset)
        
        print(f"Source position: {source_pos}")
        print(f"Listener position: {listener_pos}")
        
        # Build walls
        walls, scaled_vertices = build_walls_from_model(model, scale)
        print(f"Created {len(walls)} walls")
        
        # Create scene manager
        scene_manager = SceneManager()
        scene_manager.add_sound_source(source_pos, audio_path, volume=1.0, name="TestSource")
        scene_manager.add_listener(listener_pos, name="TestListener")
        
        # Create simulator
        simulator = AcousticSimulator()
        
        # Run simulation
        print("Running simulation...")
        result_dir = simulator.simulate_scene(
            scene_manager=scene_manager,
            walls_from_render=walls,
            room_center=scaled_center,
            model_vertices=scaled_vertices,
            max_order=MAX_ORDER,
            n_rays=N_RAYS,
            energy_absorption=ENERGY_ABSORPTION,
            scattering=SCATTERING,
        )
        
        if result_dir is None:
            return TestResult(
                model_name=model_name,
                audio_file=audio_name,
                scale=scale,
                placement_name=placement.name,
                success=False,
                error_message="Simulation returned None",
            )
        
        # Find output file
        output_files = [f for f in os.listdir(result_dir) if f.endswith('.wav')]
        if not output_files:
            return TestResult(
                model_name=model_name,
                audio_file=audio_name,
                scale=scale,
                placement_name=placement.name,
                success=False,
                error_message="No output WAV files found",
            )
        
        output_wav = os.path.join(result_dir, output_files[0])
        
        # Load and analyze output
        print("Analyzing output audio...")
        sample_rate, signal = wavfile.read(output_wav)
        audio_quality = analyze_audio_quality(signal, sample_rate)
        
        print(f"Peak amplitude: {audio_quality.peak_amplitude:.4f}")
        print(f"RMS amplitude: {audio_quality.rms_amplitude:.4f}")
        print(f"Duration: {audio_quality.duration_seconds:.2f}s")
        print(f"Quality valid: {audio_quality.is_valid}")
        
        # Generate visualizations
        safe_name = f"test_{test_index}_{model_name.replace('.stl', '')}_{audio_name.replace('.wav', '')}_{scale}x_{placement.name}"
        
        waveform_dir = os.path.join(output_dir, "waveforms")
        spectrogram_dir = os.path.join(output_dir, "spectrograms")
        os.makedirs(waveform_dir, exist_ok=True)
        os.makedirs(spectrogram_dir, exist_ok=True)
        
        waveform_path = os.path.join(waveform_dir, f"{safe_name}_waveform.png")
        spectrogram_path = os.path.join(spectrogram_dir, f"{safe_name}_spectrogram.png")
        
        plot_waveform(
            signal, sample_rate,
            f"Waveform: {model_name} | {audio_name} | {scale}x | {placement.name}",
            waveform_path
        )
        
        plot_spectrogram(
            signal, sample_rate,
            f"Spectrogram: {model_name} | {audio_name} | {scale}x | {placement.name}",
            spectrogram_path
        )
        
        return TestResult(
            model_name=model_name,
            audio_file=audio_name,
            scale=scale,
            placement_name=placement.name,
            success=True,
            output_path=output_wav,
            audio_quality=audio_quality,
            waveform_plot=waveform_path,
            spectrogram_plot=spectrogram_path,
        )
        
    except Exception as e:
        import traceback
        traceback.print_exc()
        return TestResult(
            model_name=model_name,
            audio_file=audio_name,
            scale=scale,
            placement_name=placement.name,
            success=False,
            error_message=str(e),
        )


def run_all_tests() -> List[TestResult]:
    """
    Run all configured test cases.
    
    Returns:
        List of TestResult objects
    """
    # Set up paths
    resources_dir = os.path.join(pyroomstudio_dir, "resources")
    sounds_dir = os.path.join(pyroomstudio_dir, "sounds", "sources")
    
    # Create output directory
    timestamp = datetime.now().strftime("%Y-%m-%d_%H-%M-%S")
    output_dir = os.path.join(pyroomstudio_dir, "tests", "test_outputs", f"simulation_{timestamp}")
    os.makedirs(output_dir, exist_ok=True)
    
    print(f"\n{'#'*80}")
    print("PYROOMSTUDIO ACOUSTIC SIMULATION TEST SUITE")
    print(f"{'#'*80}")
    print(f"\nOutput directory: {output_dir}")
    print(f"Models: {TEST_MODELS}")
    print(f"Audio files: {TEST_AUDIO_FILES}")
    print(f"Scales: {TEST_SCALES}")
    print(f"Placements: {[p.name for p in PLACEMENT_STRATEGIES]}")
    
    results = []
    test_index = 0
    
    # Run tests for each combination
    for model_name in TEST_MODELS:
        model_path = os.path.join(resources_dir, model_name)
        
        if not os.path.exists(model_path):
            print(f"WARNING: Model not found: {model_path}")
            continue
        
        for audio_name in TEST_AUDIO_FILES:
            audio_path = os.path.join(sounds_dir, audio_name)
            
            if not os.path.exists(audio_path):
                print(f"WARNING: Audio file not found: {audio_path}")
                continue
            
            for scale in TEST_SCALES:
                for placement in PLACEMENT_STRATEGIES:
                    test_index += 1
                    result = run_single_test(
                        model_path=model_path,
                        audio_path=audio_path,
                        scale=scale,
                        placement=placement,
                        output_dir=output_dir,
                        test_index=test_index,
                    )
                    results.append(result)
    
    # Generate comparison plot
    comparison_path = os.path.join(output_dir, "comparison.png")
    plot_comparison(results, comparison_path)
    
    # Generate report
    report_path = generate_report(results, output_dir)
    print(f"\nReport saved to: {report_path}")
    
    # Print summary
    total = len(results)
    passed = sum(1 for r in results if r.success and r.audio_quality and r.audio_quality.is_valid)
    
    print(f"\n{'#'*80}")
    print("TEST SUMMARY")
    print(f"{'#'*80}")
    print(f"Total tests: {total}")
    print(f"Passed: {passed}")
    print(f"Failed: {total - passed}")
    print(f"Pass rate: {passed/total*100:.1f}%" if total > 0 else "N/A")
    print(f"Output directory: {output_dir}")
    
    return results


if __name__ == "__main__":
    # Change to pyroomstudio directory for relative paths to work
    os.chdir(pyroomstudio_dir)
    
    results = run_all_tests()
    
    # Exit with error code if any tests failed
    failed = sum(1 for r in results if not r.success or not r.audio_quality or not r.audio_quality.is_valid)
    sys.exit(1 if failed > 0 else 0)
