#include <math.h>

#include "util.h"

size_t get_bin_index(size_t size) {
    for (size_t index = 0U; index < BINS; index++){
	if (size <= (16 * pow(2, index))) return index;
    }

    return BINS;
}
