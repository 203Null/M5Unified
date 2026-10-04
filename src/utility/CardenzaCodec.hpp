// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Cardenza contributors
#pragma once
#include <cstdint>

namespace m5 { namespace cardenza {
// ES8156's documented family signature is 0x81/0x55 (the second byte is
// NOT 0x56). Revision low nibble may differ between production batches.
static constexpr std::uint8_t codec_address = 0x08;
static constexpr std::uint32_t codec_frequency = 100000;

template<class Bus> bool identify(const Bus& bus)
{
  std::uint8_t msb = 0, lsb = 0, revision = 0;
  return bus.readRegister(codec_address, 0xFD, &msb, 1, codec_frequency)
      && bus.readRegister(codec_address, 0xFE, &lsb, 1, codec_frequency)
      && bus.readRegister(codec_address, 0xFF, &revision, 1, codec_frequency)
      && msb == 0x81 && lsb == 0x55 && (revision & 0xF0) == 0x10;
}

// frame_bits is BCLK/LRCK (two physical slots), not PCM sample width.
// Keep this algorithm independent of GPIO/Arduino for fault-injection tests.
template<class Bus> bool configure(const Bus& bus, unsigned frame_bits, unsigned sample_bits)
{
  if ((frame_bits != 32 && frame_bits != 64)
      || (sample_bits != 16 && sample_bits != 24 && sample_bits != 32)
      || 2 * sample_bits > frame_bits || !identify(bus)) return false;
  const std::uint8_t pairs[][2] = {
    {0xFC,0x00}, {0x02,0x84}, {0x13,0x00}, {0x20,0x2A}, {0x21,0x3C},
    {0x22,0x02}, {0x24,0x07}, {0x23,0x40}, {0x0A,0x01}, {0x0B,0x01},
    {0x11,(std::uint8_t)(sample_bits == 16 ? 0x30 : sample_bits == 32 ? 0x40 : 0)},
    {0x14,0xBF}, {0x01,(std::uint8_t)(frame_bits == 32 ? 0xE1 : 0xA1)},
    {0x09,0x02}, {0x03,0x00}, {0x04,(std::uint8_t)frame_bits},
    {0x05,(std::uint8_t)(frame_bits == 32 ? 1 : 2)}, {0x0D,0x14},
    {0x18,0x00}, {0x08,0x3F}, {0x00,0x02}, {0x00,0x03}, {0x25,0x20}
  };
  for (const auto& pair : pairs)
    if (!bus.writeRegister8(codec_address, pair[0], pair[1], codec_frequency)) return false;
  const std::uint8_t verify[] = {0x02,0x11,0x14,0x01,0x09,0x04,0x05};
  for (auto reg : verify) {
    std::uint8_t actual = 0, expected = 0;
    for (const auto& pair : pairs) if (pair[0] == reg) expected = pair[1];
    if (!bus.readRegister(codec_address, reg, &actual, 1, codec_frequency)
        || actual != expected) return false;
  }
  return true;
}
}} // namespace m5::cardenza
