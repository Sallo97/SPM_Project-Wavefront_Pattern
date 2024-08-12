/**
* @file constants.h
 * @brief  contains all the constants and aliases
 *         used by the various implementations.
 * @author Salvatore Salerno
 */

#ifndef CONSTANTS_H
#define CONSTANTS_H

using u64 = std::uint64_t;
using u8 = std::uint8_t;
constexpr u64 default_length = 1 << 12;  // it is read as "2^12"
constexpr u8 default_workers = 8;
constexpr u64 default_chunk_size = 512;


#endif //CONSTANTS_H
