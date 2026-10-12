#ifndef DUCTA_CONFIGURATION_JSON_HPP
#define DUCTA_CONFIGURATION_JSON_HPP

#include "ducta/Configuration/Diagnostics.hpp"
#include "ducta/Configuration/PropTree.hpp"

#include "ducta/Core/Config.hpp"
#include "ducta/Core/Types/Integers.hpp"
#include "ducta/Core/Types/StringView.hpp"

// Capacity of the embedded backend: JSON tokens of one document (values and keys)
#ifndef DUCTA_CONFIG_MAX_JSON_TOKENS
#define DUCTA_CONFIG_MAX_JSON_TOKENS (2 * DUCTA_CONFIG_MAX_NODES)
#endif

namespace ducta {
namespace Configuration {

struct JsonOptions
{
    /* Maximum nesting of objects / arrays (bounds the recursion of the loader) */
    size_t max_depth{32};
};

/*
 * @brief Load JSON text into tree, replacing its content (the source name is kept)
 *
 * The top-level value must be an object. Every value keeps the line / column where it starts
 * (1-based, columns count UTF-8 characters). Problems are reported to diagnostics: a syntax
 * error stops loading, other problems (duplicate keys, out of range numbers, values that do
 * not fit) are all collected. On failure the tree content is incomplete.
 *
 * @note Numbers with a fraction or an exponent are parsed with strtod, which assumes the "C"
 *       locale. Integers must fit into int64_t.
 * @return true if the whole text was loaded without problems
 */
bool loadJson(StringView text, PropTree& tree, Diagnostics& diagnostics, JsonOptions options = {});

#if !defined(DUCTA_USE_EMBEDDED_BACKEND)
/* Read the file at path, make path the source name of tree and load it (standard backend only) */
bool loadJsonFile(StringView path, PropTree& tree, Diagnostics& diagnostics, JsonOptions options = {});
#endif

} // namespace Configuration
} // namespace ducta

#if !defined(DUCTA_SEPARATE_COMPILATION)
#include "ducta/Configuration/Private/Json.inl"
#endif

#endif // DUCTA_CONFIGURATION_JSON_HPP
