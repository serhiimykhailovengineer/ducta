#ifndef DUCTA_CORE_PREPROCESSOR_HPP
#define DUCTA_CORE_PREPROCESSOR_HPP

#include "ducta/Core/Config.hpp"

// ========== basic concatenation ==========
#define DUCTA_PP_CAT(a, b) DUCTA_PP_CAT_I(a, b)
#define DUCTA_PP_CAT_I(a, b) a##b

// ========== remove optional parentheses (for NodeType with commas) ==========
#define DUCTA_PP_REMOVE_PARENS(...) __VA_ARGS__
#define DUCTA_PP_UNPAREN(x) DUCTA_PP_REMOVE_PARENS x

// ========== argument counting (up to 25) ==========
#define DUCTA_PP_NARG(...) DUCTA_PP_NARG_I(__VA_ARGS__, DUCTA_PP_RSEQ_N())
#define DUCTA_PP_NARG_I(...) DUCTA_PP_ARG_N(__VA_ARGS__)
#define DUCTA_PP_ARG_N( \
    _1,                 \
    _2,                 \
    _3,                 \
    _4,                 \
    _5,                 \
    _6,                 \
    _7,                 \
    _8,                 \
    _9,                 \
    _10,                \
    _11,                \
    _12,                \
    _13,                \
    _14,                \
    _15,                \
    _16,                \
    _17,                \
    _18,                \
    _19,                \
    _20,                \
    _21,                \
    _22,                \
    _23,                \
    _24,                \
    _25,                \
    N,                  \
    ...)                \
    N
#define DUCTA_PP_RSEQ_N() \
    25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0

// ========== comma if i > 0 (0..25) ==========
#define DUCTA_COMMA_IF_0
#define DUCTA_COMMA_IF_1 ,
#define DUCTA_COMMA_IF_2 ,
#define DUCTA_COMMA_IF_3 ,
#define DUCTA_COMMA_IF_4 ,
#define DUCTA_COMMA_IF_5 ,
#define DUCTA_COMMA_IF_6 ,
#define DUCTA_COMMA_IF_7 ,
#define DUCTA_COMMA_IF_8 ,
#define DUCTA_COMMA_IF_9 ,
#define DUCTA_COMMA_IF_10 ,
#define DUCTA_COMMA_IF_11 ,
#define DUCTA_COMMA_IF_12 ,
#define DUCTA_COMMA_IF_13 ,
#define DUCTA_COMMA_IF_14 ,
#define DUCTA_COMMA_IF_15 ,
#define DUCTA_COMMA_IF_16 ,
#define DUCTA_COMMA_IF_17 ,
#define DUCTA_COMMA_IF_18 ,
#define DUCTA_COMMA_IF_19 ,
#define DUCTA_COMMA_IF_20 ,
#define DUCTA_COMMA_IF_21 ,
#define DUCTA_COMMA_IF_22 ,
#define DUCTA_COMMA_IF_23 ,
#define DUCTA_COMMA_IF_24 ,
#define DUCTA_COMMA_IF_25 ,
#define DUCTA_COMMA_IF(i) DUCTA_PP_CAT(DUCTA_COMMA_IF_, i)

// ========== tuple elem for 2-tuple: ("left", left) ==========
#define DUCTA_TUPLE_ELEM_2_0(a, b) a
#define DUCTA_TUPLE_ELEM_2_1(a, b) b

// ========== tuple elem for 3-tuple: ("gain", config.gain > 0, "must be positive") ==========
#define DUCTA_TUPLE_ELEM_3_0(a, b, c) a
#define DUCTA_TUPLE_ELEM_3_1(a, b, c) b
#define DUCTA_TUPLE_ELEM_3_2(a, b, c) c
#define DUCTA_TUPLE_ELEM(n, i, tuple) \
    DUCTA_PP_CAT(DUCTA_PP_CAT(DUCTA_TUPLE_ELEM_, n), DUCTA_PP_CAT(_, i)) tuple

// ========== FOR_EACH_I over variadics (up to 25) ==========
#define DUCTA_FOR_EACH_I(M, node, ...) \
    DUCTA_PP_CAT(DUCTA_FOR_EACH_I_, DUCTA_PP_NARG(__VA_ARGS__))(M, node, __VA_ARGS__)

#define DUCTA_FOR_EACH_I_1(M, node, a1) M(node, 0, a1)

#define DUCTA_FOR_EACH_I_2(M, node, a1, a2) M(node, 0, a1) M(node, 1, a2)

#define DUCTA_FOR_EACH_I_3(M, node, a1, a2, a3) M(node, 0, a1) M(node, 1, a2) M(node, 2, a3)

#define DUCTA_FOR_EACH_I_4(M, node, a1, a2, a3, a4) \
    M(node, 0, a1) M(node, 1, a2) M(node, 2, a3) M(node, 3, a4)

#define DUCTA_FOR_EACH_I_5(M, node, a1, a2, a3, a4, a5) \
    M(node, 0, a1) M(node, 1, a2) M(node, 2, a3) M(node, 3, a4) M(node, 4, a5)

#define DUCTA_FOR_EACH_I_6(M, node, a1, a2, a3, a4, a5, a6) \
    M(node, 0, a1) M(node, 1, a2) M(node, 2, a3) M(node, 3, a4) M(node, 4, a5) M(node, 5, a6)

#define DUCTA_FOR_EACH_I_7(M, node, a1, a2, a3, a4, a5, a6, a7) \
    M(node, 0, a1)                                              \
    M(node, 1, a2) M(node, 2, a3) M(node, 3, a4) M(node, 4, a5) M(node, 5, a6) M(node, 6, a7)

#define DUCTA_FOR_EACH_I_8(M, node, a1, a2, a3, a4, a5, a6, a7, a8) \
    M(node, 0, a1)                                                  \
    M(node, 1, a2)                                                  \
    M(node, 2, a3) M(node, 3, a4) M(node, 4, a5) M(node, 5, a6) M(node, 6, a7) M(node, 7, a8)

#define DUCTA_FOR_EACH_I_9(M, node, a1, a2, a3, a4, a5, a6, a7, a8, a9) \
    M(node, 0, a1)                                                      \
    M(node, 1, a2)                                                      \
    M(node, 2, a3)                                                      \
    M(node, 3, a4) M(node, 4, a5) M(node, 5, a6) M(node, 6, a7) M(node, 7, a8) M(node, 8, a9)

#define DUCTA_FOR_EACH_I_10(M, node, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10) \
    M(node, 0, a1)                                                            \
    M(node, 1, a2)                                                            \
    M(node, 2, a3)                                                            \
    M(node, 3, a4)                                                            \
    M(node, 4, a5) M(node, 5, a6) M(node, 6, a7) M(node, 7, a8) M(node, 8, a9) M(node, 9, a10)

#define DUCTA_FOR_EACH_I_11(M, node, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11) \
    DUCTA_FOR_EACH_I_10(M, node, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10) M(node, 10, a11)

#define DUCTA_FOR_EACH_I_12(M, node, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12) \
    DUCTA_FOR_EACH_I_11(M, node, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11) M(node, 11, a12)

#define DUCTA_FOR_EACH_I_13(M, node, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13) \
    DUCTA_FOR_EACH_I_12(M, node, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12) M(node, 12, a13)

#define DUCTA_FOR_EACH_I_14(M, node, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14) \
    DUCTA_FOR_EACH_I_13(M, node, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13) M(node, 13, a14)

#define DUCTA_FOR_EACH_I_15(M, node, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15) \
    DUCTA_FOR_EACH_I_14(M, node, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14) M(node, 14, a15)

#define DUCTA_FOR_EACH_I_16(M, node, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16) \
    DUCTA_FOR_EACH_I_15(M, node, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15) M(node, 15, a16)

#define DUCTA_FOR_EACH_I_17(M, node, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17) \
    DUCTA_FOR_EACH_I_16(M, node, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16) M(node, 16, a17)

#define DUCTA_FOR_EACH_I_18(M, node, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18) \
    DUCTA_FOR_EACH_I_17(M, node, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17) M(node, 17, a18)

#define DUCTA_FOR_EACH_I_19(M, node, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19) \
    DUCTA_FOR_EACH_I_18(M, node, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18) M(node, 18, a19)

#define DUCTA_FOR_EACH_I_20(M, node, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20) \
    DUCTA_FOR_EACH_I_19(M, node, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19) M(node, 19, a20)

#define DUCTA_FOR_EACH_I_21(M, node, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21) \
    DUCTA_FOR_EACH_I_20(M, node, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20) M(node, 20, a21)

#define DUCTA_FOR_EACH_I_22(M, node, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22) \
    DUCTA_FOR_EACH_I_21(M, node, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21) M(node, 21, a22)

#define DUCTA_FOR_EACH_I_23(M, node, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23) \
    DUCTA_FOR_EACH_I_22(M, node, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22) M(node, 22, a23)

#define DUCTA_FOR_EACH_I_24(M, node, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24) \
    DUCTA_FOR_EACH_I_23(M, node, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23) M(node, 23, a24)

#define DUCTA_FOR_EACH_I_25(M, node, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25) \
    DUCTA_FOR_EACH_I_24(M, node, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24) M(node, 24, a25)

#endif // DUCTA_CORE_PREPROCESSOR_HPP