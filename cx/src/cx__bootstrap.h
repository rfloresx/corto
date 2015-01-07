/*
 * cx_bootstrap.h
 *
 *  Created on: Aug 2, 2012
 *      Author: sander
 */

#ifndef CX_BOOTSTRAP_H_
#define CX_BOOTSTRAP_H_

#include "cx__object.h"
#include "cx__equals.h"
#include "cx_util.h"

#include "cx.h"

/* Notes about the typesystem
 *
 * Composite objects
 *  Composite objects describe aggregations of information. There are 5 classes that make up for the
 *  interface framework. Each of these classes implements a distinct and orthogonal piece of functionality.
 *  Here follows a short description of each one:
 *
 *   - Interface
 *      This class is the base-interface type. An interface can hold methods and members. It is not possible
 *      to directly instantiate an interface. Interfaces must be implemented by classes. Interfaces can
 *      inherit from other interfaces.
 *
 *   - Struct (base = Interface)
 *      The struct class is derived from interface, and has non-reference semantics. The struct-class
 *      introduces the ability to apply a modifier (PRIVATE,LOCAL,READONLY,..) to its base. 
 * 
 *   - Class (base = Struct)
 *      Classes inherit the behavior from struct but are of a reference type. They add the capability to
 *      define a constructor, destructor and template observables.
 *
 *   - Procedure (base = Struct)
 *      The procedure class describes objects that implement functionality. There are five
 *      procedure-types in the type-system, each with unique characteristics:
 *
 *      - Function
 *          The function is the most basic procedure. It has a returntype, parameterlist and an
 *          implementation that can be provided in various languages. All other procedure-types
 *          inherit from function. Functions with a returntype introduce a variable with the
 *          name of the function, which allows the implementation to assign a returnvalue for
 *          the function.
 *
 *      - Method (base = function)
 *          A method is procedure that is always defined in an interface type. A method can be invoked on
 *          an instantiated object from any type derived from interface. Methods can be virtual
 *          when they are declared in an struct parent, in which case they can be re-implemented
 *          by a subtype. Methods introduce a 'this' variable, and in case of an struct parent,
 *          which inherits from a base-class, a 'super' alias for 'this' with the type of the base-class.
 *
 *      - Delegate (base = function)
 *          Delegates provide a type-safe manner of specifying callback routines. They result in a
 *          callback-vtable on the instantiated object. This table is populated by callback objects
 *          which must have a compatible signature with the delegate definition. Delegates can only
 *          be declared in class-objects.
 *
 *      - Callback (base = function)
 *          Callbacks are used to provide implementations for delegates. Their signature must match
 *          that of the delegate object. Callbacks may be declared within the scope of any object
 *          for which it's type is a class and has delegates. A callback is always annotated with the
 *          delegate it implements.
 *
 *       - Observer (base = function)
 *          Observers are procedures that are executed whenever an event occurs in the system. They
 *          are therefore not called in a traditional fashion and as such, the argumentlist of an
 *          observer is usually hidden for the user. Observers have access to the source of the event
 *          and the observable, through resp. the 'source' variable and the 'observable' variable.
 *
 *
 */

/* Bootstrap curiosities:
 *  1. The virtual callback table is usually allocated along with the memory of an object, and
 *     is placed after the object's value. This vtable is free'd along with the memory of the object.
 *     Since at creation of an object it is known how many delegates it's type has, the vtable for
 *     the worst case can be allocated at object creation.
 *     For SSO objects, the vtable must be of a static size however. Therefore all vtables of SSO's
 *     are of size 16, which is more than the maximum number of delegates for any builtin-class.
 *
 *  2. All static objects are assigned an initial refcount of 2. This prevents static objects being
 *     cleaned up using the regular path, which would result in trying to free static memory and
 *     bootstrap issues at shutdown (wouldn't be able to free class, because it's a class etc.).
 *     The database shutdown takes care of correct cleanup of builtin objects.
 *
 *  3. The 'object' type has a bootstrap issue because it's scope is an 'object'. Static objects are checked on
 *     refcounts when they are free'd, but 'object' is still referenced by it's scope and therefore
 *     would report a refcount other than expected. 'object' can't be removed after the scopes
 *     however, because that would result in a 'scope not empty' error during the removal of the scope.
 *     Therefore an exception is made in the freeSSO function, to not check object. object is checked
 *     manually in the bootstrap.
 *
 *  4. During destruction, types set their typedef.real pointer to NULL. When destructing the builtin-types,
 *     this prevents from referring to the real type, because there is no 'right' order in which objects can be
 *     destructed without this issue occurring. To counter this issue, the 'real' pointer is restored for
 *     builtin-types after destruction.
 *
 *  5. The delegate-callback mechanism is used by type initializers and class constructors\destructors, which
 *     are in turn required by the bootstrap to initialize and define types. However, initially, the corresponding
 *     delegates are not yet defined, thus no callbacks can be binded to them, causing a major pain during
 *     bootstrap - it would be necessary to first initialize and define all callbacks before anything other could
 *     happen. As a relaxation, callbacks are allowed to call their constructors (transparently, without this being
 *     visible in the object-state) during bootstrap-initialization, because all information is then already available
 *     during initialization. This localized solution keeps the bootstrap smal(ler) and easier to comprehend.
 *
 *  6. The unbind callbacks are not resolved using a regular delegate-lookup function, but instead are directly
 *     forwarded to the cx_function_unbind function. Because procedures are destructed after the destruction of
 *     all classes, the vtable of cx_procedure_o does no longer exist, so the callback lookup would crash.
 *     Because lang::function is the only object with an unbind this is a safe workaround. Consequence is that
 *     end-users won't be able to use the procedure::unbind delegate, which is acceptable.
 *
 *  7. The virtual method 'interface::bindMethod' is required to bind methods to classes and interfaces. However,
 *     there is a bootstrap issue here (who binds bindMethod) which cannot be easily solved by a hard-coded
 *     bind of bindMethod. Since virtual methods are looked-up by methodId's, and methodId's are only stable
 *     after the parent of a method is defined, bindMethod can only be looked up after both 'interface' and
 *     'class' are defined. To get defined though, these classes need to bind all their methods, hence the
 *     bootstrap. This issue is solved by calling the method::init initializer call the implementations of the
 *     virtual method 'interface::bindMethod' directly, so no methodId lookup is required.
 *     For delegate functions this is the same, but here the implementation is easier, since the delegate::init
 *     function does not need to distinct between interface's and classes. Delegates can only be defined on a class.
 */

/* Implementations of virtual functions */
extern cx_object cx_cortex_new(cx_typedef type);
extern cx_object cx_cortex__new(cx_typedef type, cx_attr attributes);

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_UINT8  ((int8_t)-1)
#define MAX_UINT16 ((int16_t)-1)
#define MAX_UINT32 ((int32_t)-1)
#define MAX_UINT64 ((int64_t)-1)
#define MAX_INT8  ((MAX_UINT8/2-1))
#define MAX_INT16 ((MAX_UINT16/2-1))
#define MAX_INT32 ((MAX_UINT32/2-1))
#define MAX_INT64 ((MAX_UINT64/2-1))
#define MIN_INT8  (-(MAX_UINT8/2+1))
#define MIN_INT16 (-(MAX_UINT16/2+1))
#define MIN_INT32 (-(MAX_UINT32/2+1))
#define MIN_INT64 (-(MAX_UINT64/2+1))

#define CX_STATIC_SCOPED_OBJECT(type)\
typedef struct sso_##type {\
    cx_SSO o;\
    cx_##type v;\
    struct { /* vtable */\
        cx_uint32 length;\
        void* buffer;\
    }vtable;\
    cx_object vbuff[16];\
}sso_##type

#define CX_STATIC_SCOPED_REFOBJECT(type)\
typedef struct sso_##type {\
    cx_SSO o;\
    struct cx_##type##_s v;\
    struct { /* vtable */\
        cx_uint32 length;\
        void* buffer;\
    }vtable;\
    cx_object vbuff[16];\
}sso_##type

/* Static Scoped Object */
typedef struct cx_SSO {
    cx__scope s;
    cx__object o;
}cx_SSO;

/* Static Scoped Observable Object */
typedef struct cx_SSOO {
    cx__observable v;
    cx__scope s;
    cx__object o;
}cx_SSOO;

/* Static Scoped Observable Object (used for scopes) */
typedef struct cx_SSOO_object {
    cx_SSOO o;
}cx_SSOO_object;

CX_STATIC_SCOPED_REFOBJECT(typedef);
CX_STATIC_SCOPED_REFOBJECT(type);
CX_STATIC_SCOPED_REFOBJECT(primitive);
CX_STATIC_SCOPED_REFOBJECT(collection);
CX_STATIC_SCOPED_REFOBJECT(procedure);
CX_STATIC_SCOPED_REFOBJECT(binary);
CX_STATIC_SCOPED_REFOBJECT(boolean);
CX_STATIC_SCOPED_REFOBJECT(character);
CX_STATIC_SCOPED_REFOBJECT(int);
CX_STATIC_SCOPED_REFOBJECT(uint);
CX_STATIC_SCOPED_REFOBJECT(float);
CX_STATIC_SCOPED_REFOBJECT(text);
CX_STATIC_SCOPED_REFOBJECT(enum);
CX_STATIC_SCOPED_REFOBJECT(bitmask);
CX_STATIC_SCOPED_REFOBJECT(interface);
CX_STATIC_SCOPED_REFOBJECT(struct);
CX_STATIC_SCOPED_REFOBJECT(class);
CX_STATIC_SCOPED_REFOBJECT(array);
CX_STATIC_SCOPED_REFOBJECT(sequence);
CX_STATIC_SCOPED_REFOBJECT(list);
CX_STATIC_SCOPED_REFOBJECT(map);
CX_STATIC_SCOPED_REFOBJECT(function);
CX_STATIC_SCOPED_REFOBJECT(method);
CX_STATIC_SCOPED_REFOBJECT(delegate);
CX_STATIC_SCOPED_REFOBJECT(callback);
CX_STATIC_SCOPED_REFOBJECT(metaprocedure);
CX_STATIC_SCOPED_REFOBJECT(member);
CX_STATIC_SCOPED_OBJECT(parameter);

CX_STATIC_SCOPED_OBJECT(constant);

#define VTABLE_V {16,NULL},{NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL}
#define CX_FWDECL_SSO(type, name) sso_##type name##__o
#define CX_FWDECL(type, name) CX_FWDECL_SSO(type, name); cx_##type cx_##name##_o = &name##__o.v

/* SSO */
#define CX_ATTR_SSOO {1,0,1,CX_VALID | CX_DECLARED}
#define CX_ATTR_SSO {1,0,0,CX_VALID | CX_DECLARED}
#define CX_ATTR_SO {0,0,0,CX_VALID | CX_DECLARED}
#define CX_ROOT_V() {{NULL,NULL,CX_RWMUTEX_INITIALIZER,CX_RWMUTEX_INITIALIZER,NULL,NULL,FALSE,FALSE,FALSE,NULL},{NULL, NULL, _(scope)NULL, _(scopeLock)CX_RWMUTEX_INITIALIZER, _(attached)NULL, _(orphaned)0},{CX_ATTR_SSOO, 2, (cx_typedef)&object__o.v, CX_MMNODE_INIT}}
#define CX_OBJECT_V(parent, name) {{NULL,NULL,CX_RWMUTEX_INITIALIZER,CX_RWMUTEX_INITIALIZER,NULL,NULL,FALSE,FALSE,FALSE,NULL},{CX_OFFSET(&parent##__o, sizeof(cx_SSOO)), name, _(scope)NULL, _(scopeLock)CX_RWMUTEX_INITIALIZER, _(attached)NULL, _(orphaned)0},{CX_ATTR_SSOO, 2, (cx_typedef)&object__o.v, CX_MMNODE_INIT}}
#define CX_SSO_V(parent, name, type) {{CX_OFFSET(&parent##__o, sizeof(cx_SSOO)), name, _(scope)NULL, _(scopeLock)CX_RWMUTEX_INITIALIZER, _(attached)NULL, _(orphaned)0},{CX_ATTR_SSO, 2, (cx_typedef)&type##__o.v, CX_MMNODE_INIT}}
#define CX_SSO_PO_V(parent, name, type) {{CX_OFFSET(&parent##__o, sizeof(cx_SSO)), name, _(scope)NULL, _(scopeLock)CX_RWMUTEX_INITIALIZER, _(attached)NULL, _(orphaned)0},{CX_ATTR_SSO, 2, (cx_typedef)&type##__o.v, CX_MMNODE_INIT}}

/* SSO identifier */
#define CX_SSO_TYPE_ID(name) (cx_typedef)&name##__o.v

/* typedef */
#define CX_TYPEDEF_V(name) {CX_OFFSET(&name##__o, sizeof(cx_SSO)),CX_OFFSET(&name##__o, sizeof(cx_SSO))}

/* type */
#define CX_TYPE_V(name, kind, reference, scopeType, scopeTypeKind) {CX_TYPEDEF_V(name), kind, reference, FALSE, 0, 0, 0, NULL, scopeType, scopeTypeKind,{0,NULL}}

/* primitive */
#define CX_PRIMITIVE_V(name, kind, width, scopeType, scopeStateKind) {CX_TYPE_V(name, CX_PRIMITIVE, FALSE, scopeType, scopeStateKind), kind, width, 0}

/* interface */
#define CX_COMPOSITE_V(name, kind, base, reference, scopeType, scopeStateKind) {CX_TYPE_V(name, CX_COMPOSITE, reference, scopeType, scopeStateKind), kind, 0, {0, NULL}, {0,NULL}, cx_interface(&base##__o.v)}

/* interface */
#define CX_COMPOSITE_NOBASE_V(name, kind, reference, scopeType, scopeStateKind) {CX_TYPE_V(name, CX_COMPOSITE, reference, scopeType, scopeStateKind), kind, 0, {0, NULL}, {0,NULL}, NULL}

/* struct */
#define CX_STRUCT_V(name, kind, base, baseAccess, reference, scopeType, scopeStateKind) \
    {CX_COMPOSITE_V(name, kind, base, reference, scopeType, scopeStateKind), baseAccess, 0}

#define CX_STRUCT_NOBASE_V(name, kind, reference, scopeType, scopeStateKind) \
    {CX_COMPOSITE_NOBASE_V(name, kind, reference, scopeType, scopeStateKind), CX_LOCAL, 0}

/* collection */
#define CX_COLLECTION_V(name, kind, elementType, max) {CX_TYPE_V(name, CX_COLLECTION, FALSE, NULL, CX_DECLARED | CX_DEFINED), kind, (cx_typedef)&elementType##__o.v, max}

/* sequence */
#define CX_SEQUENCE_V(subType, length, ...) {length, (subType[]){__VA_ARGS__}}
#define CX_SEQUENCE_EMPTY_V(subType) {0, NULL}

/* member */
#define CX_MEMBER_V(type, access, state, weak) {(cx_typedef)&type##__o.v, access, state, weak, 0, 0}

/* object */
#define CX_OBJECT_O(name) cx_SSOO_object name##__o = {CX_OBJECT_V(root, #name)}
#define CX_OBJECT_O_SCOPE(parent, name) cx_SSOO_object parent##_##name##__o = {CX_OBJECT_V(parent, #name)}

/* type object */
#define CX_TYPE_O(name, kind, reference) static sso_type name##__o = {CX_SSO_V(cortex_lang, #name, type), CX_TYPE_V(name, kind, reference, NULL, CX_DECLARED | CX_DEFINED), VTABLE_V}

/* primitive object */
#define CX_PRIMITIVE_O(name, kind, width) sso_primitive name##__o = {CX_SSO_V(cortex_lang, #name, primitive), CX_PRIMITIVE_V(name, kind, width), VTABLE_V}

/* binary object */
#define CX_BINARY_O(name, width) sso_binary name##__o = {CX_SSO_V(cortex_lang, #name, binary), {CX_PRIMITIVE_V(name, CX_BINARY, width, NULL, CX_DECLARED | CX_DEFINED)}, VTABLE_V}

/* boolean object */
#define CX_BOOLEAN_O(name) sso_boolean name##__o = {CX_SSO_V(cortex_lang, #name, boolean), {CX_PRIMITIVE_V(name, CX_BOOLEAN, CX_WIDTH_8, NULL, CX_DECLARED | CX_DEFINED)}, VTABLE_V}

/* character object */
#define CX_CHARACTER_O(name, width) sso_character name##__o = {CX_SSO_V(cortex_lang, #name, character), {CX_PRIMITIVE_V(name, CX_CHARACTER, width, NULL, CX_DECLARED | CX_DEFINED)}, VTABLE_V}

/* int object */
#define CX_INT_O(name, width, min, max, scopeType, scopeStateKind) sso_int name##__o = {CX_SSO_V(cortex_lang, #name, int), {CX_PRIMITIVE_V(name, CX_INTEGER, width, scopeType, scopeStateKind), min, max}, VTABLE_V}

/* uint object */
#define CX_UINT_O(name, width, min, max) sso_uint name##__o = {CX_SSO_V(cortex_lang, #name, uint), {CX_PRIMITIVE_V(name, CX_UINTEGER, width, NULL, CX_DECLARED | CX_DEFINED), min, max}, VTABLE_V}

/* float object */
#define CX_FLOAT_O(name, width, min, max) sso_float name##__o = {CX_SSO_V(cortex_lang, #name, float), {CX_PRIMITIVE_V(name, CX_FLOAT, width, NULL, CX_DECLARED | CX_DEFINED), min, max}, VTABLE_V}

/* text object */
#define CX_TEXT_O(name, width, length) sso_text name##__o = {CX_SSO_V(cortex_lang, #name, text), {CX_PRIMITIVE_V(name, CX_TEXT, CX_WIDTH_WORD, NULL, CX_DECLARED | CX_DEFINED), width, length}, VTABLE_V}

/* enum object */
#define CX_ENUM_O(name) sso_enum name##__o = {CX_SSO_V(cortex_lang, #name, enum), {CX_PRIMITIVE_V(name, CX_ENUM, CX_WIDTH_32, NULL, CX_DECLARED | CX_DEFINED), CX_SEQUENCE_EMPTY_V(constant)}, VTABLE_V}

/* bitmask object */
#define CX_BITMASK_O(name) sso_bitmask name##__o = {CX_SSO_V(cortex_lang, #name, bitmask), {{CX_PRIMITIVE_V(name, CX_BITMASK, CX_WIDTH_32, NULL, CX_DECLARED | CX_DEFINED), CX_SEQUENCE_EMPTY_V(constant)}}, VTABLE_V}

/* constant object */
#define CX_CONSTANT_O(parent, name) sso_constant parent##_##name##__o = {CX_SSO_PO_V(parent, #name, constant), CX_##name, VTABLE_V}

/* struct object */
#define CX_STRUCT_O(name, scopeType, scopeStateKind) sso_struct name##__o = \
    {CX_SSO_V(cortex_lang, #name, struct), CX_STRUCT_NOBASE_V(name, CX_STRUCT, FALSE, scopeType, scopeStateKind), VTABLE_V}

/* interface object */
#define CX_INTERFACE_O(name) sso_interface name##__o = \
    {CX_SSO_V(cortex_lang, #name, interface), CX_COMPOSITE_NOBASE_V(name, CX_INTERFACE, TRUE, NULL, 0), VTABLE_V}

/* class object */
#define CX_CLASS_NOBASE_O(name, implements, scopeType, scopeStateKind) sso_class name##__o = \
    {CX_SSO_V(cortex_lang, #name, class), {CX_STRUCT_NOBASE_V(name, CX_CLASS, TRUE, scopeType, scopeStateKind), {0,NULL}, {0,NULL}, {0,NULL}}, VTABLE_V}

#define CX_CLASS_O(name, base, baseAccess, implements, scopeType, scopeStateKind) sso_class name##__o = \
        {CX_SSO_V(cortex_lang, #name, class), {CX_STRUCT_V(name, CX_CLASS, base, baseAccess, TRUE, scopeType, scopeStateKind), implements, {0,NULL}, {0,NULL}}, VTABLE_V}

/* array object */
#define CX_ARRAY_O(name, elementType, size) sso_array name##__o = {CX_SSO_V(cortex_lang, #name, array), {CX_COLLECTION_V(name, CX_ARRAY, elementType, size)}, VTABLE_V}

/* sequence object */
#define CX_SEQUENCE_O(name, elementType, max) sso_sequence name##__o = {CX_SSO_V(cortex_lang, #name, sequence), {CX_COLLECTION_V(name, CX_SEQUENCE, elementType, max)}, VTABLE_V}

/* list object */
#define CX_LIST_O(name, elementType, max) sso_list name##__o = {CX_SSO_V(cortex_lang, #name, list), {CX_COLLECTION_V(name, CX_LIST, elementType, max)}, VTABLE_V}

/* map object */
#define CX_MAP_O(name, elementType, keyType, max) sso_map name##__o = {CX_SSO_V(cortex_lang, #name, map), {CX_COLLECTION_V(name, CX_MAP, elementType, max), (cx_typedef)&keyType##__o.v}, VTABLE_V}

/* procedure object */
#define CX_PROCEDURE_O(name, kind, base, baseAccess, scopeType, scopeStateKind) sso_procedure name##__o = \
        {CX_SSO_V(cortex_lang, #name, procedure), {CX_STRUCT_V(name, CX_PROCEDURE, base, baseAccess, TRUE, scopeType, scopeStateKind), kind}, VTABLE_V}
#define CX_PROCEDURE_NOBASE_O(name, kind, scopeType, scopeStateKind) sso_procedure name##__o = \
        {CX_SSO_V(cortex_lang, #name, procedure), {CX_STRUCT_NOBASE_V(name, CX_PROCEDURE, TRUE, scopeType, scopeStateKind), kind}, VTABLE_V}

/* function object */
#define CX_FUNCTION_O(parent, name, args, returnType, impl) \
        void __##impl(void *f, void *r, void *a); \
        sso_function parent##_##name##__o = {CX_SSO_PO_V(parent, #name args, function), {(cx_typedef)&returnType##__o.v, FALSE, FALSE, CX_PROCEDURE_CDECL, (cx_word)__##impl, (cx_word)impl, NULL, 0, {0,NULL},0}, VTABLE_V}

#define CX_FUNCTION_OO_O(parent, name, args, returnType, impl) \
        void __##impl(void *f, void *r, void *a); \
        sso_function parent##_##name##__o = {CX_SSO_V(parent, #name args, function), {(cx_typedef)&returnType##__o.v, FALSE, FALSE, CX_PROCEDURE_CDECL, (cx_word)__##impl, (cx_word)impl, NULL, 0, {0,NULL},0}, VTABLE_V}

#define CX_FUNCTION_OVERLOAD_OO_O(parent, name, args, returnType, impl) \
        void __##impl(void *f, void *r, void *a); \
        sso_function parent##_##name##__o = {CX_SSO_V(parent, args, function), {(cx_typedef)&returnType##__o.v, FALSE, FALSE, CX_PROCEDURE_CDECL, (cx_word)__##impl, (cx_word)impl, NULL, 0, {0,NULL},0}, VTABLE_V}

/* method object */
#define CX_METHOD_O(parent, name, args, returnType, virtual, impl) \
        void __##impl(void *f, void *r, void *a); \
        sso_method parent##_##name##__o = {CX_SSO_PO_V(parent, #name args, method), {{(cx_typedef)&returnType##__o.v, FALSE, FALSE, CX_PROCEDURE_CDECL, (cx_word)__##impl, (cx_word)impl, NULL, 0,{0,NULL},0}, virtual}, VTABLE_V}

/* interface method object */
#define CX_IMETHOD_O(parent, name, args, returnType, virtual) \
        sso_method parent##_##name##__o = {CX_SSO_PO_V(parent, #name args, method), {{(cx_typedef)&returnType##__o.v, FALSE, FALSE, CX_PROCEDURE_CDECL, 0, 0, NULL, 0,{0,NULL},0}, virtual}, VTABLE_V}

/* delegate object */
#define CX_DELEGATE_O(parent, name, args, returnType) \
        sso_delegate parent##_##name##__o = {CX_SSO_PO_V(parent, #name args, delegate), {{(cx_typedef)&returnType##__o.v, FALSE, FALSE, CX_PROCEDURE_CDECL, 0, 0, NULL, 0, {0,NULL},0}, 0}, VTABLE_V}

/* callback object */
#define CX_CALLBACK_O(parent, name, args, delegate, returnType, impl) \
        void __##impl(void *f, void *r, void *a); \
        sso_callback parent##_##name##__o = {CX_SSO_PO_V(parent, #name args, callback), {{(cx_typedef)&returnType##__o.v, FALSE, FALSE, CX_PROCEDURE_CDECL, (cx_word)__##impl, (cx_word)impl, NULL, sizeof(cx_word), {0,NULL},0}, (cx_delegate)&delegate##__o.v}, VTABLE_V}

/* metaprocedure object */
#define CX_METAPROCEDURE_O(parent, name, args, returnType, referenceOnly, impl) \
        void __##impl(void *f, void *r, void *a); \
        sso_metaprocedure parent##_##name##__o = {CX_SSO_PO_V(parent, #name args, metaprocedure), {{(cx_typedef)&returnType##__o.v, FALSE, FALSE, CX_PROCEDURE_CDECL, (cx_word)__##impl, (cx_word)impl, NULL, 0, {0,NULL},0}, referenceOnly}, VTABLE_V}

#define CX_METAPROCEDURE_NAME_O(parent, name, actualName, args, returnType, referenceOnly, impl) \
        void __##impl(void *f, void *r, void *a); \
        sso_metaprocedure parent##_##name##__o = {CX_SSO_PO_V(parent, #actualName args, metaprocedure), {{(cx_typedef)&returnType##__o.v, FALSE, FALSE, CX_PROCEDURE_CDECL, (cx_word)__##impl, (cx_word)impl, NULL, 0, {0,NULL},0}, referenceOnly}, VTABLE_V}
    
/* member object */
#define CX_MEMBER_O(parent, name, type, access) sso_member parent##_##name##__o = {CX_SSO_PO_V(parent, #name, member), CX_MEMBER_V(type, access, CX_DECLARED | CX_DEFINED, FALSE), VTABLE_V}

/* reference object */
#define CX_REFERENCE_O(parent, name, type, access, state, weak) sso_member parent##_##name##__o = {CX_SSO_PO_V(parent, #name, member), CX_MEMBER_V(type, access, state, weak), VTABLE_V}

/* Forward declarations of classes */
CX_FWDECL(class, typedef);
CX_FWDECL(class, type);
CX_FWDECL(class, template);
CX_FWDECL(class, primitive);
CX_FWDECL(class, interface);
CX_FWDECL(class, collection);
CX_FWDECL(class, procedure);
CX_FWDECL(class, binary);
CX_FWDECL(class, boolean);
CX_FWDECL(class, character);
CX_FWDECL(class, int);
CX_FWDECL(class, uint);
CX_FWDECL(class, float);
CX_FWDECL(class, text);
CX_FWDECL(class, enum);
CX_FWDECL(class, bitmask);
CX_FWDECL(class, alias);
CX_FWDECL(class, struct);
CX_FWDECL(class, class);
CX_FWDECL(class, procptr);
CX_FWDECL(class, array);
CX_FWDECL(class, sequence);
CX_FWDECL(class, list);
CX_FWDECL(class, map);
CX_FWDECL(class, member);
CX_FWDECL(class, reference);
CX_FWDECL(class, event);
CX_FWDECL(class, observableEvent);
CX_FWDECL(struct, interfaceVector);
CX_FWDECL(struct, parameter);
CX_FWDECL(struct, procptrdata);

/* Abstract classes */
CX_FWDECL(interface, dispatcher);

CX_FWDECL(binary, octet);
CX_FWDECL(boolean, bool);
CX_FWDECL(character, char);
CX_FWDECL(uint, uint8);
CX_FWDECL(uint, uint16);
CX_FWDECL(uint, uint32);
CX_FWDECL(uint, uint64);
CX_FWDECL(int, int8);
CX_FWDECL(int, int16);
CX_FWDECL(int, int32);
CX_FWDECL(int, int64);
CX_FWDECL(float, float32);
CX_FWDECL(float, float64);
CX_FWDECL(text, string);
CX_FWDECL(binary, word);
CX_FWDECL(int, constant);

static CX_FWDECL(type, any);
static CX_FWDECL(type, void);
static CX_FWDECL(type, object);

CX_FWDECL(procedure, function);
CX_FWDECL(procedure, method);
CX_FWDECL(procedure, virtual);
CX_FWDECL(procedure, delegate);
CX_FWDECL(procedure, callback);
CX_FWDECL(procedure, observer);
CX_FWDECL(procedure, metaprocedure);

CX_FWDECL(enum, width);
CX_FWDECL(enum, typeKind);
CX_FWDECL(enum, primitiveKind);
CX_FWDECL(enum, compositeKind);
CX_FWDECL(enum, collectionKind);
CX_FWDECL(enum, procedureKind);
CX_FWDECL(enum, scopeStateKind);
CX_FWDECL(enum, equalityKind);
CX_FWDECL(enum, operatorKind);
CX_FWDECL(bitmask, state);
CX_FWDECL(bitmask, attr);
CX_FWDECL(bitmask, eventMask);
CX_FWDECL(bitmask, modifier);

CX_FWDECL(sequence, objectSeq);
CX_FWDECL(sequence, interfaceSeq);
CX_FWDECL(sequence, memberSeq);
CX_FWDECL(sequence, parameterSeq);
CX_FWDECL(sequence, observerSeq);
CX_FWDECL(sequence, vtable);
CX_FWDECL(sequence, interfaceVectorSeq);

/* ::cortex::serialization module */
CX_FWDECL(class, serializer);
CX_FWDECL(enum, serializerTraceKind);

/* Forward declare static delegate objects */
CX_FWDECL(delegate, type_init);
CX_FWDECL(delegate, class_construct);
CX_FWDECL(delegate, class_destruct);
CX_FWDECL(delegate, procedure_bind);

/* database root */
cx_SSOO_object root__o = {CX_ROOT_V()};
cx_object root_o = CX_OFFSET(&root__o.o.o, sizeof(cx__object));

/* ::cortex, ::cortex::lang and ::cortex::serialization */
CX_OBJECT_O(cortex);
CX_OBJECT_O_SCOPE(cortex, lang);
CX_OBJECT_O_SCOPE(cortex, serialization);

cx_object cortex_o = CX_OFFSET(&cortex__o.o.o, sizeof(cx__object));
    CX_FUNCTION_OO_O(cortex, new, "(lang::typedef type)", object, cx_cortex_new);
    CX_FUNCTION_OVERLOAD_OO_O(cortex, _new, "new(lang::typedef type,lang::attr attributes)", object, cx_cortex__new);

cx_object cortex_lang_o = CX_OFFSET(&cortex_lang__o.o.o, sizeof(cx__object));

/* Primitives */
CX_BINARY_O(octet, CX_WIDTH_8);
CX_BOOLEAN_O(bool);
CX_CHARACTER_O(char, CX_WIDTH_8);
CX_UINT_O(uint8, CX_WIDTH_8, 0, MAX_UINT8);
CX_UINT_O(uint16, CX_WIDTH_16, 0, MAX_UINT16);
CX_UINT_O(uint32, CX_WIDTH_32, 0, MAX_UINT32);
CX_UINT_O(uint64, CX_WIDTH_64, 0, MAX_UINT64);
CX_INT_O(int8, CX_WIDTH_8, MIN_INT8, MAX_INT8, NULL, CX_DECLARED | CX_DEFINED);
CX_INT_O(int16, CX_WIDTH_16, MIN_INT16, MAX_INT16, NULL, CX_DECLARED | CX_DEFINED);
CX_INT_O(int32, CX_WIDTH_32, MIN_INT32, MAX_INT32, NULL, CX_DECLARED | CX_DEFINED);
CX_INT_O(int64, CX_WIDTH_64, MIN_INT64, MAX_INT64, NULL, CX_DECLARED | CX_DEFINED);
CX_FLOAT_O(float32, CX_WIDTH_32, 0, 0);
CX_FLOAT_O(float64, CX_WIDTH_64, 0, 0);
CX_TEXT_O(string, CX_WIDTH_8, 0);
CX_BINARY_O(word, CX_WIDTH_WORD);
CX_INT_O(constant, CX_WIDTH_32, 0, MAX_INT32, CX_SSO_TYPE_ID(enum), CX_DECLARED);
    CX_CALLBACK_O(constant, init, "(constant& object)", type_init, int16, cx_constant_init);

/* Any type */
CX_TYPE_O(any, CX_ANY, FALSE);

/* Void type */
CX_TYPE_O(void, CX_VOID, FALSE);

/* Object type */
CX_TYPE_O(object, CX_VOID, TRUE);

/* Enumerations */
CX_ENUM_O(width);
    CX_CONSTANT_O(width, WIDTH_8);
    CX_CONSTANT_O(width, WIDTH_16);
    CX_CONSTANT_O(width, WIDTH_32);
    CX_CONSTANT_O(width, WIDTH_64);
    CX_CONSTANT_O(width, WIDTH_WORD);

CX_ENUM_O(typeKind);
    CX_CONSTANT_O(typeKind, VOID);
    CX_CONSTANT_O(typeKind, ANY);
    CX_CONSTANT_O(typeKind, PRIMITIVE);
    CX_CONSTANT_O(typeKind, COMPOSITE);
    CX_CONSTANT_O(typeKind, COLLECTION);

CX_ENUM_O(primitiveKind);
    CX_CONSTANT_O(primitiveKind, BINARY);
    CX_CONSTANT_O(primitiveKind, BOOLEAN);
    CX_CONSTANT_O(primitiveKind, CHARACTER);
    CX_CONSTANT_O(primitiveKind, INTEGER);
    CX_CONSTANT_O(primitiveKind, UINTEGER);
    CX_CONSTANT_O(primitiveKind, FLOAT);
    CX_CONSTANT_O(primitiveKind, TEXT);
    CX_CONSTANT_O(primitiveKind, ENUM);
    CX_CONSTANT_O(primitiveKind, BITMASK);
    CX_CONSTANT_O(primitiveKind, ALIAS);

CX_ENUM_O(compositeKind);
    CX_CONSTANT_O(compositeKind, STRUCT);
    CX_CONSTANT_O(compositeKind, INTERFACE);
    CX_CONSTANT_O(compositeKind, CLASS);
    CX_CONSTANT_O(compositeKind, PROCPTR);
    CX_CONSTANT_O(compositeKind, PROCEDURE);

CX_ENUM_O(collectionKind);
    CX_CONSTANT_O(collectionKind, ARRAY);
    CX_CONSTANT_O(collectionKind, SEQUENCE);
    CX_CONSTANT_O(collectionKind, LIST);
    CX_CONSTANT_O(collectionKind, MAP);

CX_ENUM_O(procedureKind);
    CX_CONSTANT_O(procedureKind, FUNCTION);
    CX_CONSTANT_O(procedureKind, METHOD);
    CX_CONSTANT_O(procedureKind, DELEGATE);
    CX_CONSTANT_O(procedureKind, CALLBACK);
    CX_CONSTANT_O(procedureKind, OBSERVER);
    CX_CONSTANT_O(procedureKind, METAPROCEDURE);

CX_ENUM_O(equalityKind);
    CX_CONSTANT_O(equalityKind, EQ);
    CX_CONSTANT_O(equalityKind, LT);
    CX_CONSTANT_O(equalityKind, GT);
    CX_CONSTANT_O(equalityKind, NEQ);

CX_ENUM_O(operatorKind);
    CX_CONSTANT_O(operatorKind, ASSIGN);
    CX_CONSTANT_O(operatorKind, ADD);
    CX_CONSTANT_O(operatorKind, SUB);
    CX_CONSTANT_O(operatorKind, MUL);
    CX_CONSTANT_O(operatorKind, DIV);
    CX_CONSTANT_O(operatorKind, MOD);
    CX_CONSTANT_O(operatorKind, INC);
    CX_CONSTANT_O(operatorKind, DEC);
    CX_CONSTANT_O(operatorKind, XOR);
    CX_CONSTANT_O(operatorKind, OR);
    CX_CONSTANT_O(operatorKind, AND);
    CX_CONSTANT_O(operatorKind, NOT);
    CX_CONSTANT_O(operatorKind, ASSIGN_ADD);
    CX_CONSTANT_O(operatorKind, ASSIGN_SUB);
    CX_CONSTANT_O(operatorKind, ASSIGN_MUL);
    CX_CONSTANT_O(operatorKind, ASSIGN_DIV);
    CX_CONSTANT_O(operatorKind, ASSIGN_MOD);
    CX_CONSTANT_O(operatorKind, ASSIGN_XOR);
    CX_CONSTANT_O(operatorKind, ASSIGN_OR);
    CX_CONSTANT_O(operatorKind, ASSIGN_AND);
    CX_CONSTANT_O(operatorKind, COND_OR);
    CX_CONSTANT_O(operatorKind, COND_AND);
    CX_CONSTANT_O(operatorKind, COND_NOT);
    CX_CONSTANT_O(operatorKind, COND_EQ);
    CX_CONSTANT_O(operatorKind, COND_NEQ);
    CX_CONSTANT_O(operatorKind, COND_GT);
    CX_CONSTANT_O(operatorKind, COND_LT);
    CX_CONSTANT_O(operatorKind, COND_GTEQ);
    CX_CONSTANT_O(operatorKind, COND_LTEQ);
    CX_CONSTANT_O(operatorKind, SHIFT_LEFT);
    CX_CONSTANT_O(operatorKind, SHIFT_RIGHT);
    CX_CONSTANT_O(operatorKind, COMMA);
    CX_CONSTANT_O(operatorKind, REF);
    CX_CONSTANT_O(operatorKind, MEMBER_RESOLVE);
    CX_CONSTANT_O(operatorKind, ELEMENT_OPEN);
    CX_CONSTANT_O(operatorKind, ELEMENT_CLOSE);
    CX_CONSTANT_O(operatorKind, BRACKET_OPEN);
    CX_CONSTANT_O(operatorKind, BRACKET_CLOSE);

CX_BITMASK_O(state);
    CX_CONSTANT_O(state, VALID);
    CX_CONSTANT_O(state, DECLARED);
    CX_CONSTANT_O(state, DEFINED);

CX_BITMASK_O(attr);
    CX_CONSTANT_O(attr, ATTR_SCOPED);
    CX_CONSTANT_O(attr, ATTR_WRITABLE);
    CX_CONSTANT_O(attr, ATTR_OBSERVABLE);

CX_BITMASK_O(eventMask);
    CX_CONSTANT_O(eventMask, ON_DECLARE);
    CX_CONSTANT_O(eventMask, ON_DEFINE);
    CX_CONSTANT_O(eventMask, ON_DESTRUCT);
    CX_CONSTANT_O(eventMask, ON_INVALIDATE);
    CX_CONSTANT_O(eventMask, ON_UPDATE);
    CX_CONSTANT_O(eventMask, ON_SELF);
    CX_CONSTANT_O(eventMask, ON_SCOPE);
    CX_CONSTANT_O(eventMask, ON_VALUE);
    CX_CONSTANT_O(eventMask, ON_METAVALUE);

CX_BITMASK_O(modifier);
    CX_CONSTANT_O(modifier, GLOBAL);
    CX_CONSTANT_O(modifier, LOCAL);
    CX_CONSTANT_O(modifier, PRIVATE);
    CX_CONSTANT_O(modifier, READONLY);
    CX_CONSTANT_O(modifier, CONST);

/* Collections */
CX_SEQUENCE_O(objectSeq, object, 0);
CX_SEQUENCE_O(interfaceSeq, interface, 0);
CX_SEQUENCE_O(memberSeq, member, 0);
CX_SEQUENCE_O(parameterSeq, parameter, 0);
CX_SEQUENCE_O(observerSeq, observer, 0);
CX_SEQUENCE_O(vtable, function, 0);
CX_SEQUENCE_O(interfaceVectorSeq, interfaceVector, 0);

/* ::cortex::lang::typedef */
CX_CLASS_NOBASE_O(typedef, CX_SEQUENCE_EMPTY_V(interface), NULL, CX_DECLARED | CX_DEFINED);
    CX_REFERENCE_O(typedef, type, typedef, CX_GLOBAL, CX_DEFINED, FALSE);
    CX_REFERENCE_O(typedef, real, type, CX_LOCAL, CX_DEFINED, FALSE);
    CX_METHOD_O(typedef, realType, "()", type, FALSE, cx_typedef_realType);
    CX_CALLBACK_O(typedef, init, "(lang::typedef object)", type_init, int16, cx_typedef_init);
    CX_CALLBACK_O(typedef, construct, "(lang::typedef object)", class_construct, int16, cx_typedef_construct);
    CX_CALLBACK_O(typedef, destruct, "(lang::typedef object)", class_destruct, void, cx_typedef_destruct);

/* ::cortex::lang::type */
CX_CLASS_O(type, typedef, CX_LOCAL | CX_READONLY, CX_SEQUENCE_EMPTY_V(interface), NULL, CX_DECLARED | CX_DEFINED);
    CX_MEMBER_O(type, kind, typeKind, CX_READONLY | CX_LOCAL);
    CX_MEMBER_O(type, reference, bool, CX_READONLY | CX_LOCAL);
    CX_MEMBER_O(type, hasResources, bool, CX_READONLY | CX_LOCAL);
    CX_MEMBER_O(type, templateId, uint32, CX_READONLY | CX_LOCAL);
    CX_MEMBER_O(type, size, uint32, CX_PRIVATE | CX_LOCAL);
    CX_MEMBER_O(type, alignment, uint16, CX_PRIVATE | CX_LOCAL);
    CX_REFERENCE_O(type, defaultType, typedef, CX_GLOBAL, CX_DEFINED, FALSE);
    CX_REFERENCE_O(type, parentType, typedef, CX_GLOBAL, CX_DEFINED, FALSE);
    CX_MEMBER_O(type, parentState, state, CX_GLOBAL);
    CX_MEMBER_O(type, metaprocedures, vtable, CX_LOCAL | CX_PRIVATE);
    CX_METHOD_O(type, sizeof, "()", uint32, FALSE, cx_type_sizeof);
    CX_METHOD_O(type, alignmentof, "()", uint16, FALSE, cx_type_alignmentof);
    CX_METHOD_O(type, allocSize, "()", uint32, TRUE, cx_type_allocSize_v);
    CX_METHOD_O(type, castable, "(lang::type type)", bool, TRUE, cx_type_castable_v);
    CX_METHOD_O(type, compatible, "(lang::type type)", bool, TRUE, cx_type_compatible_v);
    CX_METHOD_O(type, resolveProcedure, "(lang::string name)", function, FALSE, cx_type_resolveProcedure);
    CX_DELEGATE_O(type, init, "(lang::object object)", int16);
    CX_CALLBACK_O(type, _init, "(lang::type object)", type_init, int16, cx_type__init);
    CX_CALLBACK_O(type, construct, "(lang::type object)", class_construct, int16, cx_type_construct);
    CX_CALLBACK_O(type, _destruct, "(lang::type object)", class_destruct, void, cx_type__destruct);
    CX_METAPROCEDURE_O(type, parentof, "()", object, TRUE, cx_type_parentof);
    CX_METAPROCEDURE_O(type, nameof, "()", string, TRUE, cx_type_nameof);
    CX_METAPROCEDURE_O(type, fullname, "()", string, TRUE, cx_type_fullname);
    CX_METAPROCEDURE_O(type, relname, "(lang::object from)", string, TRUE, cx_type_relname);
    CX_METAPROCEDURE_O(type, declare, "(lang::string name,lang::typedef type)", object, TRUE, cx_type_declare);
    CX_METAPROCEDURE_O(type, define, "()", int16, TRUE, cx_type_define);
    CX_METAPROCEDURE_O(type, invalidate, "()", void, TRUE, cx_type_invalidate);
    CX_METAPROCEDURE_O(type, destruct, "()", void, TRUE, cx_type_destruct);
    CX_METAPROCEDURE_O(type, resolve, "(lang::string name)", object, TRUE, cx_type_resolve);
    CX_METAPROCEDURE_O(type, lookup, "(lang::string name)", object, TRUE, cx_type_lookup);
    CX_METAPROCEDURE_O(type, checkAttr, "(lang::attr attributes)", bool, TRUE, cx_type_checkAttr);
    CX_METAPROCEDURE_O(type, checkState, "(lang::state state)", bool, TRUE, cx_type_checkState);
    CX_METAPROCEDURE_O(type, typeof, "()", type, FALSE, cx_type_typeof);
    CX_METAPROCEDURE_O(type, instanceof, "(lang::typedef type)", bool, TRUE, cx_type_instanceof);
    CX_METAPROCEDURE_O(type, compare, "(lang::any value)", equalityKind, FALSE, cx_type_compare);
    CX_METAPROCEDURE_O(type, copy, "(lang::any value)", int16, FALSE, cx_type_copy);
    CX_METAPROCEDURE_O(type, toString, "()", string, FALSE, cx_type_toString);

/* ::cortex::lang::primitive */
CX_CLASS_O(primitive, type, CX_LOCAL | CX_READONLY, CX_SEQUENCE_EMPTY_V(interface), NULL, CX_DECLARED | CX_DEFINED);
    CX_MEMBER_O(primitive, kind, primitiveKind, CX_LOCAL|CX_READONLY);
    CX_MEMBER_O(primitive, width, width, CX_GLOBAL);
    CX_MEMBER_O(primitive, convertId, uint8, CX_LOCAL | CX_PRIVATE);
    CX_METHOD_O(primitive, castable, "(lang::type type)", bool, TRUE, cx_primitive_castable_v);
    CX_METHOD_O(primitive, compatible, "(lang::type type)", bool, TRUE, cx_primitive_compatible_v);
    CX_CALLBACK_O(primitive, init, "(lang::primitive object)", type_init, int16, cx_primitive_init);
    CX_CALLBACK_O(primitive, construct, "(lang::primitive object)", class_construct, int16, cx_primitive_construct);

/* ::cortex::lang::interface */
CX_CLASS_O(interface, type, CX_READONLY, CX_SEQUENCE_EMPTY_V(interface), NULL, CX_DECLARED | CX_DEFINED);
    CX_MEMBER_O(interface, kind, compositeKind, CX_LOCAL|CX_READONLY);
    CX_MEMBER_O(interface, nextMemberId, uint32, CX_LOCAL | CX_PRIVATE);
    CX_MEMBER_O(interface, members, memberSeq, CX_LOCAL | CX_PRIVATE);
    CX_MEMBER_O(interface, methods, vtable, CX_LOCAL | CX_PRIVATE);
    CX_REFERENCE_O(interface, base, interface, CX_GLOBAL, CX_DEFINED, FALSE);
    CX_CALLBACK_O(interface, init, "(lang::interface object)", type_init, int16, cx_interface_init);
    CX_CALLBACK_O(interface, construct, "(lang::interface object)", class_construct, int16, cx_interface_construct);
    CX_CALLBACK_O(interface, destruct, "(lang::interface object)", class_destruct, void, cx_interface_destruct);
    CX_METHOD_O(interface, resolveMember, "(lang::string name)", member, TRUE, cx_interface_resolveMember_v);
    CX_METHOD_O(interface, compatible, "(lang::type type)", bool, TRUE, cx_interface_compatible_v);
    CX_METHOD_O(interface, resolveMethod, "(lang::string name)", method, FALSE, cx_interface_resolveMethod);
    CX_METHOD_O(interface, resolveMethodId, "(lang::string name)", uint32, FALSE, cx_interface_resolveMethodId);
    CX_METHOD_O(interface, resolveMethodById, "(lang::uint32 id)", method, FALSE, cx_interface_resolveMethodById);
    CX_METHOD_O(interface, bindMethod, "(lang::method method)", int16, TRUE, cx_interface_bindMethod_v);
    CX_METHOD_O(interface, baseof, "(lang::interface type)", int16, FALSE, cx_interface_baseof);

/* ::cortex::lang::collection */
CX_CLASS_O(collection, type, CX_LOCAL | CX_READONLY, CX_SEQUENCE_EMPTY_V(interface), NULL, CX_DECLARED | CX_DEFINED);
    CX_MEMBER_O(collection, kind, collectionKind, CX_LOCAL|CX_READONLY);
    CX_REFERENCE_O(collection, elementType, typedef, CX_GLOBAL, CX_DECLARED, FALSE);
    CX_MEMBER_O(collection, max, uint32, CX_GLOBAL);
    CX_METHOD_O(collection, castable, "(lang::type type)", bool, TRUE, cx_collection_castable_v);
    CX_METHOD_O(collection, compatible, "(lang::type type)", bool, TRUE, cx_collection_compatible_v);
    CX_METHOD_O(collection, elementRequiresAlloc, "()", bool, FALSE, cx_collection_elementRequiresAlloc);
    CX_CALLBACK_O(collection, init, "(lang::collection object)", type_init, int16, cx_collection_init);
    CX_METAPROCEDURE_O(collection, size, "()", uint32, FALSE, cx_collection_size);

/* ::cortex::lang::binary */
CX_CLASS_O(binary, primitive, CX_GLOBAL, CX_SEQUENCE_EMPTY_V(interface), NULL, CX_DECLARED | CX_DEFINED);
    CX_CALLBACK_O(binary, init, "(lang::binary object)", type_init, int16, cx_binary_init);

/* ::cortex::lang::boolean */
CX_CLASS_O(boolean, primitive, CX_GLOBAL | CX_READONLY, CX_SEQUENCE_EMPTY_V(interface), NULL, CX_DECLARED | CX_DEFINED);
    CX_CALLBACK_O(boolean, init, "(lang::boolean object)", type_init, int16, cx_boolean_init);

/* ::cortex::lang::character */
CX_CLASS_O(character, primitive, CX_GLOBAL, CX_SEQUENCE_EMPTY_V(interface), NULL, CX_DECLARED | CX_DEFINED);
    CX_CALLBACK_O(character, init, "(lang::character object)", type_init, int16, cx_character_init);

/* ::cortex::lang::int */
CX_CLASS_O(int, primitive, CX_GLOBAL, CX_SEQUENCE_EMPTY_V(interface), NULL, CX_DECLARED | CX_DEFINED);
    CX_MEMBER_O(int, min, int64, CX_GLOBAL);
    CX_MEMBER_O(int, max, int64, CX_GLOBAL);
    CX_CALLBACK_O(int, init, "(lang::int object)", type_init, int16, cx_int_init);

/* ::cortex::lang::uint */
CX_CLASS_O(uint, primitive, CX_GLOBAL, CX_SEQUENCE_EMPTY_V(interface), NULL, CX_DECLARED | CX_DEFINED);
    CX_MEMBER_O(uint, min, uint64, CX_GLOBAL);
    CX_MEMBER_O(uint, max, uint64, CX_GLOBAL);
    CX_CALLBACK_O(uint, init, "(lang::uint object)", type_init, int16, cx_uint_init);

/* ::cortex::lang::float */
CX_CLASS_O(float, primitive, CX_GLOBAL, CX_SEQUENCE_EMPTY_V(interface), NULL, CX_DECLARED | CX_DEFINED);
    CX_MEMBER_O(float, min, float64, CX_GLOBAL);
    CX_MEMBER_O(float, max, float64, CX_GLOBAL);
    CX_CALLBACK_O(float, init, "(lang::float object)", type_init, int16, cx_float_init);

/* ::cortex::lang::text */
CX_CLASS_O(text, primitive, CX_LOCAL, CX_SEQUENCE_EMPTY_V(interface), NULL, CX_DECLARED | CX_DEFINED);
    CX_MEMBER_O(text, charWidth, width, CX_GLOBAL);
    CX_MEMBER_O(text, length, uint64, CX_GLOBAL);
    CX_CALLBACK_O(text, init, "(lang::text object)", type_init, int16, cx_text_init);

/* ::cortex::lang::enum */
CX_CLASS_O(enum, primitive, CX_LOCAL | CX_READONLY, CX_SEQUENCE_EMPTY_V(interface), NULL, CX_DECLARED | CX_DEFINED);
    CX_MEMBER_O(enum, constants, objectSeq, CX_LOCAL | CX_PRIVATE);
    CX_CALLBACK_O(enum, init, "(lang::enum object)", type_init, int16, cx_enum_init);
    CX_CALLBACK_O(enum, destruct, "(lang::enum object)", class_destruct, void, cx_enum_destruct);
    CX_CALLBACK_O(enum, construct, "(lang::enum object)", class_construct, int16, cx_enum_construct);
    CX_METHOD_O(enum, constant, "(lang::int32 value)", object, FALSE, cx_enum_constant);

/* ::cortex::lang::bitmask */
CX_CLASS_O(bitmask, enum, CX_LOCAL | CX_READONLY, CX_SEQUENCE_EMPTY_V(interface), NULL, CX_DECLARED | CX_DEFINED);
    CX_CALLBACK_O(bitmask, init, "(lang::bitmask object)", type_init, int16, cx_bitmask_init);

/* ::cortex::lang::alias */
CX_CLASS_O(alias, primitive, CX_LOCAL | CX_READONLY, CX_SEQUENCE_EMPTY_V(interface), NULL, CX_DECLARED | CX_DEFINED);
    CX_CALLBACK_O(alias, init, "(lang::alias object)", type_init, int16, cx_alias_init);
    CX_MEMBER_O(alias, typeName, string, CX_GLOBAL);

/* ::cortex::lang::struct */
CX_CLASS_O(struct, interface, CX_GLOBAL, CX_SEQUENCE_EMPTY_V(interface), NULL, CX_DECLARED | CX_DEFINED);
    CX_MEMBER_O(struct, baseAccess, modifier, CX_GLOBAL);
    CX_MEMBER_O(struct, delegateCount, uint16, CX_LOCAL|CX_PRIVATE);
    CX_METHOD_O(struct, compatible, "(lang::type type)", bool, TRUE, cx_struct_compatible_v);
    CX_METHOD_O(struct, castable, "(lang::type type)", bool, TRUE, cx_struct_castable_v);
    CX_METHOD_O(struct, resolveMember, "(lang::string name)", member, TRUE, cx_struct_resolveMember_v);
    CX_CALLBACK_O(struct, init, "(lang::struct object)", type_init, int16, cx_struct_init);
    CX_CALLBACK_O(struct, construct, "(lang::struct object)", class_construct, int16, cx_struct_construct);

/* ::cortex::lang::interfaceVector */
CX_STRUCT_O(interfaceVector, NULL, CX_DECLARED | CX_DEFINED);
    CX_MEMBER_O(interfaceVector, interface, interface, CX_GLOBAL);
    CX_MEMBER_O(interfaceVector, vector, vtable, CX_GLOBAL);

/* ::cortex::lang::class */
CX_CLASS_O(class, struct, CX_GLOBAL, CX_SEQUENCE_EMPTY_V(interface), NULL, CX_DECLARED | CX_DEFINED);
    CX_MEMBER_O(class, implements, interfaceSeq, CX_GLOBAL);
    CX_MEMBER_O(class, interfaceVector, interfaceVectorSeq, CX_LOCAL|CX_PRIVATE);
    CX_MEMBER_O(class, observers, observerSeq, CX_LOCAL|CX_PRIVATE);
    CX_DELEGATE_O(class, construct, "(lang::object object)", int16);
    CX_DELEGATE_O(class, destruct, "(lang::object object)", void);
    CX_CALLBACK_O(class, init, "(lang::class object)", type_init, int16, cx_class_init);
    CX_CALLBACK_O(class, _construct, "(lang::class object)", class_construct, int16, cx_class__construct);
    CX_CALLBACK_O(class, _destruct, "(lang::class object)", class_destruct, void, cx_class__destruct);
    CX_METHOD_O(class, bindMethod, "(lang::method method)", int16, FALSE, cx_class_bindMethod);
    CX_METHOD_O(class, allocSize, "()", uint32, TRUE, cx_class_allocSize_v);
    CX_METHOD_O(class, instanceof, "(lang::object object)", bool, FALSE, cx_class_instanceof);
    CX_METHOD_O(class, privateObserver, "(lang::object object,lang::observer observer)", observer, FALSE, cx_class_privateObserver);
    CX_METHOD_O(class, resolveDelegate, "(lang::string name)", delegate, FALSE, cx_class_resolveDelegate);
    CX_METHOD_O(class, resolveInterfaceMethod, "(lang::interface interface,lang::uint32 method)", method, FALSE, cx_class_resolveInterfaceMethod);
    CX_METHOD_O(class, resolveCallback, "(lang::delegate delegate,lang::object target)", callback, FALSE, cx_class_resolveCallback);
    CX_METHOD_O(class, bindCallback, "(lang::delegate delegate,lang::object object,lang::callback method)", int16, FALSE, cx_class_bindCallback);
    CX_METHOD_O(class, bindDelegate, "(lang::delegate delegate)", int16, FALSE, cx_class_bindDelegate);
    CX_METHOD_O(class, bindObserver, "(lang::observer observer)", void, FALSE, cx_class_bindDelegate);
    CX_METHOD_O(class, findObserver, "(lang::object observable,string expr)", observer, FALSE, cx_class_findObserver);

/* ::cortex::lang::procptrdata */
CX_STRUCT_O(procptrdata, NULL, CX_DECLARED | CX_DEFINED);
    CX_MEMBER_O(procptrdata, instance, object, CX_GLOBAL);
    CX_MEMBER_O(procptrdata, procedure, function, CX_GLOBAL);

/* ::cortex::lang::procptr */
CX_CLASS_O(procptr, struct, CX_READONLY, CX_SEQUENCE_EMPTY_V(interface), NULL, CX_DECLARED | CX_DEFINED);
    CX_CALLBACK_O(procptr, init, "(lang::procptr object)", type_init, int16, cx_procptr_init);
    CX_REFERENCE_O(procptr, returnType, typedef, CX_GLOBAL, CX_DEFINED | CX_DECLARED, FALSE);
    CX_MEMBER_O(procptr, returnsReference, bool, CX_GLOBAL);
    CX_MEMBER_O(procptr, parameters, parameterSeq, CX_GLOBAL);
    CX_METHOD_O(procptr, compatible, "(lang::type type)", bool, TRUE, cx_procptr_compatible_v);

/* ::cortex::lang::procedure */
CX_CLASS_O(procedure, struct, CX_GLOBAL, CX_SEQUENCE_EMPTY_V(interface), NULL, CX_DECLARED | CX_DEFINED);
    CX_MEMBER_O(procedure, kind, procedureKind, CX_READONLY);
    CX_CALLBACK_O(procedure, init, "(lang::procedure object)", type_init, int16, cx_procedure_init);
    CX_DELEGATE_O(procedure, bind, "(lang::object object)", int16);
    CX_METHOD_O(procedure, unbind, "(lang::object object)", void, FALSE, cx_procedure_unbind);

/* ::cortex::lang::array */
CX_CLASS_O(array, collection, CX_GLOBAL, CX_SEQUENCE_EMPTY_V(interface), NULL, CX_DECLARED | CX_DEFINED);
    CX_REFERENCE_O(array, elementType, typedef, CX_GLOBAL|CX_PRIVATE, CX_DEFINED, FALSE);
    CX_CALLBACK_O(array, init, "(lang::array object)", type_init, int16, cx_array_init);
    CX_CALLBACK_O(array, construct, "(lang::array object)", class_construct, int16, cx_array_construct);
    CX_CALLBACK_O(array, destruct, "(lang::array object)", class_destruct, void, cx_array_destruct);

/* ::cortex::lang::sequence */
CX_CLASS_O(sequence, collection, CX_GLOBAL, CX_SEQUENCE_EMPTY_V(interface), NULL, CX_DECLARED | CX_DEFINED);
    CX_CALLBACK_O(sequence, init, "(lang::sequence object)", type_init, int16, cx_sequence_init);
    CX_CALLBACK_O(sequence, construct, "(lang::sequence object)", class_construct, int16, cx_sequence_construct);
    CX_METAPROCEDURE_O(sequence, size, "(lang::uint32 size)", void, FALSE, cx_sequence_size);

/* ::cortex::lang::list */
CX_CLASS_O(list, collection, CX_GLOBAL, CX_SEQUENCE_EMPTY_V(interface), NULL, CX_DECLARED | CX_DEFINED);
    CX_CALLBACK_O(list, init, "(lang::list object)", type_init, int16, cx_list_init);
    CX_CALLBACK_O(list, construct, "(lang::list object)", class_construct, int16, cx_list_construct);
    CX_METAPROCEDURE_O(list, insert, "(lang::any element)", void, FALSE, cx_list_insert_lang_any);
    CX_METAPROCEDURE_O(list, append, "(lang::any element)", void, FALSE, cx_list_append_lang_any);
    CX_METAPROCEDURE_NAME_O(list, insert_, insert, "()", any, FALSE, cx_list_insert_);
    CX_METAPROCEDURE_NAME_O(list, append_, append, "()", any, FALSE, cx_list_append_);
    CX_METAPROCEDURE_O(list, reverse, "()", void, FALSE, cx_list_reverse);
    CX_METAPROCEDURE_O(list, clear, "()", void, FALSE, cx_list_clear);

/* ::cortex::lang::map */
CX_CLASS_O(map, collection, CX_LOCAL, CX_SEQUENCE_EMPTY_V(interface), NULL, CX_DECLARED | CX_DEFINED);
    /* Duplicate members for a more convenient order in the initializer */
    CX_REFERENCE_O(map, elementType, typedef, CX_GLOBAL, CX_DECLARED | CX_DEFINED, FALSE);
    CX_REFERENCE_O(map, keyType, typedef, CX_GLOBAL, CX_DECLARED | CX_DEFINED, FALSE);
    CX_MEMBER_O(map, max, uint32, CX_GLOBAL);
    CX_CALLBACK_O(map, init, "(lang::map object)", type_init, int16, cx_map_init);
    CX_CALLBACK_O(map, construct, "(lang::map object)", class_construct, int16, cx_map_construct);

/* ::cortex::lang::function */
CX_PROCEDURE_NOBASE_O(function, CX_FUNCTION, NULL, CX_DECLARED | CX_DEFINED);
    CX_REFERENCE_O(function, returnType, typedef, CX_GLOBAL, CX_DEFINED | CX_DECLARED, FALSE);
    CX_MEMBER_O(function, returnsReference, bool, CX_GLOBAL);
    CX_MEMBER_O(function, overloaded, bool, CX_LOCAL | CX_READONLY);
    CX_MEMBER_O(function, kind, uint32, CX_LOCAL | CX_PRIVATE);
    CX_MEMBER_O(function, impl, word, CX_LOCAL|CX_PRIVATE);
    CX_MEMBER_O(function, implData, word, CX_LOCAL|CX_PRIVATE);
    CX_REFERENCE_O(function, resource, object, CX_LOCAL|CX_PRIVATE, CX_DEFINED | CX_DECLARED, FALSE);
    CX_MEMBER_O(function, size, int16, CX_LOCAL|CX_PRIVATE);
    CX_MEMBER_O(function, parameters, parameterSeq, CX_LOCAL | CX_READONLY);
    CX_MEMBER_O(function, nextParameterId, uint32, CX_LOCAL | CX_PRIVATE);
    CX_CALLBACK_O(function, init, "(lang::function object)", type_init, int16, cx_function_init);
    CX_CALLBACK_O(function, bind, "(lang::function object)", procedure_bind, int16, cx_function_bind);
    CX_FUNCTION_O(function, unbind, "(lang::function object)", void, cx_function_unbind);
    CX_FUNCTION_O(function, stringToParameterSeq, "(lang::string name,lang::object scope)", parameterSeq, cx_function_stringToParameterSeq);

/* ::cortex::lang::dispatcher */
CX_INTERFACE_O(dispatcher);
    CX_IMETHOD_O(dispatcher, post, "(lang::event event)", void, FALSE);
    CX_IMETHOD_O(dispatcher, getEvent, "(lang::observer observer,lang::object me,lang::object observable,lang::object src)", observableEvent, FALSE);

/* ::cortex::lang::event */
CX_CLASS_NOBASE_O(event, CX_SEQUENCE_EMPTY_V(interface), NULL, CX_DECLARED | CX_DEFINED);
    CX_MEMBER_O(event, kind, uint16, CX_GLOBAL);
    CX_MEMBER_O(event, handled, bool, CX_LOCAL | CX_READONLY);
    CX_METHOD_O(event, processed, "()", void, FALSE, cx_event_processed);
    CX_FUNCTION_O(event, uniqueKind, "()", int16, cx_event_uniqueKind);

/* ::cortex::lang::observableEvent */
CX_CLASS_O(observableEvent, event, CX_GLOBAL, CX_SEQUENCE_EMPTY_V(interface), NULL, CX_DECLARED | CX_DEFINED);
    CX_REFERENCE_O(observableEvent, observer, observer, CX_GLOBAL, CX_DEFINED | CX_DECLARED, FALSE);
    CX_REFERENCE_O(observableEvent, me, object, CX_GLOBAL, CX_DEFINED | CX_DECLARED, FALSE);
    CX_REFERENCE_O(observableEvent, source, object, CX_GLOBAL, CX_DEFINED | CX_DECLARED, FALSE);
    CX_REFERENCE_O(observableEvent, observable, object, CX_GLOBAL, CX_DEFINED | CX_DECLARED, FALSE);

/* ::cortex::lang::method */
CX_PROCEDURE_O(method, CX_METHOD, function, CX_GLOBAL, CX_SSO_TYPE_ID(interface), CX_DECLARED);
    CX_MEMBER_O(method, virtual, bool, CX_GLOBAL);
    CX_CALLBACK_O(method, init, "(lang::method object)", type_init, int16, cx_method_init);
    CX_CALLBACK_O(method, bind, "(lang::method object)", procedure_bind, int16, cx_method_bind);

CX_PROCEDURE_O(virtual, CX_METHOD, method, CX_GLOBAL, CX_SSO_TYPE_ID(interface), CX_DECLARED);
    CX_CALLBACK_O(virtual, init, "(lang::virtual object)", type_init, int16, cx_virtual_init);
    
/* ::cortex::lang::delegate */
CX_PROCEDURE_O(delegate, CX_DELEGATE, function, CX_GLOBAL, NULL, CX_DECLARED | CX_DEFINED);
    CX_MEMBER_O(delegate, id, uint32, CX_LOCAL);
    CX_CALLBACK_O(delegate, init, "(lang::delegate object)", type_init, int16, cx_delegate_init);

/* ::cortex::lang::callback */
CX_PROCEDURE_O(callback, CX_CALLBACK, function, CX_GLOBAL, NULL, CX_DECLARED);
    CX_REFERENCE_O(callback, delegate, delegate, CX_GLOBAL, CX_DEFINED, FALSE);
    CX_CALLBACK_O(callback, init, "(lang::callback object)", type_init, int16, cx_callback_init);
    CX_CALLBACK_O(callback, bind, "(lang::callback object)", procedure_bind, int16, cx_callback_bind);

/* ::cortex::lang::observer */
CX_PROCEDURE_O(observer, CX_OBSERVER, function, CX_LOCAL | CX_READONLY, NULL, CX_DECLARED | CX_DEFINED);
    CX_REFERENCE_O(observer, observable, object, CX_GLOBAL, CX_DEFINED | CX_DECLARED, FALSE);
    CX_MEMBER_O(observer, mask, eventMask, CX_GLOBAL);
    CX_MEMBER_O(observer, expression, string, CX_GLOBAL);
    CX_MEMBER_O(observer, template, uint32, CX_GLOBAL);
    CX_REFERENCE_O(observer, dispatcher, dispatcher, CX_GLOBAL, CX_DEFINED | CX_DECLARED, FALSE);
    CX_REFERENCE_O(observer, me, object, CX_GLOBAL, CX_DEFINED | CX_DECLARED, FALSE);
    CX_REFERENCE_O(observer, observing, object, CX_LOCAL | CX_PRIVATE, CX_DEFINED | CX_DECLARED, FALSE);
    CX_REFERENCE_O(observer, delayedBinder, observer, CX_LOCAL | CX_PRIVATE, CX_DEFINED | CX_DECLARED, FALSE);
    CX_CALLBACK_O(observer, init, "(lang::observer object)", type_init, int16, cx_observer_init);
    CX_CALLBACK_O(observer, bind, "(lang::observer object)", procedure_bind, int16, cx_observer_bind);
    CX_METHOD_O(observer, listen, "(lang::object observable,lang::object me)", int16, FALSE, cx_observer_listen);
    CX_METHOD_O(observer, silence, "(lang::object me)", int16, FALSE, cx_observer_silence);
    CX_METHOD_O(observer, setDispatcher, "(lang::dispatcher dispatcher)", void, FALSE, cx_observer_setDispatcher);
    CX_FUNCTION_O(observer, unbind, "(lang::observer object)", void, cx_observer_unbind);

/* ::cortex::lang::metaprocedure */
CX_PROCEDURE_O(metaprocedure, CX_METAPROCEDURE, function, CX_GLOBAL, NULL, CX_DECLARED);
    CX_CALLBACK_O(metaprocedure, bind, "(lang::metaprocedure object)", procedure_bind, int16, cx_metaprocedure_bind);
    CX_MEMBER_O(metaprocedure, referenceOnly, bool, CX_GLOBAL);

/* ::cortex::lang::member */
CX_CLASS_NOBASE_O(member, CX_SEQUENCE_EMPTY_V(interface), CX_SSO_TYPE_ID(interface), CX_DECLARED);
    CX_REFERENCE_O(member, type, typedef, CX_GLOBAL, CX_DECLARED | CX_DEFINED, FALSE);
    CX_MEMBER_O(member, modifiers, modifier, CX_GLOBAL);
    CX_MEMBER_O(member, state, state, CX_GLOBAL);
    CX_MEMBER_O(member, weak, bool, CX_GLOBAL);
    CX_MEMBER_O(member, id, uint32, CX_GLOBAL | CX_PRIVATE);
    CX_MEMBER_O(member, offset, uint32, CX_LOCAL | CX_PRIVATE);
    CX_CALLBACK_O(member, init, "(lang::member object)", type_init, int16, cx_member_init);
    CX_CALLBACK_O(member, construct, "(lang::member object)", class_construct, int16, cx_member_construct);

/* ::cortex::lang::parameter */
CX_STRUCT_O(parameter, NULL, CX_DECLARED | CX_DEFINED);
    CX_MEMBER_O(parameter, name, string, CX_GLOBAL);
    CX_REFERENCE_O(parameter, type, typedef, CX_GLOBAL, CX_DECLARED | CX_DEFINED, FALSE);
    CX_MEMBER_O(parameter, passByReference, bool, CX_GLOBAL);

#ifdef __cplusplus
}
#endif

#endif /* CX_BOOTSTRAP_H_ */