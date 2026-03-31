#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/usb/usb_device.h>
#include <stdio.h>

int main(void)
{
    int ret;
    uint32_t count = 0;

    /* Enable USB */
    ret = usb_enable(NULL);
    if (ret != 0 && ret != -EALREADY) {
        printk("USB enable failed: %d\n", ret);
    }

    /* Wait for USB to be ready */
    k_sleep(K_MSEC(2000));

    printk("\n========================================\n");
    printk("UART Test - REVITA Started\n");
    printk("RAK4631 via PCA10056 S/N: 1050295470\n");
    printk("========================================\n\n");

    while (1) {
        printk("revita: %u\n", count);
        count++;
        k_sleep(K_MSEC(1000));
    }

    return 0;
}
