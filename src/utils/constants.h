/**
* @file constants.h
 * @brief  contains all the constants and aliases
 *         used by the various implementations.
 * @author Salvatore Salerno
 */

#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <iostream>

using u16 = std::uint64_t;
using u8 = std::uint8_t;
constexpr u16 default_length = 1 << 14;  // it is read as "2^14"
constexpr u8 default_workers = 4;
constexpr u8 default_chunk_size = 64;


#endif //CONSTANTS_H
