#include "bt1036c_drv.h"

#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define RX_BUFF_SIZE 500

#define BT1036C_DECODE_THREAD_STACK 512
K_THREAD_STACK_DEFINE(bt1036c_decode_stack, BT1036C_DECODE_THREAD_STACK);

K_SEM_DEFINE(uart_sem, 0, 1);

static struct k_thread bt1036c_decode_tcb;

static struct device *uart_int;

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
} bt1036c_status = {0};

static void uart_write_str(const char *s);
static int decode_bt1036c_data(char *s);
static void bt1036c_decode_thread(void *a, void *b, void *c);
static void uart_irq_cb(const struct device *dev, void *user_data);

/* Temporary buffer used to read from FIFO in IRQ context */
static uint8_t irq_buf[64];
static uint8_t cmd_buff_rx[RX_BUFF_SIZE];
static uint16_t rx_buff_idx = 0;

void bt1036c_config(struct device *uart)
{
    uart_int = uart;

    uart_irq_callback_user_data_set(uart, uart_irq_cb, NULL);
    uart_irq_rx_enable(uart);

    k_thread_create(&bt1036c_decode_tcb,
                    bt1036c_decode_stack,
                    BT1036C_DECODE_THREAD_STACK,
                    bt1036c_decode_thread,
                    NULL, NULL, NULL,
                    5, 0, K_NO_WAIT);
    k_sleep(K_MSEC(2000));
    //bt1036c_at_send("NAME=Sound Tx,0");
    //bt1036c_at_send("NAME");
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
        k_sem_take(&uart_sem, K_FOREVER);
        uint16_t dist = (rx_buff_idx + RX_BUFF_SIZE - decode_idx) % RX_BUFF_SIZE;
        if (dist > 2)
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
                        payload[payload_idx] = '\0'; 
                        decode_bt1036c_data(payload);
                        decode_idx = (i + 1) % RX_BUFF_SIZE;
                        break;
                    }

                    if (c != '\r' && payload_idx < sizeof(payload) - 1)
                    {
                        payload[payload_idx++] = c;
                    }

                    i = (i + 1) % RX_BUFF_SIZE;
                }
            }
            else
            {
                while((cmd_buff_rx[decode_idx] != '\r') || (cmd_buff_rx[next] != '\n'))
                {
                    if(next != rx_buff_idx)
                    {
                        decode_idx = next;
                        next = (decode_idx + 1) % RX_BUFF_SIZE;
                    }
                    else
                    {
                        break;
                    }
                }
            }
        }
        k_sleep(K_MSEC(10));
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
    char *cmd_end = strchr(s, '=');
    char *data_start = NULL;

    if (!cmd_end)
    {
        /* OK */
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

    data_start = cmd_end + 1;

    int len = cmd_end - s;
    memcpy(cmd, s, len);
    cmd[len] = '\0';

    strcpy(data, data_start);

    /* +DEVSTAT */
    if (strcmp(cmd, "+DEVSTAT") == 0)
    {
        bt1036c_status.devstat = atoi(data);
        return 0;
    }

    /* +PWRSTAT */
    if (strcmp(cmd, "+PWRSTAT") == 0)
    {
        bt1036c_status.pwrstat = atoi(data);
        return 0;
    }

    /* +VER */
    if (strcmp(cmd, "+VER") == 0)
    {
        strncpy(bt1036c_status.ver, data, sizeof(bt1036c_status.ver)-1);
        return 0;
    }

    /* +PROFILE */
    if (strcmp(cmd, "+PROFILE") == 0)
    {
        bt1036c_status.profile = atoi(data);
        return 0;
    }

    /* +SPPSTAT */
    if (strcmp(cmd, "+SPPSTAT") == 0)
    {
        bt1036c_status.sppstat = atoi(data);
        return 0;
    }

    /* +GATTSTAT */
    if (strcmp(cmd, "+GATTSTAT") == 0)
    {
        bt1036c_status.gattstat = atoi(data);
        return 0;
    }

    /* +A2DPSTAT */
    if (strcmp(cmd, "+A2DPSTAT") == 0)
    {
        bt1036c_status.a2dpstat = atoi(data);
        return 0;
    }

    /* +AVRCPSTAT */
    if (strcmp(cmd, "+AVRCPSTAT") == 0)
    {
        bt1036c_status.avrcpstat = atoi(data);
        return 0;
    }

    /* +HFPSTAT */
    if (strcmp(cmd, "+HFPSTAT") == 0)
    {
        bt1036c_status.hfpstat = atoi(data);
        return 0;
    }

    /* +PBSTAT */
    if (strcmp(cmd, "+PBSTAT") == 0)
    {
        bt1036c_status.pbstat = atoi(data);
        return 0;
    }

    /* +NAME */
    if (strcmp(cmd, "+NAME") == 0)
    {
        strncpy(bt1036c_status.name, data, sizeof(bt1036c_status.name)-1);
        return 0;
    }    

    return -1;
}
