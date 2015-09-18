/* cx_function.h
 *
 * This file contains generated code. Do not modify!
 */

#ifndef corto_lang_function_H
#define corto_lang_function_H

#include "corto.h"
#ifdef corto_lang_LIB
#include "cx__type.h"
#include "cx__api.h"
#include "cx__meta.h"
#else
#include "corto/lang/cx__type.h"
#include "corto/lang/cx__api.h"
#include "corto/lang/cx__meta.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* ::corto::lang::function::bind() */
cx_int16 _cx_function_bind(cx_function _this);
#define cx_function_bind(_this) _cx_function_bind(cx_function(_this))

/* ::corto::lang::function::init() */
cx_int16 _cx_function_init(cx_function _this);
#define cx_function_init(_this) _cx_function_init(cx_function(_this))

/* ::corto::lang::function::stringToParameterSeq(string name,object scope) */
cx_parameterseq _cx_function_stringToParameterSeq(cx_string name, cx_object scope);
#define cx_function_stringToParameterSeq(name, scope) _cx_function_stringToParameterSeq(name, scope)

/* ::corto::lang::function::unbind(function object) */
cx_void _cx_function_unbind(cx_function object);
#define cx_function_unbind(object) _cx_function_unbind(cx_function(object))

#ifdef __cplusplus
}
#endif
#endif
