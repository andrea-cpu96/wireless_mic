#include "bt1036c_drv.h"

#define TXRX_MODULE BT103036C_CONFIG_TX // (0, TX) ; (1, RX)
#define ENABLE_DSP_FILTER false
#define ENABLE_STEREO_DIFF true
#define ENABLE_SIGNAL_GEN false

/* Audio defines */
#define AMP_FACTOR 3 // NOTE; I2S data are 32 bit in size, only 24 lower bit are valid, but bt module considers only 16 higher bit in a 32 bit data
#define MAX_LIMIT (INT32_MAX / (pow(2, AMP_FACTOR)))
#define MIN_LIMIT (INT32_MIN / (pow(2, AMP_FACTOR)))
