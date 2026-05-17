#include <stdint.h>

void veeprom_init(void);
int veeprom_write(const void *data, int size);
void veeprom_read(int address, void *data, int size);