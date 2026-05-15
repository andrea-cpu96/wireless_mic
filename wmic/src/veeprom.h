#include <stdint.h>

void veeprom_init(void);
int veeprom_write(const int16_t *data, int size);
void veeprom_read(int address, int32_t *data, int size);