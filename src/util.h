#ifndef SPLATT_UTIL_H
#define SPLATT_UTIL_H


/******************************************************************************
 * INCLUDES
 *****************************************************************************/
#include "base.h"



/******************************************************************************
 * PUBLIC FUNCTIONS
 *****************************************************************************/

static inline val_t vmin(
  val_t const v1,
  val_t const v2)
{
  return v1 < v2 ? v1 : v2;
}

static inline val_t vmax(
  val_t const v1,
  val_t const v2)
{
  return v1 > v2 ? v1 : v2;
}

/**
* @brief Generate a random val_t in the range [0, 1].
*
* @return A pseudo-random val_t.
*/
val_t rand_val(void);


/**
* @brief Generate a random idx_t in the range [0, RAND_MAX << 16].
*
* @return A pseudo-random idx_t.
*/
idx_t rand_idx(void);


/**
* @brief Return a string describing a human-readable number of bytes.
*
* @param bytes The number of bytes to describe
*
* @return The human-readable string. NOTE: this string needs to be freed!
*/
char * bytes_str(
  idx_t const bytes);

#endif
