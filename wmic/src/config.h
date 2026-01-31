#include "bt1036c_drv.h"

#define TXRX_MODULE BT103036C_CONFIG_TX // (0, TX) ; (1, RX)
#define ENABLE_DSP_FILTER false
#define ENABLE_STEREO_DIFF flase
#define ENABLE_SIGNAL_GEN true

/* Audio defines */
#define AMP_FACTOR 8 // NOTE; I2S data are 32 bit in size, only 24 lower bit are valid, but bt module considers only 16 higher bit in a 32 bit data, so set a shift of 8 bit
#define MAX_LIMIT (INT32_MAX / (pow(2, AMP_FACTOR)))
#define MIN_LIMIT (INT32_MIN / (pow(2, AMP_FACTOR)))
