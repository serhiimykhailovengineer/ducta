#ifndef DUCTA_CORE_CONFIG_HPP
#define DUCTA_CORE_CONFIG_HPP

// ========== declaration specifier for non-template definitions in .inl files ==========
// Header-only (default): definitions are inline so every TU may include them.
// DUCTA_SEPARATE_COMPILATION: definitions are compiled once in a single source file.
#if defined(DUCTA_SEPARATE_COMPILATION)
#define DUCTA_DECL
#else
#define DUCTA_DECL inline
#endif

#endif // DUCTA_CORE_CONFIG_HPP
