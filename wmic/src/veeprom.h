#include <stdint.h>

void veeprom_init(void);
int veeprom_write(const int32_t *data, int size);
void veeprom_read(uint8_t id, int32_t *data, int size);
