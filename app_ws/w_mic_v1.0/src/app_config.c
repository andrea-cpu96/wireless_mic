#include "app_config.h"

K_SEM_DEFINE(adc_sem, 0, 1);
K_SEM_DEFINE(i2s_sem, 0, 1);

/** @brief Slab memory structure
 *
 * A slab memory structure organizes data into blocks
 * of memory with the same size and aligned.
 *
 * The memory is then accessed per block (not single data)
 * This means;
 * 1) More blocks that are smaller → higher granularity, lower audio delay,
 *    but increased overhead due to more frequent accesses to different blocks.
 * 2) Fewer blocks that are bigger → lower overhead (fewer memory accesses,
 *    potentially reducing distortion), but increased audio delay since more
 *    time is needed to wait for a block to become available for new data.
 *
 * The main advantages of this data structure are;
 * 1) Deterministic memory access to the data
 * 2) No memory fragmentation of the data
 */
K_MEM_SLAB_DEFINE(tx_0_mem_slab, BLOCK_SIZE, NUM_BLOCKS, 4);