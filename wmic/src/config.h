#define I2S_DEBUG 0

/* Audio defines */
#define AMP_FACTOR 3
#define MAX_LIMIT (INT32_MAX / (pow(2, AMP_FACTOR)))
#define MIN_LIMIT (INT32_MIN / (pow(2, AMP_FACTOR)))
