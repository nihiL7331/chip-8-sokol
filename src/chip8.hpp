#ifndef CHIP8_HPP
#define CHIP8_HPP

#include <array>
#include <cstdint>
#include <string>

class Chip8 {
private:
  std::array<uint8_t, 4096> RAM{};
  std::array<uint8_t, 16> V{};
  uint16_t I{};
  uint16_t PC;
  std::array<uint16_t, 16> S{};
  uint8_t SP{};
  uint8_t DT{};
  uint8_t ST{};
  const uint8_t CPF;
  std::array<bool, 16> KB{};
  std::array<uint32_t, 64 * 32> SCR{};

public:
  Chip8();
  bool loadROM(const std::string &path);
  void cycle();
  void debugDraw();
  void updateTimers();
  uint8_t getSoundTimer() const { return ST; }
  const uint8_t getCyclesPerFrame() const { return CPF; }
  const void *getDisplayData() const { return SCR.data(); }
  void setKey(uint8_t idx, bool prsd) {
    if (idx < 16)
      KB[idx] = prsd;
  }
};

#endif
