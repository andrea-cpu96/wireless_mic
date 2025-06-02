#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

/* Include header for nrfx drivers */
#include <nrfx_saadc.h>
#include <nrfx_timer.h>
#include <helpers/nrfx_gppi.h>
#if defined(DPPI_PRESENT)
#include <nrfx_dppi.h>
#else
#include <nrfx_ppi.h>
#endif

#include "adc_cont_samp.h"

#define SAADC_BUFFER_SIZE 1
#define SAADC_SAMPLE_INTERVAL_US 5
#define TIMER_INSTANCE_NUMBER 2
#define SAADC_INPUT_PIN NRF_SAADC_INPUT_AIN0

const nrfx_timer_t timer_instance = NRFX_TIMER_INSTANCE(TIMER_INSTANCE_NUMBER);          // Timer 2 instance
static nrfx_saadc_channel_t channel = NRFX_SAADC_DEFAULT_CHANNEL_SE(SAADC_INPUT_PIN, 0); // Channel 0 default config
static int16_t saadc_sample_buffer[2][SAADC_BUFFER_SIZE];
static uint32_t saadc_current_buffer = 0;

static int timer_config(void);
static int saadc_config(void);
static int configure_ppi(void);

volatile int32_t average = 0;

static void saadc_event_handler(nrfx_saadc_evt_t const *p_event)
{
    nrfx_err_t err;
    switch (p_event->type)
    {
    case NRFX_SAADC_EVT_READY: // First buffer initialized event

        /* Enable timer (this also enable the sampling triggered by the PPI) */
        nrfx_timer_enable(&timer_instance);
        break;

    case NRFX_SAADC_EVT_BUF_REQ: // A buffer has been required by the ADC

        /* Set up the next available buffer. Alternate between buffer 0 and 1 */
        err = nrfx_saadc_buffer_set(saadc_sample_buffer[(saadc_current_buffer++) % 2], SAADC_BUFFER_SIZE);
        if (err != NRFX_SUCCESS)
        {
            printk("ADC; buffer set error\r\n");
            return;
        }
        break;

    case NRFX_SAADC_EVT_DONE: // A buffer has been filled
/*
        // Mesure the average ADC value inside the buffer 
        int16_t max = INT16_MIN;
        int16_t min = INT16_MAX;
        int16_t current_value;
        for (int i = 0; i < p_event->data.done.size; i++)
        {
            current_value = ((int16_t *)(p_event->data.done.p_buffer))[i];
            average += current_value;
            if (current_value > max)
            {
                max = current_value;
            }
            if (current_value < min)
            {
                min = current_value;
            }
        }
        average = average / p_event->data.done.size;
        printk("ADC; average = %d\r\n", average);
        break;
        */
    default:
        printk("ADC; event error\r\n");
        break;
    }
}

void adc_continuos_sampling_sample(void)
{
    timer_config();
    saadc_config();
    configure_ppi();
}

static int timer_config(void)
{
    nrfx_err_t err;

    nrfx_timer_config_t timer_config = NRFX_TIMER_DEFAULT_CONFIG(1000000); // Timer frequency
    err = nrfx_timer_init(&timer_instance, &timer_config, NULL);           // The third parameter is the reference to the event handler (NULL if none)
    if (err != NRFX_SUCCESS)
    {
        printk("TIMER; init error\r\n");
        return -1;
    }

    uint32_t timer_ticks = nrfx_timer_us_to_ticks(&timer_instance, SAADC_SAMPLE_INTERVAL_US);
    nrfx_timer_extended_compare(&timer_instance, NRF_TIMER_CC_CHANNEL0, timer_ticks, NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK, false); // Compare the timer value with the number of ticks passed and creates an event, also specify an action (reset the timer)

    return 0;
}

static int saadc_config(void)
{
    nrfx_err_t err;

    /* Connect ADC interrupt to nrfx interrupt handler */
    IRQ_CONNECT(DT_IRQN(DT_NODELABEL(adc)),           // Extract irq number
                DT_IRQ(DT_NODELABEL(adc), priority),  // Extract irq priority
                nrfx_isr, nrfx_saadc_irq_handler, 0); // Connect the interrupt to the nrf interrupt handler

    err = nrfx_saadc_init(DT_IRQ(DT_NODELABEL(adc), priority));
    if (err != NRFX_SUCCESS)
    {
        printk("ADC; init error\r\n");
        return -1;
    }

    channel.channel_config.gain = NRF_SAADC_GAIN1_6;
    err = nrfx_saadc_channels_config(&channel, 1);
    if (err != NRFX_SUCCESS)
    {
        printk("ADC; channel config error\r\n");
        return -1;
    }

    nrfx_saadc_adv_config_t saadc_adv_config = NRFX_SAADC_DEFAULT_ADV_CONFIG; // ADC default advanced config
    err = nrfx_saadc_advanced_mode_set(BIT(0),
                                       NRF_SAADC_RESOLUTION_12BIT,
                                       &saadc_adv_config,
                                       saadc_event_handler); // If NULL it will work in blocking mode
    if (err != NRFX_SUCCESS)
    {
        printk("ADC; advanced config error\r\n");
        return -1;
    }

    /* Buffer 1 */
    err = nrfx_saadc_buffer_set(saadc_sample_buffer[0], SAADC_BUFFER_SIZE);
    if (err != NRFX_SUCCESS)
    {
        printk("ADC; buffer set error\r\n");
        return -1;
    }

    /* Buffer 2 */
    err = nrfx_saadc_buffer_set(saadc_sample_buffer[1], SAADC_BUFFER_SIZE);
    if (err != NRFX_SUCCESS)
    {
        printk("ADC; buffer set error\r\n");
        return -1;
    }

    /* Set a buffer for sampling triggered later through PPI */
    err = nrfx_saadc_mode_trigger();
    if (err != NRFX_SUCCESS)
    {
        printk("ADC; ADC trigger error\r\n");
        return -1;
    }

    return 0;
}

static int configure_ppi(void)
{
    nrfx_err_t err;
    /* Declare variables used to hold the (D)PPI channel number */
    uint8_t m_saadc_sample_ppi_channel;
    uint8_t m_saadc_start_ppi_channel;

    /* Trigger ADC SAMPLE TASK */
    err = nrfx_gppi_channel_alloc(&m_saadc_sample_ppi_channel);
    if (err != NRFX_SUCCESS)
    {
        printk("ADC; task trigger error\r\n");
        return -1;
    }

    /* Trigger ADC START task */
    err = nrfx_gppi_channel_alloc(&m_saadc_start_ppi_channel);
    if (err != NRFX_SUCCESS)
    {
        printk("ADC; task trigger error\r\n");
        return -1;
    }

    /* Trigger task sample from timer */
    nrfx_gppi_channel_endpoints_setup(m_saadc_sample_ppi_channel,
                                      nrfx_timer_compare_event_address_get(&timer_instance, NRF_TIMER_CC_CHANNEL0), // Set COMPARE0 timer event as trigger
                                      nrf_saadc_task_address_get(NRF_SAADC, NRF_SAADC_TASK_SAMPLE));                // Set SAMPLE ADC task as action

    /* Trigger task start from end event */
    nrfx_gppi_channel_endpoints_setup(m_saadc_start_ppi_channel,
                                      nrf_saadc_event_address_get(NRF_SAADC, NRF_SAADC_EVENT_END),  // Set END ADC event as trigger (the current buffer is full, BUFF_REQ event is triggerd)
                                      nrf_saadc_task_address_get(NRF_SAADC, NRF_SAADC_TASK_START)); // Set START ADC task as action (start sampling again)

    /* Enable both (D)PPI channels */
    nrfx_gppi_channels_enable(BIT(m_saadc_sample_ppi_channel));
    nrfx_gppi_channels_enable(BIT(m_saadc_start_ppi_channel));

    return 0;
}