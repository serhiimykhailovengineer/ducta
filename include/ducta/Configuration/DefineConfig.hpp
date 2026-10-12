/**
 * @file DefineConfig.hpp
 * @brief Macros to define how a plain configuration struct is decoded and validated
 */

#ifndef DUCTA_CONFIGURATION_DEFINE_CONFIG_HPP
#define DUCTA_CONFIGURATION_DEFINE_CONFIG_HPP

#include "ducta/Configuration/Decode.hpp"
#include "ducta/Configuration/Diagnostics.hpp"
#include "ducta/Configuration/PropTree.hpp"

#include "ducta/Core/Types/StringView.hpp"

#include "ducta/Core/Preprocessor.hpp"

#define DUCTA_CONFIG_FIELD_DECODE(config, i, elem) \
    decoder.field(DUCTA_TUPLE_ELEM(2, 0, elem), (config).DUCTA_TUPLE_ELEM(2, 1, elem));

#define DUCTA_CONFIG_FIELD_NAME(config, i, elem) \
    DUCTA_COMMA_IF(i)::ducta::StringView         \
    {                                            \
        DUCTA_TUPLE_ELEM(2, 0, elem)             \
    }

// Main macro to define configuration fields of a struct:
//   DEFINE_CONFIG(GainConfig, ("gain", gain), ("name", name));
// Missing fields keep their default value, unknown keys are reported as errors.
#define DEFINE_CONFIG(ConfigType, ...)                                                 \
    template <>                                                                        \
    struct ducta::Configuration::ConfigTraits<DUCTA_PP_UNPAREN((ConfigType))>          \
    {                                                                                  \
        using Config = DUCTA_PP_UNPAREN((ConfigType));                                 \
        static bool decode(                                                            \
            ::ducta::Configuration::PropNode node,                                     \
            Config& config,                                                            \
            ::ducta::Configuration::Diagnostics& diagnostics,                          \
            ::ducta::StringView ignored = {})                                          \
        {                                                                              \
            ::ducta::Configuration::Private::ObjectDecoder decoder{node, diagnostics}; \
            if (!decoder.valid())                                                      \
            {                                                                          \
                return false;                                                          \
            }                                                                          \
            DUCTA_FOR_EACH_I(DUCTA_CONFIG_FIELD_DECODE, config, __VA_ARGS__)           \
            ::ducta::StringView const names[] = {                                      \
                DUCTA_FOR_EACH_I(DUCTA_CONFIG_FIELD_NAME, config, __VA_ARGS__)};       \
            decoder.reject_unknown(names, ignored);                                    \
            return decoder.ok();                                                       \
        }                                                                              \
    };

#define DUCTA_CONFIG_RULE_CHECK(config, i, elem)         \
    validator.check(                                     \
        static_cast<bool>(DUCTA_TUPLE_ELEM(3, 1, elem)), \
        DUCTA_TUPLE_ELEM(3, 0, elem),                    \
        DUCTA_TUPLE_ELEM(3, 2, elem));

// Macro to define semantic rules of a configuration struct, checked after decoding:
//   DEFINE_CONFIG_RULES(GainConfig,
//       ("gain", config.gain > 0.0, "must be positive"),
//       ("name", !config.name.empty(), "must not be empty"));
// Each rule is (field, condition, message): the condition may use `config` (the decoded
// struct) and `validator` (e.g. validator.has("name")); wrap it in parentheses if it
// contains commas. A failed rule is reported at the field's location in the source.
#define DEFINE_CONFIG_RULES(ConfigType, ...)                                                     \
    template <>                                                                                  \
    struct ducta::Configuration::ConfigRules<DUCTA_PP_UNPAREN((ConfigType))>                     \
    {                                                                                            \
        using Config = DUCTA_PP_UNPAREN((ConfigType));                                           \
        static void validate(Config const& config, ::ducta::Configuration::Validator& validator) \
        {                                                                                        \
            (void)config;                                                                        \
            DUCTA_FOR_EACH_I(DUCTA_CONFIG_RULE_CHECK, config, __VA_ARGS__)                       \
        }                                                                                        \
    };

#define DUCTA_CONFIG_VARIANT_CASE(variant, i, elem)                                 \
    if (decoder.tag() == ::ducta::StringView{DUCTA_TUPLE_ELEM(2, 0, elem)})         \
    {                                                                               \
        return decoder.template alternative<DUCTA_TUPLE_ELEM(2, 1, elem)>(variant); \
    }

#define DUCTA_CONFIG_VARIANT_NAME(variant, i, elem) \
    DUCTA_COMMA_IF(i)::ducta::StringView            \
    {                                               \
        DUCTA_TUPLE_ELEM(2, 0, elem)                \
    }

// Macro to decode a Variant of DEFINE_CONFIG structs selected by a tag member:
//   using FilterConfig = ducta::Variant<LowPassConfig, HighPassConfig>;
//   DEFINE_CONFIG_VARIANT(FilterConfig, "type", ("lowpass", LowPassConfig), ("highpass", HighPassConfig));
// decodes { "type": "lowpass", "cutoff": 100 } as LowPassConfig (the tag is not a field of
// the alternative). Variants without this macro are decoded by the kind of the value.
#define DEFINE_CONFIG_VARIANT(VariantType, TagKey, ...)                            \
    template <>                                                                    \
    struct ducta::Configuration::ConfigTraits<DUCTA_PP_UNPAREN((VariantType))>     \
    {                                                                              \
        using Config = DUCTA_PP_UNPAREN((VariantType));                            \
        static bool decode(                                                        \
            ::ducta::Configuration::PropNode node,                                 \
            Config& config,                                                        \
            ::ducta::Configuration::Diagnostics& diagnostics,                      \
            ::ducta::StringView ignored = {})                                      \
        {                                                                          \
            (void)ignored;                                                         \
            ::ducta::Configuration::Private::TaggedVariantDecoder decoder{         \
                node, ::ducta::StringView{TagKey}, diagnostics};                   \
            if (!decoder.valid())                                                  \
            {                                                                      \
                return false;                                                      \
            }                                                                      \
            DUCTA_FOR_EACH_I(DUCTA_CONFIG_VARIANT_CASE, config, __VA_ARGS__)       \
            ::ducta::StringView const names[] = {                                  \
                DUCTA_FOR_EACH_I(DUCTA_CONFIG_VARIANT_NAME, config, __VA_ARGS__)}; \
            return decoder.reject_unknown_tag(names);                              \
        }                                                                          \
    };

#endif // DUCTA_CONFIGURATION_DEFINE_CONFIG_HPP
