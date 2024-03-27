/**
 * @Author: Amélie Heinrich
 * @Create Time: 2024-03-27 13:36:21
 */

#include "core/log.h"

int main(void)
{
    log_init();
    log_info("Pipi");
    log_info("Prout");
    log_exit();
    return 0;
}
