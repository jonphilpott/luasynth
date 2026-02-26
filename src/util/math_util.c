/**
 * math_util.c — DSP Math Utilities implementation
 */

#include <math.h>
#include "math_util.h"

float db_to_linear(float db) {
    return powf(10.0f, db / 20.0f);
}
