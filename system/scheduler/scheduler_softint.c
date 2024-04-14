#include "scheduler_softint.h"

#include "scheduler_internal.h"

#if SCH_CFG_ENABLE_SOFTINT

static __IO uint8_t imm = 0;
static __IO uint8_t ism[8] = {0};

_INLINE void sch_softint_trigger(uint8_t main_channel, uint8_t sub_channel) {
    // if (main_channel > 7 || sub_channel > 7) return;
    imm |= 1 << main_channel;
    ism[main_channel] |= 1 << sub_channel;
}

__weak void scheduler_softint_handler(uint8_t main_channel,
                                      uint8_t sub_channel_mask) {
    LOG_DEBUG_LIMIT(100, "soft_int %d:%d", main_channel, sub_channel_mask);
}

_INLINE void soft_int_runner(void) {
    if (imm) {
        uint8_t _ism;
        for (uint8_t i = 0; i < 8; i++) {
            if (imm & (1 << i)) {
                _ism = ism[i];
                ism[i] = 0;
                imm &= ~(1 << i);
                scheduler_softint_handler(i, _ism);
            }
        }
    }
}

#if SCH_CFG_ENABLE_TERMINAL
void softint_cmd_func(EmbeddedCli* cli, char* args, void* context) {
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
    sch_softint_trigger(ch, sub);
    PRINTLN(T_FMT(T_BOLD, T_GREEN) "soft_int: %d-%d triggered" T_RST, ch, sub);
}
#endif  // SCH_CFG_ENABLE_TERMINAL
#endif  // SCH_CFG_ENABLE_SOFTINT
