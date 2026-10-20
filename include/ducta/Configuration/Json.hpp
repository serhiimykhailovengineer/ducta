#ifndef DUCTA_CONFIGURATION_JSON_HPP
#define DUCTA_CONFIGURATION_JSON_HPP

#include "ducta/Core/Capacity.hpp"

#include "ducta/Configuration/Diagnostics.hpp"
#include "ducta/Configuration/PropTree.hpp"

#include "ducta/Core/Config.hpp"
#include "ducta/Core/Types/Integers.hpp"
#include "ducta/Core/Types/StringView.hpp"

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
 * @note Numbers with a fraction or an exponent are parsed without the C library: exactly when
 *       they have at most 15 significant digits and a decimal exponent within +-22 (e.g. 0.1,
 *       2.5, 1e-3); others are passed to strtod ("C" locale) by the standard backend and
 *       approximated within 3 units in the last place by the embedded backend.
 *       Integers must fit into int64_t.
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
