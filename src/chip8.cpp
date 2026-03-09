#include "chip8.hpp"
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iostream>

const std::array<uint8_t, 80> FNT = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, 0x20, 0x60, 0x20, 0x20, 0x70, 0xF0, 0x10,
    0xF0, 0x80, 0xF0, 0xF0, 0x10, 0xF0, 0x10, 0xF0, 0x90, 0x90, 0xF0, 0x10,
    0x10, 0xF0, 0x80, 0xF0, 0x10, 0xF0, 0xF0, 0x80, 0xF0, 0x90, 0xF0, 0xF0,
    0x10, 0x20, 0x40, 0x40, 0xF0, 0x90, 0xF0, 0x90, 0xF0, 0xF0, 0x90, 0xF0,
    0x10, 0xF0, 0xF0, 0x90, 0xF0, 0x90, 0x90, 0xE0, 0x90, 0xE0, 0x90, 0xE0,
    0xF0, 0x80, 0x80, 0x80, 0xF0, 0xE0, 0x90, 0x90, 0x90, 0xE0, 0xF0, 0x80,
    0xF0, 0x80, 0xF0, 0xF0, 0x80, 0xF0, 0x80, 0x80,
};

Chip8::Chip8() : PC(0x200), CPF(5) {
  std::copy(FNT.begin(), FNT.end(), RAM.begin());
  srand(time(NULL));
}

bool Chip8::loadROM(const std::string &path) {
  // binary mode + jump to end
  std::ifstream file(path, std::ios::binary | std::ios::ate);

  if (!file.is_open()) {
    std::cerr << "Error: Failed to open ROM: " << path << std::endl;
    return false;
  }

  // tellg is current pointer pos,
  // since jumped to end via 'ate' its equal to size of file
  std::streamsize size = file.tellg();

  if (size > (4096 - 0x200)) {
    std::cerr << "Error: ROM overflow (size > 3584)" << std::endl;
    return false;
  }

  // jump to beginning
  file.seekg(0, std::ios::beg);

  // read with cast char* to uint8_t
  file.read(reinterpret_cast<char *>(&RAM[0x200]), size);

  return true;
}

void Chip8::cycle() {
  uint16_t opcode = (RAM[PC] << 8) | RAM[PC + 1];
  PC += 2;

  uint16_t addr = opcode & 0x0FFF;
  uint8_t n = (opcode & 0xF000) >> 12;
  uint8_t x = (opcode & 0x0F00) >> 8;
  uint8_t y = (opcode & 0x00F0) >> 4;
  uint8_t N = opcode & 0x000F;
  uint8_t byte = opcode & 0x00FF;

  switch (n) {
  case 0x0:
    switch (byte) {
    case 0xE0: // CLS
      SCR = {};
      std::cout << "CLS" << std::endl;
      break;
    case 0xEE: // RET
      SP--;
      PC = S[SP];
      std::cout << "RET " << std::hex << PC << std::dec << std::endl;
      break;
    }
    break;
  case 0x1: // JP addr
    PC = addr;
    std::cout << "JP " << std::hex << addr << std::dec << std::endl;
    break;
  case 0x2: // CALL addr
    S[SP] = PC;
    SP++;
    PC = addr;
    std::cout << "CALL " << std::hex << addr << std::dec << std::endl;
    break;
  case 0x3: // SE Vx, byte
    PC += (V[x] == byte) << 1;
    std::cout << "SE V" << +x << ", " << +byte << std::endl;
    break;
  case 0x4: // SNE Vx, byte
    PC += (V[x] != byte) << 1;
    std::cout << "SNE V" << +x << ", " << +byte << std::endl;
    break;
  case 0x5: // SE Vx, Vy
    PC += (V[x] == V[y]) << 1;
    std::cout << "SE V" << +x << ", V" << +y << std::endl;
    break;
  case 0x6: // LD Vx, byte
    V[x] = byte;
    std::cout << "LD V" << +x << ", " << std::hex << +byte << std::dec
              << std::endl;
    break;
  case 0x7: // ADD Vx, byte
    V[x] += byte;
    std::cout << "ADD V" << +x << ", " << std::hex << +byte << std::dec
              << std::endl;
    break;
  case 0x8:
    switch (N) {
    case 0x0: // LD Vx, Vy
      V[x] = V[y];
      std::cout << "LD V" << +x << ", V" << +y << std::endl;
      break;
    case 0x1: // OR Vx, Vy
      V[x] = V[x] | V[y];
      std::cout << "OR V" << +x << ", V" << +y << std::endl;
      break;
    case 0x2: // AND Vx, Vy
      V[x] = V[x] & V[y];
      std::cout << "AND V" << +x << ", V" << +y << std::endl;
      break;
    case 0x3: // XOR Vx, Vy
      V[x] = V[x] ^ V[y];
      std::cout << "XOR V" << +x << ", V" << +y << std::endl;
      break;
    case 0x4: // ADD Vx, Vy
      V[0xF] = 0xFF - V[x] < V[y];
      V[x] += V[y];
      std::cout << "ADD V" << +x << ", V" << +y << std::endl;
      break;
    case 0x5: // SUB Vx, Vy
      V[0xF] = V[x] >= V[y];
      V[x] -= V[y];
      std::cout << "SUB V" << +x << ", V" << +y << std::endl;
      break;
    case 0x6: // SHR Vx {, Vy}
      V[0xF] = V[x] & 0x1;
      V[x] >>= 1;
      std::cout << "SHR V" << +x << "{, V" << +y << "}" << std::endl;
      break;
    case 0x7: // SUBN Vx, Vy
      V[0xF] = V[y] >= V[x];
      V[x] = V[y] - V[x];
      std::cout << "SUBN V" << +x << ", V" << +y << std::endl;
      break;
    case 0xE: // SHL Vx, {, Vy}
      V[0xF] = (V[x] >> 7) & 0x1;
      V[x] <<= 1;
      std::cout << "SHL V" << +x << ", {, V" << +y << "}" << std::endl;
      break;
    }
    break;
  case 0x9: // SNE Vx, Vy
    PC += (V[x] != V[y]) << 1;
    std::cout << "SNE V" << +x << ", V" << +y << std::endl;
    break;
  case 0xA: // LD I, addr
    I = addr;
    std::cout << "LD I, " << std::hex << addr << std::dec << std::endl;
    break;
  case 0xB: // JP V0, addr
    PC = addr + V[0];
    std::cout << "JP V0, " << std::hex << addr << std::dec << std::endl;
    break;
  case 0xC: // RND Vx, byte
    V[x] = (rand() % 0x100) & byte;
    std::cout << "RND V" << +x << ", " << byte << std::endl;
    break;
  case 0xD: // DRW Vx, Vy, nibble
    V[0xF] = 0;
    for (uint16_t r = 0; r < N; ++r) {
      uint8_t pxs = RAM[I + r];
      for (uint8_t c = 0; c < 8; ++c) {
        if ((pxs & (0x80 >> c))) {
          uint8_t dstX = (V[x] + c) % 64;
          uint8_t dstY = (V[y] + r) % 32;
          uint16_t idx = (dstY << 6) + dstX;
          SCR[idx] ^= 0xFFFFFFFF;
          V[0xF] = !SCR[idx] + !!SCR[idx] * V[0xF];
        }
      }
    }
    std::cout << "DRW V" << +x << ", V" << +y << ", " << std::hex << +N
              << std::dec << std::endl;
    break;
  case 0xE:
    switch (byte) {
    case 0x9E: // SKP Vx
      PC += KB[V[x] & 0xF] << 1;
      std::cout << "SKP V" << +x << std::endl;
      break;
    case 0xA1: // SKNP Vx
      PC += !KB[V[x] & 0xF] << 1;
      std::cout << "SKNP V" << +x << std::endl;
      break;
    }
    break;
  case 0xF:
    switch (byte) {
    case 0x07: // LD Vx, DT
      V[x] = DT;
      std::cout << "LD V" << +x << ", " << +DT << std::endl;
      break;
    case 0x0A: // LD Vx, K
      bool key_press;

      for (uint8_t i = 0; i < 16; ++i) {
        if (KB[i]) {
          V[x] = i;
          key_press = true;
          break;
        }
      }

      if (!key_press)
        PC -= 2;
      else
        std::cout << "LD V" << +x << ", " << std::hex << +V[x] << std::dec
                  << std::endl;

      break;
    case 0x15: // LD DT, Vx
      DT = V[x];
      std::cout << "LD " << +DT << ", V" << +x << std::endl;
      break;
    case 0x18: // LD ST, Vx
      ST = V[x];
      std::cout << "LD " << +ST << ", V" << +x << std::endl;
      break;
    case 0x1E: // ADD I, Vx
      I += V[x];
      std::cout << "ADD " << std::hex << I << std::dec << ", V" << +x
                << std::endl;
      break;
    case 0x29: // LD F, Vx
      I = V[x] * 5;
      std::cout << "LD F, V" << +x << std::endl;
      break;
    case 0x33: // LD B, Vx
      RAM[I] = V[x] / 100;
      RAM[I + 1] = (V[x] / 10) % 10;
      RAM[I + 2] = V[x] % 10;
      std::cout << "LD B, V" << +x << std::endl;
      break;
    case 0x55: // LD [I], Vx
      for (uint8_t i = 0; i < x; ++i)
        RAM[I + i] = V[i];
      std::cout << "LD [I], V" << +x << std::endl;
      break;
    case 0x65: // LD Vx, [I]
      for (uint8_t i = 0; i < x; ++i)
        V[i] = RAM[I + i];
      std::cout << "LD V" << +x << ", [I]" << std::endl;
      break;
    }
    break;
  }
}

void Chip8::debugDraw() {
  for (uint8_t r = 0; r < 32; ++r) {
    for (uint8_t c = 0; c < 64; ++c)
      !SCR[r * 64 + c] ? std::cout << " " : std::cout << "█";
    std::cout << std::endl;
  }
}

void Chip8::updateTimers() {
  if (DT > 0)
    DT--;
  if (ST > 0)
    ST--;
}
