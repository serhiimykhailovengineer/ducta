#ifndef DUCTA_UTILS_LOG_HPP
#define DUCTA_UTILS_LOG_HPP

#include <iostream>

#include "ducta/Core/Algorithms/Format.hpp"
#include "ducta/Core/Types/String.hpp"

#define DUCTA_LOG_INFO(fmt, args...)                                                               \
do {                                                                                               \
        String<100> msg;                                                                           \
        (void) format_to(msg, "[ INFO ] {}(): Line {}: " fmt "\n", __func__, __LINE__, ##args);    \
} while (false)

#define DUCTA_LOG_WARNING(fmt, args...)                                                            \
do {                                                                                               \
        String<100> msg;                                                                           \
        (void) format_to(msg, "[ WARNING ] {}(): Line {}: " fmt "\n", __func__, __LINE__, ##args); \
} while (false)

#define DUCTA_LOG_ERROR(fmt, args...)                                                            \
do {                                                                                             \
        String<100> msg;                                                                         \
        (void) format_to(msg, "[ ERROR ] {}(): Line {}: " fmt "\n", __func__, __LINE__, ##args); \
} while (false)

#endif // DUCTA_UTILS_LOG_HPP