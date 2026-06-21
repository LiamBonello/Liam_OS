#include "system.h"
#include "version.h"
#include "log.h"

void system_print_boot_info(void) {
    log_info("System: " LIAM_OS_NAME " " LIAM_OS_VERSION " (" LIAM_OS_CODENAME ")");
    log_info("Target: " LIAM_OS_ARCH " / " LIAM_OS_BOOT_PROTOCOL " / " LIAM_OS_BOOTLOADER);
}