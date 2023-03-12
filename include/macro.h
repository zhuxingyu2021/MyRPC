#ifndef MYRPC_MACRO_H
#define MYRPC_MACRO_H

/**
 * @note: 实现enum2string功能
 * 有两种定义enum类型的方式：
 *
 * 一、
 * ENUM_DEF(ExampleEnum, a, b, c, d)
 *
 * 上述宏会被展开为：
 * enum ExampleEnum {
 *  a, b, c, d
 * };
 * inline const char *ToString(ExampleEnum _enum_var) {
 *     switch (_enum_var) {
 *         case a:
 *             return "a";
 *         case b:
 *             return "b";
 *         case c:
 *             return "c";
 *         case d:
 *             return "d";
 *         default:
 *             return "";
 *     }
 * }
 *
 * 二、
 * ENUM_DEF_2(ExampleEnum, (a,1) , (b,2) , (c,3) , (d,4))
 *
 * 上述宏会被展开为：
 * enum ExampleEnum {
 *     a = 1, b = 2, c = 3, d = 4
 * };
 * inline const char *ToString(ExampleEnum _enum_var) {
 *     ... //省略
 * }
 */

#define MACRO_TO_STRING(arg) MACRO_TO_STRING_(arg)
#define MACRO_TO_STRING_(arg) #arg

#define MACRO_CONCAT(arg1, arg2) MACRO_CONCAT_(arg1, arg2)
#define MACRO_CONCAT_(arg1, arg2) arg1##arg2

// 实现MACRO_GET_NARG宏，用于统计变长参数宏中参数的个数
#define MACRO_GET_NARG(...) MACRO_GET_NARG_(__VA_ARGS__, MACRO_GET_NARG_RSEQ)
#define MACRO_GET_NARG_(...) MACRO_GET_ARG_N(__VA_ARGS__)
#define MACRO_GET_NARG_RSEQ 32,31,30,29,28,27,26,25,24,23,22,21,20,19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0
#define MACRO_GET_ARG_N(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24, \
_25,_26,_27,_28,_29,_30,_31,_32, N, ...) N

// 实现MACRO_WRAP宏，用法如下：
// #define WRAP(x) (x)
// MACRO_WRAP(WRAP,1,2,3,4)
// 上述宏会被展开为： (1)(2)(3)(4)

#define MACRO_WRAP_0(_wrap)
#define MACRO_WRAP_1(_wrap, x, ...) _wrap(x)
#define MACRO_WRAP_2(_wrap, x, ...) _wrap(x) MACRO_WRAP_1(_wrap, __VA_ARGS__)
#define MACRO_WRAP_3(_wrap, x, ...) _wrap(x) MACRO_WRAP_2(_wrap, __VA_ARGS__)
#define MACRO_WRAP_4(_wrap, x, ...) _wrap(x) MACRO_WRAP_3(_wrap, __VA_ARGS__)
#define MACRO_WRAP_5(_wrap, x, ...) _wrap(x) MACRO_WRAP_4(_wrap, __VA_ARGS__)
#define MACRO_WRAP_6(_wrap, x, ...) _wrap(x) MACRO_WRAP_5(_wrap, __VA_ARGS__)
#define MACRO_WRAP_7(_wrap, x, ...) _wrap(x) MACRO_WRAP_6(_wrap, __VA_ARGS__)
#define MACRO_WRAP_8(_wrap, x, ...) _wrap(x) MACRO_WRAP_7(_wrap, __VA_ARGS__)
#define MACRO_WRAP_9(_wrap, x, ...) _wrap(x) MACRO_WRAP_8(_wrap, __VA_ARGS__)
#define MACRO_WRAP_10(_wrap, x, ...) _wrap(x) MACRO_WRAP_9(_wrap, __VA_ARGS__)
#define MACRO_WRAP_11(_wrap, x, ...) _wrap(x) MACRO_WRAP_10(_wrap, __VA_ARGS__)
#define MACRO_WRAP_12(_wrap, x, ...) _wrap(x) MACRO_WRAP_11(_wrap, __VA_ARGS__)
#define MACRO_WRAP_13(_wrap, x, ...) _wrap(x) MACRO_WRAP_12(_wrap, __VA_ARGS__)
#define MACRO_WRAP_14(_wrap, x, ...) _wrap(x) MACRO_WRAP_13(_wrap, __VA_ARGS__)
#define MACRO_WRAP_15(_wrap, x, ...) _wrap(x) MACRO_WRAP_14(_wrap, __VA_ARGS__)
#define MACRO_WRAP_16(_wrap, x, ...) _wrap(x) MACRO_WRAP_15(_wrap, __VA_ARGS__)
#define MACRO_WRAP_17(_wrap, x, ...) _wrap(x) MACRO_WRAP_16(_wrap, __VA_ARGS__)
#define MACRO_WRAP_18(_wrap, x, ...) _wrap(x) MACRO_WRAP_17(_wrap, __VA_ARGS__)
#define MACRO_WRAP_19(_wrap, x, ...) _wrap(x) MACRO_WRAP_18(_wrap, __VA_ARGS__)
#define MACRO_WRAP_20(_wrap, x, ...) _wrap(x) MACRO_WRAP_19(_wrap, __VA_ARGS__)
#define MACRO_WRAP_21(_wrap, x, ...) _wrap(x) MACRO_WRAP_20(_wrap, __VA_ARGS__)
#define MACRO_WRAP_22(_wrap, x, ...) _wrap(x) MACRO_WRAP_21(_wrap, __VA_ARGS__)
#define MACRO_WRAP_23(_wrap, x, ...) _wrap(x) MACRO_WRAP_22(_wrap, __VA_ARGS__)
#define MACRO_WRAP_24(_wrap, x, ...) _wrap(x) MACRO_WRAP_23(_wrap, __VA_ARGS__)
#define MACRO_WRAP_25(_wrap, x, ...) _wrap(x) MACRO_WRAP_24(_wrap, __VA_ARGS__)
#define MACRO_WRAP_26(_wrap, x, ...) _wrap(x) MACRO_WRAP_25(_wrap, __VA_ARGS__)
#define MACRO_WRAP_27(_wrap, x, ...) _wrap(x) MACRO_WRAP_26(_wrap, __VA_ARGS__)
#define MACRO_WRAP_28(_wrap, x, ...) _wrap(x) MACRO_WRAP_27(_wrap, __VA_ARGS__)
#define MACRO_WRAP_29(_wrap, x, ...) _wrap(x) MACRO_WRAP_28(_wrap, __VA_ARGS__)
#define MACRO_WRAP_30(_wrap, x, ...) _wrap(x) MACRO_WRAP_29(_wrap, __VA_ARGS__)
#define MACRO_WRAP_31(_wrap, x, ...) _wrap(x) MACRO_WRAP_30(_wrap, __VA_ARGS__)
#define MACRO_WRAP_32(_wrap, x, ...) _wrap(x) MACRO_WRAP_31(_wrap, __VA_ARGS__)

#define MACRO_WRAP_(N, _wrap, ...) MACRO_CONCAT(MACRO_WRAP_,N)(_wrap, __VA_ARGS__)
#define MACRO_WRAP(_wrap, ...) MACRO_WRAP_(MACRO_GET_NARG(__VA_ARGS__), _wrap, __VA_ARGS__)


#define _ENUM_BEG(name) enum name{
#define _ENUM_LIST(...) __VA_ARGS__
#define _ENUM_END };

#define _ENUM_TO_STRING_BEG(name) inline const char* ToString(name _enum_var){ \
    switch(_enum_var){

#define _ENUM_TO_STRING_CASE(enum_value) case enum_value: return #enum_value;

#define _ENUM_TO_STRING_END default: return "";}}

// 第一种ENUM_DEF宏的实现
#define ENUM_DEF(name, ...) _ENUM_BEG(name) \
_ENUM_LIST(__VA_ARGS__) \
_ENUM_END                                   \
_ENUM_TO_STRING_BEG(name)                   \
MACRO_WRAP(_ENUM_TO_STRING_CASE, __VA_ARGS__)\
_ENUM_TO_STRING_END


// 提取括号中的元素
#define MACRO_PAIR_SUBSTRACT_1(arg) MACRO_PAIR_SUBSTRACT_1_ arg
#define MACRO_PAIR_SUBSTRACT_2(arg) MACRO_PAIR_SUBSTRACT_2_ arg
#define MACRO_PAIR_SUBSTRACT_1_(arg1, arg2) arg1
#define MACRO_PAIR_SUBSTRACT_2_(arg1, arg2) arg2
#define MACRO_PAIR_SUBSTRACT_1_WITH_COMMA(arg) MACRO_PAIR_SUBSTRACT_1(arg),


#define MACRO_WRAP2_0(_wrap,_wrap2)
#define MACRO_WRAP2_1(_wrap,_wrap2, x, ...) _wrap2(x)
#define MACRO_WRAP2_2(_wrap,_wrap2, x, ...) _wrap(x) MACRO_WRAP2_1(_wrap, _wrap2, __VA_ARGS__)
#define MACRO_WRAP2_3(_wrap,_wrap2, x, ...) _wrap(x) MACRO_WRAP2_2(_wrap, _wrap2, __VA_ARGS__)
#define MACRO_WRAP2_4(_wrap,_wrap2, x, ...) _wrap(x) MACRO_WRAP2_3(_wrap, _wrap2, __VA_ARGS__)
#define MACRO_WRAP2_5(_wrap,_wrap2, x, ...) _wrap(x) MACRO_WRAP2_4(_wrap, _wrap2, __VA_ARGS__)
#define MACRO_WRAP2_6(_wrap,_wrap2, x, ...) _wrap(x) MACRO_WRAP2_5(_wrap, _wrap2, __VA_ARGS__)
#define MACRO_WRAP2_7(_wrap,_wrap2, x, ...) _wrap(x) MACRO_WRAP2_6(_wrap, _wrap2, __VA_ARGS__)
#define MACRO_WRAP2_8(_wrap,_wrap2, x, ...) _wrap(x) MACRO_WRAP2_7(_wrap, _wrap2, __VA_ARGS__)
#define MACRO_WRAP2_9(_wrap,_wrap2, x, ...) _wrap(x) MACRO_WRAP2_8(_wrap, _wrap2, __VA_ARGS__)
#define MACRO_WRAP2_10(_wrap,_wrap2, x, ...) _wrap(x) MACRO_WRAP2_9(_wrap, _wrap2, __VA_ARGS__)
#define MACRO_WRAP2_11(_wrap,_wrap2, x, ...) _wrap(x) MACRO_WRAP2_10(_wrap, _wrap2, __VA_ARGS__)
#define MACRO_WRAP2_12(_wrap,_wrap2, x, ...) _wrap(x) MACRO_WRAP2_11(_wrap, _wrap2, __VA_ARGS__)
#define MACRO_WRAP2_13(_wrap,_wrap2, x, ...) _wrap(x) MACRO_WRAP2_12(_wrap, _wrap2, __VA_ARGS__)
#define MACRO_WRAP2_14(_wrap,_wrap2, x, ...) _wrap(x) MACRO_WRAP2_13(_wrap, _wrap2, __VA_ARGS__)
#define MACRO_WRAP2_15(_wrap,_wrap2, x, ...) _wrap(x) MACRO_WRAP2_14(_wrap, _wrap2, __VA_ARGS__)
#define MACRO_WRAP2_16(_wrap,_wrap2, x, ...) _wrap(x) MACRO_WRAP2_15(_wrap, _wrap2, __VA_ARGS__)
#define MACRO_WRAP2_17(_wrap,_wrap2, x, ...) _wrap(x) MACRO_WRAP2_16(_wrap, _wrap2, __VA_ARGS__)
#define MACRO_WRAP2_18(_wrap,_wrap2, x, ...) _wrap(x) MACRO_WRAP2_17(_wrap, _wrap2, __VA_ARGS__)
#define MACRO_WRAP2_19(_wrap,_wrap2, x, ...) _wrap(x) MACRO_WRAP2_18(_wrap, _wrap2, __VA_ARGS__)
#define MACRO_WRAP2_20(_wrap,_wrap2, x, ...) _wrap(x) MACRO_WRAP2_19(_wrap, _wrap2, __VA_ARGS__)
#define MACRO_WRAP2_21(_wrap,_wrap2, x, ...) _wrap(x) MACRO_WRAP2_20(_wrap, _wrap2, __VA_ARGS__)
#define MACRO_WRAP2_22(_wrap,_wrap2, x, ...) _wrap(x) MACRO_WRAP2_21(_wrap, _wrap2, __VA_ARGS__)
#define MACRO_WRAP2_23(_wrap,_wrap2, x, ...) _wrap(x) MACRO_WRAP2_22(_wrap, _wrap2, __VA_ARGS__)
#define MACRO_WRAP2_24(_wrap,_wrap2, x, ...) _wrap(x) MACRO_WRAP2_23(_wrap, _wrap2, __VA_ARGS__)
#define MACRO_WRAP2_25(_wrap,_wrap2, x, ...) _wrap(x) MACRO_WRAP2_24(_wrap, _wrap2, __VA_ARGS__)
#define MACRO_WRAP2_26(_wrap,_wrap2, x, ...) _wrap(x) MACRO_WRAP2_25(_wrap, _wrap2, __VA_ARGS__)
#define MACRO_WRAP2_27(_wrap,_wrap2, x, ...) _wrap(x) MACRO_WRAP2_26(_wrap, _wrap2, __VA_ARGS__)
#define MACRO_WRAP2_28(_wrap,_wrap2, x, ...) _wrap(x) MACRO_WRAP2_27(_wrap, _wrap2, __VA_ARGS__)
#define MACRO_WRAP2_29(_wrap,_wrap2, x, ...) _wrap(x) MACRO_WRAP2_28(_wrap, _wrap2, __VA_ARGS__)
#define MACRO_WRAP2_30(_wrap,_wrap2, x, ...) _wrap(x) MACRO_WRAP2_29(_wrap, _wrap2, __VA_ARGS__)
#define MACRO_WRAP2_31(_wrap,_wrap2, x, ...) _wrap(x) MACRO_WRAP2_30(_wrap, _wrap2, __VA_ARGS__)
#define MACRO_WRAP2_32(_wrap,_wrap2, x, ...) _wrap(x) MACRO_WRAP2_31(_wrap, _wrap2, __VA_ARGS__)


#define MACRO_WRAP2_(N, _wrap, _wrap2, ...) MACRO_CONCAT(MACRO_WRAP2_,N)(_wrap, _wrap2, __VA_ARGS__)
#define MACRO_WRAP2(_wrap, _wrap2, ...) MACRO_WRAP2_(MACRO_GET_NARG(__VA_ARGS__), _wrap, _wrap2, __VA_ARGS__)


#define _ENUM_EQ(x) MACRO_PAIR_SUBSTRACT_1(x) = MACRO_PAIR_SUBSTRACT_2(x)
#define _ENUM_EQ_WITH_COMMA(x) MACRO_PAIR_SUBSTRACT_1(x) = MACRO_PAIR_SUBSTRACT_2(x),
#define _ENUM_LIST_2(...) MACRO_WRAP2(_ENUM_EQ_WITH_COMMA, _ENUM_EQ, __VA_ARGS__)

#define _ENUM_LIST_ENUM_NAME(...) MACRO_WRAP2(MACRO_PAIR_SUBSTRACT_1_WITH_COMMA,MACRO_PAIR_SUBSTRACT_1, __VA_ARGS__)

#define _ENUM_TO_STRING_CASE_2(enum_value) case enum_value: return MACRO_TO_STRING(enum_value);

// 第二种ENUM_DEF宏的实现
#define ENUM_DEF_2(name, ...) _ENUM_BEG(name) \
_ENUM_LIST_2(__VA_ARGS__) \
_ENUM_END                                   \
_ENUM_TO_STRING_BEG(name)                   \
MACRO_WRAP(_ENUM_TO_STRING_CASE_2, _ENUM_LIST_ENUM_NAME(__VA_ARGS__))\
_ENUM_TO_STRING_END

#endif //MYRPC_MACRO_H
