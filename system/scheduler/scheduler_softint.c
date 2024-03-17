#include "scheduler_softint.h"

#include "scheduler_internal.h"

#if SCH_CFG_ENABLE_SOFTINT

static __IO uint8_t imm = 0;
static __IO uint8_t ism[8] = {0};

_INLINE void Sch_TriggerSoftInt(uint8_t mainChannel, uint8_t subChannel) {
  if (mainChannel > 7 || subChannel > 7) return;
  imm |= 1 << mainChannel;
  ism[mainChannel] |= 1 << subChannel;
}

__weak void Scheduler_SoftInt_Handler(uint8_t mainChannel, uint8_t subMask) {
  LOG_DEBUG_LIMIT(100, "SoftInt %d:%d", mainChannel, subMask);
}

_INLINE void SoftInt_Runner(void) {
  if (imm) {
    uint8_t _ism;
    for (uint8_t i = 0; i < 8; i++) {
      if (imm & (1 << i)) {
        _ism = ism[i];
        ism[i] = 0;
        imm &= ~(1 << i);
        Scheduler_SoftInt_Handler(i, _ism);
      }
    }
  }
}

#if SCH_CFG_ENABLE_TERMINAL
void softint_cmd_func(EmbeddedCli *cli, char *args, void *context) {
  size_t argc = embeddedCliGetTokenCount(args);
  if (!argc) {
    embeddedCliPrintCurrentHelp(cli);
    return;
  }
  if (argc < 2) {
    PRINTLN(T_FMT(T_BOLD, T_RED) "2 parameters are required" T_RST);
    return;
  }
  uint8_t ch = atoi(embeddedCliGetToken(args, 1));
  uint8_t sub = atoi(embeddedCliGetToken(args, 2));
  if (ch > 7 || sub > 7) {
    PRINTLN(T_FMT(T_BOLD, T_RED) "(Sub-)Channel must be 0~7" T_RST);
    return;
  }
  Sch_TriggerSoftInt(ch, sub);
  PRINTLN(T_FMT(T_BOLD, T_GREEN) "SoftInt: %d-%d triggered" T_RST, ch, sub);
}
#endif  // SCH_CFG_ENABLE_TERMINAL
#endif  // SCH_CFG_ENABLE_SOFTINT
