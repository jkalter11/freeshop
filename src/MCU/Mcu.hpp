#ifndef FREESHOP_MCU_HPP
#define FREESHOP_MCU_HPP

#include <3ds.h>

namespace FreeShop
{

class MCU {

public:
  static MCU& getInstance();

  typedef struct
  {
    u32 ani;
    u8 r[32];
    u8 g[32];
    u8 b[32];
  } RGBLedPattern;

  Result mcuInit();
  Result mcuExit();

  void ledBlinkOnce(u32 col);
  void ledBlinkThrice(u32 col);
  void ledStay(u32 col);

  void ledReset();

private:
  Handle m_mcuHandle;
  RGBLedPattern m_ledPattern;
  Result mcuWriteRegister(u8 reg, void* data, u32 size);

  void ledApply();

};

} // namespace FreeShop

#endif // FREESHOP_MCU_HPP
