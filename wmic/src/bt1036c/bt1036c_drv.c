#include "bt1036c_drv.h"

#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>

#define RX_BUFF_SIZE 500

#define BT1036C_DECODE_THREAD_STACK 512
K_THREAD_STACK_DEFINE(bt1036c_decode_stack, BT1036C_DECODE_THREAD_STACK);

K_SEM_DEFINE(uart_sem, 0, 1);

static struct k_thread bt1036c_decode_tcb;

const static struct device *uart_int;

struct bluetooth_peers
{
    char name[50];
    char mac[13];
};

struct bt1036c_status_handler
{
    int devstat;
    int pwrstat;
    char ver[64];
    int profile;
    int sppstat;
    int gattstat;
    int a2dpstat;
    int avrcpstat;
    int hfpstat;
    int pbstat;
    int ok;
    char name[30];
    int i2scfg;
    struct bluetooth_peers peer[10];
    uint8_t peer_num;
} bt1036c_status = {0};

static void uart_write_str(const char *s);
static int decode_bt1036c_data(char *s);
static void bt1036c_decode_thread(void *a, void *b, void *c);
static void uart_irq_cb(const struct device *dev, void *user_data);
static void extract_name(const char *input, char *name_out);
static void extract_mac(const char *input, char *mac_out);

// Temporary buffer used to read from FIFO in IRQ context 
static uint8_t irq_buf[128];
static uint8_t cmd_buff_rx[RX_BUFF_SIZE];
static uint16_t rx_buff_idx = 0;

void bt1036c_config(const struct device *uart, const uint8_t txrx_config)
{
    uart_int = uart;

    uart_irq_callback_user_data_set(uart, uart_irq_cb, NULL);
    uart_irq_rx_enable(uart);

    k_thread_create(&bt1036c_decode_tcb,
                    bt1036c_decode_stack,
                    BT1036C_DECODE_THREAD_STACK,
                    bt1036c_decode_thread,
                    NULL, NULL, NULL,
                    3, 0, K_NO_WAIT);

    // Reset the module to default settings
    bt1036c_at_send("RESTORE");
    k_sleep(K_MSEC(1000));

    /*
     * NOTE; here we are still in the main branch,
     *       we need to add a small sleep delay
     *       after each send to allow the bt thread
     *       to execute and decode the received data
     */

    if (txrx_config == BT103036C_CONFIG_RX)
    {
        // SINK (riceve audio via Bluetooth)
        bt1036c_at_send("PROFILE=32"); // A2DP Sink
        k_sleep(K_MSEC(50));

        bt1036c_at_send("PAIR=1"); // Discoverable
        k_sleep(K_MSEC(50));
    }
    else // txrx_config == BT103036C_CONFIG_TX
    {
        bt1036c_at_send("PROFILE=64"); // A2DP Source
        k_sleep(K_MSEC(50));

        bt1036c_at_send("I2SCFG=71"); // I2S slave, 44.1kHz, 32 bit data size
        k_sleep(K_MSEC(50));

        bt1036c_at_send("AUTOCONN=64"); // Reconnect in case of connection lost
        k_sleep(K_MSEC(50));
    }

    k_sleep(K_MSEC(500));

    bt1036c_at_send("REBOOT"); // Reboot to make changes effective
    k_sleep(K_MSEC(1000));

    bt1036c_at_send("A2DPSTAT"); // Read AD2DP status
    k_sleep(K_MSEC(200));

    if (txrx_config != BT103036C_CONFIG_RX)
    {
        bt1036c_at_send("SCAN=1"); // Scan MACs address advertised

        k_sleep(K_MSEC(10000)); // Longer wait to scan all the peripherals available

        bt1036c_at_send("A2DPSTAT"); // Should be connected (3)

        k_sleep(K_MSEC(200));

        // Wait for sink MACS address
        while (bt1036c_status.peer[0].name[0] == 0)
            ;

        bt1036c_at_send("A2DPCONN=414211F3B97A"); // Hardwired MAC address
        k_sleep(K_MSEC(2000));

        bt1036c_at_send("AUDROUTE=1"); // Start AD2DP communication of I2S data received
        k_sleep(K_MSEC(100));
    }
}

void bt1036c_at_send(const char *cmd)
{
    char buffer[100];
    strcpy(buffer, "AT+");
    strncat(buffer, cmd, sizeof(buffer) - strlen(buffer) - 1);
    strncat(buffer, "\r\n", sizeof(buffer) - strlen(buffer) - 1);
    uart_write_str(buffer);
}

static void bt1036c_decode_thread(void *a, void *b, void *c)
{
    static uint16_t decode_idx = 0;
    char payload[100];

    while (1)
    {
        // Wait for UART received data interrupt
        k_sem_take(&uart_sem, K_FOREVER);

        // Avoid UART interrupt when here
        uart_irq_rx_disable(uart_int);

        // Consider the distance between the two indexes
        // NOTE; summing RX_BUFF_SIZE is necessary to cojnsider also the case in which
        //       rx_buff_idx is one turn haed compared to decode_idx
        uint16_t dist = (rx_buff_idx + RX_BUFF_SIZE - decode_idx) % RX_BUFF_SIZE;
        if (dist > 2) // Condition needed because each packet starts with \r\n
        {
            uint16_t next = (decode_idx + 1) % RX_BUFF_SIZE;

            if (cmd_buff_rx[decode_idx] == '\r' &&
                cmd_buff_rx[next] == '\n')
            {
                uint16_t i = (decode_idx + 2) % RX_BUFF_SIZE;
                uint16_t payload_idx = 0;

                while (i != rx_buff_idx)
                {
                    char c = cmd_buff_rx[i];

                    if (c == '\n')
                    {
                        // End sequence \n found
                        payload[payload_idx] = '\0';
                        decode_bt1036c_data(payload);
                        decode_idx = (i + 1) % RX_BUFF_SIZE; // only now we can increase the decoding index (in this way we start previous index until the end sequence is reached)
                        break;
                    }

                    if (c != '\r' && payload_idx < sizeof(payload) - 1)
                    {
                        payload[payload_idx++] = c;
                    }

                    i = (i + 1) % RX_BUFF_SIZE;
                }
            }
            else // No \r\n sequence, data corrupted
            {
                // Search for \r\n sequence in the remaining buffer
                while ((cmd_buff_rx[decode_idx] != '\r') || (cmd_buff_rx[next] != '\n'))
                {
                    if (next != rx_buff_idx)
                    {
                        decode_idx = next;
                        next = (decode_idx + 1) % RX_BUFF_SIZE;
                    }
                    else
                    {
                        // End of the buffer reached
                        break;
                    }
                }
            }
        }
        // Enable UART interrupt and wait for data to accumulate
        uart_irq_rx_enable(uart_int);
        k_sleep(K_MSEC(1));
    }
}

static void uart_irq_cb(const struct device *dev, void *user_data)
{
    ARG_UNUSED(user_data);

    if (!uart_irq_update(dev))
        return;

    while (uart_irq_rx_ready(dev))
    {
        int rx = uart_fifo_read(dev, irq_buf, sizeof(irq_buf));
        if (rx > 0)
        {
            int16_t space_remaining = (RX_BUFF_SIZE - rx_buff_idx);
            if (rx <= space_remaining)
            {
                memcpy(&cmd_buff_rx[rx_buff_idx], irq_buf, rx);
                rx_buff_idx += rx;
            }
            else
            {
                memcpy(&cmd_buff_rx[rx_buff_idx], irq_buf, space_remaining);
                rx_buff_idx = 0;
                memcpy(&cmd_buff_rx[rx_buff_idx], irq_buf, (rx - space_remaining));
                rx_buff_idx = (rx - space_remaining);
            }
            k_sem_give(&uart_sem);
        }
    }
}

static void uart_write_str(const char *s)
{
    for (size_t i = 0; i < strlen(s); i++)
    {
        uart_poll_out(uart_int, s[i]);
    }
}

static int decode_bt1036c_data(char *s)
{
    char cmd[20] = {0};
    char data[100] = {0};
    char *cmd_end = strchr(s, '='); // End of the command part
    char *data_start = NULL;

    if (!cmd_end)
    {
        // If no end reached it means Ok command or error

        // OK 
        if (strcmp(s, "OK") == 0)
        {
            bt1036c_status.ok = 1;
            return 0;
        }
        else
        {
            return -1;
        }
    }

    data_start = cmd_end + 1; // Start of data payload

    int len = cmd_end - s;
    memcpy(cmd, s, len);
    cmd[len] = '\0';

    strcpy(data, data_start);

    // +DEVSTAT 
    if (strcmp(cmd, "+DEVSTAT") == 0)
    {
        bt1036c_status.devstat = atoi(data);
        return 0;
    }

    // +PWRSTAT 
    if (strcmp(cmd, "+PWRSTAT") == 0)
    {
        bt1036c_status.pwrstat = atoi(data);
        return 0;
    }

    // +VER 
    if (strcmp(cmd, "+VER") == 0)
    {
        strncpy(bt1036c_status.ver, data, sizeof(bt1036c_status.ver) - 1);
        return 0;
    }

    // +PROFILE 
    if (strcmp(cmd, "+PROFILE") == 0)
    {
        bt1036c_status.profile = atoi(data);
        return 0;
    }

    // +SPPSTAT
    if (strcmp(cmd, "+SPPSTAT") == 0)
    {
        bt1036c_status.sppstat = atoi(data);
        return 0;
    }

    // +GATTSTAT 
    if (strcmp(cmd, "+GATTSTAT") == 0)
    {
        bt1036c_status.gattstat = atoi(data);
        return 0;
    }

    // +A2DPSTAT 
    if (strcmp(cmd, "+A2DPSTAT") == 0)
    {
        bt1036c_status.a2dpstat = atoi(data);
        return 0;
    }

    // +AVRCPSTAT 
    if (strcmp(cmd, "+AVRCPSTAT") == 0)
    {
        bt1036c_status.avrcpstat = atoi(data);
        return 0;
    }

    // +HFPSTAT 
    if (strcmp(cmd, "+HFPSTAT") == 0)
    {
        bt1036c_status.hfpstat = atoi(data);
        return 0;
    }

    // +PBSTAT 
    if (strcmp(cmd, "+PBSTAT") == 0)
    {
        bt1036c_status.pbstat = atoi(data);
        return 0;
    }

    // +NAME 
    if (strcmp(cmd, "+NAME") == 0)
    {
        strncpy(bt1036c_status.name, data, sizeof(bt1036c_status.name) - 1);
        return 0;
    }

    // +I2SCFG 
    if (strcmp(cmd, "+I2SCFG") == 0)
    {
        bt1036c_status.i2scfg = atoi(data);
        return 0;
    }

    // +SCAN 
    if (strcmp(cmd, "+SCAN") == 0)
    {
        extract_name(data, bt1036c_status.peer[bt1036c_status.peer_num].name);
        extract_mac(data, bt1036c_status.peer[bt1036c_status.peer_num].mac);
        bt1036c_status.peer_num++;
        return 0;
    }
    return -1;
}

static void extract_name(const char *input, char *name_out)
{
    uint8_t comma_num = 0;

    while(comma_num < 4)
    {
        if(*input == ',')
        {
            comma_num++;
        }
        input++;
    }

    while(*input != ',')
    {
        *name_out = *input;
        name_out++;
        input++; 
    }

    return; 
}

static void extract_mac(const char *input, char *mac_out)
{
    uint8_t comma_num = 0;

    while(comma_num < 3)
    {
        if(*input == ',')
        {
            comma_num++;
        }
        input++;
    }

    while(*input != ',')
    {
        *mac_out = *input;
        mac_out++;
        input++; 
    }

    return; 
}