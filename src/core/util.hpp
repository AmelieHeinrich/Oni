//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-09-14 09:29:51
//

#pragma once

#include <cstdint>

namespace util
{
    uint64_t hash(const void *key, uint32_t len, uint64_t seed);
}
