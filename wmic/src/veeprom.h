#include <stdint.h>

void storage_init(void);
void storage_write(uint8_t id, const int32_t *data, int size);
void storage_read(uint8_t id, int32_t *data, int size);