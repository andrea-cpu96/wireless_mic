	int err;
	/* Connect ADC interrupt to nrfx interrupt handler */
	IRQ_CONNECT(DT_IRQN(DT_NODELABEL(adc)),			  // Extract irq number
				DT_IRQ(DT_NODELABEL(adc), priority),  // Extract irq priority
				nrfx_isr, nrfx_saadc_irq_handler, 0); // Connect the interrupt to the nrf interrupt handler

	err = nrfx_saadc_init(DT_IRQ(DT_NODELABEL(adc), priority));
	if (err != NRFX_SUCCESS)
	{
		printk("ADC; init error\r\n");
		return -1;
	}