#include <stdint.h>

void veeprom_init(void);
int veeprom_write(const void *data, int size);
void veeprom_read(uint32_t address, void *data, int size);