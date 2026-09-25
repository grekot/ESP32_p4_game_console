#include "engine/storage.h"

#include "engine/rng.h"
#include "platform/platform.h"

namespace engine {

bool load_data(const char* key, void* data, size_t size)
{
    if (deterministic()) return false;
    return platform::load_blob(key, data, size);
}

bool save_data(const char* key, const void* data, size_t size)
{
    if (deterministic()) return false;
    return platform::save_blob(key, data, size);
}

void erase_data(const char* key)
{
    if (deterministic()) return;
    platform::erase_blob(key);
}

const char* const RECORD_KEYS[] = { "snake_top" };
const int         RECORD_KEY_COUNT = (int)(sizeof(RECORD_KEYS) / sizeof(RECORD_KEYS[0]));

}  // namespace engine
