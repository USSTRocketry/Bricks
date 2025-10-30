#pragma once

#include <cassert>
#include <cstdint>

namespace ra
{
namespace version
{
constexpr auto MajorBits = 6;  // Major occupies the top 6 bits
constexpr auto MinorBits = 10; // Minor occupies the next 10 bits
constexpr auto PatchBits = 16; // Patch occupies the bottom 16 bits

constexpr auto MajorMask = (1U << MajorBits) - 1; // Mask for the major version (0x3F)
constexpr auto MinorMask = (1U << MinorBits) - 1; // Mask for the minor version (0x3FF)
constexpr auto PatchMask = (1U << PatchBits) - 1; // Mask for the patch version (0xFFFF)

constexpr auto MajorShift = 26; // Major version is at the top 6 bits
constexpr auto MinorShift = 16; // Minor version is at bits 16-25
constexpr auto PatchShift = 0;  // Patch version is at the bottom 16 bits
} // namespace version

constexpr uint8_t GetMajor(uint32_t version) { return (version >> version::MajorShift) & version::MajorMask; }

constexpr uint16_t GetMinor(uint32_t version) { return (version >> version::MinorShift) & version::MinorMask; }

constexpr uint16_t GetPatch(uint32_t version) { return version & version::PatchMask; }

constexpr uint32_t SetVersion(uint8_t major, uint16_t minor, uint16_t patch)
{
    assert(major <= version::MajorMask); // Major should be within 0-63 (6 bits)
    assert(minor <= version::MinorMask); // Minor should be within 0-1023 (10 bits)
    assert(patch <= version::PatchMask); // Patch should be within 0-65535 (16 bits)
                                         //
    return (static_cast<uint32_t>(major) << version::MajorShift) |
           (static_cast<uint32_t>(minor) << version::MinorShift) | (static_cast<uint32_t>(patch) & version::PatchMask);
}
} // namespace ra
