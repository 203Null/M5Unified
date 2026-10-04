// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Cardenza contributors
#include "../src/utility/CardenzaCodec.hpp"
#include <cassert>
#include <cstdio>
#include <cstring>
struct Bus {
  mutable unsigned reads = 0, writes = 0;
  mutable std::uint8_t registers[256] = {};
  bool available = true, corrupt = false;
  int fail_read = -1, fail_write = -1;
  Bus() { registers[0xFD]=0x81; registers[0xFE]=0x55; registers[0xFF]=0x11; }
  bool readRegister(std::uint8_t addr, std::uint8_t reg, std::uint8_t* data,
                    unsigned len, std::uint32_t) const {
    assert(addr==0x08 && len==1); ++reads;
    if (!available || reg==fail_read) return false;
    *data=registers[reg] ^ (corrupt && reg==0x11 ? 1 : 0); return true;
  }
  bool writeRegister8(std::uint8_t addr, std::uint8_t reg, std::uint8_t data,
                      std::uint32_t) const {
    assert(addr==0x08); ++writes;
    if (!available || reg==fail_write) return false;
    registers[reg]=data; return true;
  }
};
int main() {
  Bus good; assert(m5::cardenza::identify(good)); assert(good.writes==0);
  for (int fault=0; fault<5; ++fault) {
    Bus bus;
    if (fault==0) bus.available=false;
    if (fault==1) bus.registers[0xFD]=0x83; // ES8311 is not Cardenza
    if (fault==2) bus.registers[0xFE]=0x56;
    if (fault==3) bus.registers[0xFF]=0x21;
    if (fault==4) bus.fail_read=0xFE;
    assert(!m5::cardenza::identify(bus));
    assert(!m5::cardenza::configure(bus,32,16)); assert(bus.writes==0);
  }
  Bus invalid;
  assert(!m5::cardenza::configure(invalid,32,24));
  assert(!m5::cardenza::configure(invalid,48,16));
  assert(!m5::cardenza::configure(invalid,64,8));
  assert(invalid.reads==0 && invalid.writes==0);
  Bus sixteen; assert(m5::cardenza::configure(sixteen,32,16));
  assert(sixteen.registers[1]==0xE1 && sixteen.registers[4]==32
      && sixteen.registers[5]==1 && sixteen.registers[0x11]==0x30);
  const unsigned widths[]={16,24,32};
  for (auto bits:widths) {
    Bus bus; assert(m5::cardenza::configure(bus,64,bits));
    assert(bus.registers[1]==0xA1 && bus.registers[4]==64 && bus.registers[5]==2);
    assert(bus.registers[0x11]==(bits==16?0x30:bits==32?0x40:0));
  }
  Bus failed; failed.fail_write=0x20;
  assert(!m5::cardenza::configure(failed,32,16)); assert(failed.registers[0]==0);
  Bus readback; readback.corrupt=true;
  assert(!m5::cardenza::configure(readback,32,16));
  Bus resume; assert(m5::cardenza::configure(resume,64,32));
  assert(m5::cardenza::configure(resume,32,16));
  assert(resume.registers[0x11]==0x30 && resume.registers[4]==32);
  puts("PASS: absent/wrong ID causes no writes; valid formats; I2C faults; PCM resume");
}
