/**
 * @file audio.h
 * @author Andrea Fato
 * @date 2025-06-03
 * @brief Provides a library audio applicaitons.
 *
 * @copyright (c) 2025 Andrea Fato. Tutti i diritti riservati.
 */

#ifndef AUDIO
#define AUDIO

void audio_process(void);

int i2s_config(void);
int i2s_tone(void);

int adc_config(void);

#endif /* AUDIO */