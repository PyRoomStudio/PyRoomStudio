#pragma once

#include "HrtfDataset.h"
#include "core/Types.h"

namespace prs {

// Azimuth and elevation of a sound source in the listener's head frame.
// Azimuth 0° is straight ahead; positive azimuth is to the listener's right.
// Elevation 0° is the horizontal plane; positive elevation is above.
struct HeadFrameDirection {
    float azimuthDeg = 0.0f;
    float elevationDeg = 0.0f;
};

// Convert a world-space source position to a head-relative direction.
//
// listenerForward must be a non-zero direction vector pointing in the
// direction the listener faces.  worldUp is the scene-level up vector
// (defaults to +Y).  The function normalises all inputs internally.
HeadFrameDirection toHeadFrame(const Vec3f& sourcePos, const Vec3f& listenerPos,
                               const Vec3f& listenerForward,
                               const Vec3f& worldUp = Vec3f{0.0f, 1.0f, 0.0f});

// Look up the nearest HRTF sample for the given azimuth/elevation.
//
// Always returns a valid HrtfSample.  If the dataset returns nullopt
// (empty dataset), a unit-impulse passthrough sample is returned so the
// caller never has to special-case a missing result.
HrtfSample lookupWithFallback(const HrtfDataset& dataset, float azimuthDeg, float elevationDeg);

} // namespace prs
