#include <ctype.h>
#include <log.h>
#include <main.h>
#include <string.h>

#include "nr_micro_shell.h"

/**
 * @brief ls command
 */
void shell_ls_cmd(char argc, char *argv) {
  unsigned int i = 0;
  for (i = 0; nr_shell.static_cmd[i].fp != NULL; i++) {
    if (strcmp(nr_shell.static_cmd[i].cmd, "ls") == 0) {
      continue;
    }
    shell_printf("%s", nr_shell.static_cmd[i].cmd);
    shell_printf("\r\n");
  }
}
ADD_CMD(ls, shell_ls_cmd);

// #include "dfu.h"
void reboot_cmd(char argc, char *argv) {
  // if (argc == 2 && strcmp(argv + argv[1], "dfu") == 0) {
  //   shell_printf("Entered DFU mode\r\n");
  //   printf_flush();
  //   reset_to_dfu();
  // }
  shell_printf("Rebooting...\r\n");
  printf_flush();
  NVIC_SystemReset();
}
ADD_CMD(reboot, reboot_cmd);

// #include "coremark.h"
// void coremark_cmd(char argc, char *argv) {
//   shell_printf("Runnning Coremark for %d iterations", ITERATIONS);
//   coremark_main();
//   shell_printf("Coremark finished");
// }
// ADD_CMD(coremark, coremark_cmd);
//
// void test_cmd(char argc, char *argv) {
//   shell_printf("argc: %d\r\n", argc);
//   for (int i = 0; i < argc; i++) {
//     shell_printf("argv[%d]: %s\r\n", i, argv + argv[i]);
//   }
// }
// ADD_CMD(test, test_cmd);
