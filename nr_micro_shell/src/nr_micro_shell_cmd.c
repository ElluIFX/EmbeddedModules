#include <ctype.h>
#include <log.h>
#include <main.h>
#include <string.h>

#include "nr_micro_shell.h"

void reboot_cmd(char argc, char *argv) {
  shell_printf("Rebooting...\r\n");
  printf_flush();
  NVIC_SystemReset();
}
ADD_CMD(reboot, reboot_cmd);

// void test_cmd(char argc, char *argv) {
//   shell_printf("argc: %d\r\n", argc);
//   for (int i = 0; i < argc; i++) {
//     shell_printf("argv[%d]: %s\r\n", i, argv + argv[i]);
//   }
// }
// ADD_CMD(test, test_cmd);
