/* This is a managed file. Do not delete this comment. */

#include <include/test.h>

void test_Resolver_setup(
    test_Resolver this)
{

}

void test_Resolver_tc_caseInsensitive(
    test_Resolver this)
{

    corto_object o = corto_resolve(NULL, "COrTO");
    test_assert(o != NULL);
    test_assert(o == corto_o);
    corto_release(o);

}


int tc_resolveAllWalk(corto_object o, void *ctx) {
    CORTO_UNUSED(ctx);
    corto_id id;
    corto_object r;

    corto_fullpath(id, o);
    r = corto_resolve(NULL, id);

    /* Set errormessage to ease debugging */
    if (!r) corto_seterr("failed to resolve %s", id);
    test_assert(r != NULL);
    if (r != o) {
        corto_seterr("got %s, expected %s",
          corto_fullpath(NULL, r),
          corto_fullpath(NULL, o));
    }
    test_assert(r == o);
    corto_release(r);

    corto_objectseq scope = corto_scopeClaim(o);
    int i;
    for (i = 0; i < scope.length; i ++) {
        tc_resolveAllWalk(scope.buffer[i], NULL);
    }
    corto_scopeRelease(scope);

    return 1;
}

void test_Resolver_tc_resolveAll(
    test_Resolver this)
{
    corto_objectseq scope = corto_scopeClaim(root_o);
    int i;
    for (i = 0; i < scope.length; i ++) {
        tc_resolveAllWalk(scope.buffer[i], NULL);
    }
    corto_scopeRelease(scope);
}

void test_Resolver_tc_resolveAnonymous(
    test_Resolver this)
{

    corto_object o = corto_resolve(NULL, "string{\"Hello World\"}");
    test_assert(o != NULL);

    corto_type t = corto_typeof(o);
    test_assert(t->kind == CORTO_PRIMITIVE);
    test_assert(corto_primitive(t)->kind == CORTO_TEXT);

    corto_string s = *(corto_string*)o;
    test_assertstr(s, "Hello World");

    corto_release(o);

}

void test_Resolver_tc_resolveAnonymousAnonymousType(
    test_Resolver this)
{

    corto_object o = corto_resolve(NULL, "list{int32}{1, 2, 3}");
    test_assert(o != NULL);

    corto_type t = corto_typeof(o);
    test_assert(t->kind == CORTO_COLLECTION);
    test_assert(corto_collection(t)->kind == CORTO_LIST);
    test_assert(corto_collection(t)->elementType == corto_type(corto_int32_o));

    corto_ll l = *(corto_ll*)o;
    test_assert(corto_ll_size(l) == 3);
    test_assertint((corto_int32)(corto_word)corto_ll_get(l, 0), 1);
    test_assertint((corto_int32)(corto_word)corto_ll_get(l, 1), 2);
    test_assertint((corto_int32)(corto_word)corto_ll_get(l, 2), 3);

    corto_release(o);

}

void test_Resolver_tc_resolveCorto(
    test_Resolver this)
{

    corto_object o = corto_resolve(NULL, "corto");
    test_assert (o == corto_o);
    corto_release(o);

}

void test_Resolver_tc_resolveEmptyString(
    test_Resolver this)
{

    corto_object o = corto_resolve(NULL, "");
    test_assert (o == NULL);
    if (o) corto_release(o);

}

void test_Resolver_tc_resolveFunctionArgs(
    test_Resolver this)
{

    corto_object o = corto_resolve(NULL, "test/functionToResolve(int32,int32)");
    test_assert(o != NULL);
    test_assert(o == test_functionToResolve_o);

}

void test_Resolver_tc_resolveFunctionNoArgs(
    test_Resolver this)
{

    corto_object o = corto_resolve(NULL, "test/functionToResolve");
    test_assert(o != NULL);
    test_assert(o == test_functionToResolve_o);

}

void test_Resolver_tc_resolveG(
    test_Resolver this)
{

    /* Start loader mount */
    corto_object o = corto_resolve(NULL, "g");
    test_assert(o != NULL);
    test_assert (!strcmp(corto_idof(o), "g"));
    test_assert (corto_parentof(o) == corto_o);
    corto_release(o);

}

void test_Resolver_tc_resolveIdFromNull(
    test_Resolver this)
{

    corto_object o = corto_resolve(NULL, "corto");
    test_assert(o != NULL);
    test_assert(o == corto_o);

}

void test_Resolver_tc_resolveIdFromRoot(
    test_Resolver this)
{

    corto_object o = corto_resolve(root_o, "corto");
    test_assert(o != NULL);
    test_assert(o == corto_o);

}

void test_Resolver_tc_resolveIdFromScope(
    test_Resolver this)
{

    corto_object o = corto_resolve(corto_core_o, "mount");
    test_assert(o != NULL);
    test_assert(o != corto_word_o);

}

void test_Resolver_tc_resolveLang(
    test_Resolver this)
{

    corto_object o = corto_resolve(NULL, "lang");
    test_assert(o != NULL);
    test_assert (!strcmp(corto_idof(o), "lang"));
    test_assert (o == corto_lang_o);
    corto_release(o);

}

void test_Resolver_tc_resolveNested2FromNull(
    test_Resolver this)
{

    corto_object o = corto_resolve(NULL, "corto/core/mount");
    test_assert(o != NULL);
    test_assert(o == corto_mount_o);

}

void test_Resolver_tc_resolveNested2FromRoot(
    test_Resolver this)
{

    corto_object o = corto_resolve(root_o, "corto/core/mount");
    test_assert(o != NULL);
    test_assert(o == corto_mount_o);

}

void test_Resolver_tc_resolveNested2FromScope(
    test_Resolver this)
{

    corto_object parent = corto_createChild(corto_o, "parent", corto_void_o);
    corto_object child = corto_createChild(parent, "child", corto_void_o);
    corto_object grandchild = corto_createChild(child, "grandchild", corto_void_o);

    corto_object o = corto_resolve(corto_o, "parent/child/grandchild");
    test_assert(o != NULL);
    test_assert(o == grandchild);

    corto_release(o);
    corto_delete(parent);

}

void test_Resolver_tc_resolveNestedFromNull(
    test_Resolver this)
{

    corto_object o = corto_resolve(NULL, "corto/core");
    test_assert(o != NULL);
    test_assert(o == corto_core_o);

}

void test_Resolver_tc_resolveNestedFromRoot(
    test_Resolver this)
{

    corto_object o = corto_resolve(root_o, "corto/core");
    test_assert(o != NULL);
    test_assert(o == corto_core_o);

}

void test_Resolver_tc_resolveNestedFromScope(
    test_Resolver this)
{

    corto_object o = corto_resolve(corto_o, "core/mount");
    test_assert(o != NULL);
    test_assert(o == corto_mount_o);

}

void test_Resolver_tc_resolveNull(
    test_Resolver this)
{

    corto_object o = corto_resolve(NULL, NULL);
    test_assert (o == NULL);

}

void test_Resolver_tc_resolveObjectAFromScopeWithFunctionA(
    test_Resolver this)
{

    corto_object o = corto_resolve(corto_o, "/corto/lang/class/construct");
    test_assert(o != NULL);
    test_assertstr("/corto/lang/class/construct", corto_fullpath(NULL, o));

}

void test_Resolver_tc_resolveParent(
    test_Resolver this)
{

    corto_object o = corto_resolve(corto_lang_o, "..");
    test_assert(o == corto_o);
    corto_release(o);

}

void test_Resolver_tc_resolveParentAfterExpr(
    test_Resolver this)
{

    corto_object o = corto_resolve(corto_o, "lang/class/..");
    test_assert(o == corto_lang_o);
    corto_release(o);

}

void test_Resolver_tc_resolveParentBeforeExpr(
    test_Resolver this)
{

    corto_object o = corto_resolve(corto_class_o, "../method");
    test_assert(o == corto_method_o);
    corto_release(o);

}

void test_Resolver_tc_resolveParenthesesNoFunction(
    test_Resolver this)
{

    corto_object o = corto_voidCreateChild(NULL, "o()");
    test_assert(o != NULL);
    test_assert(!strcmp(corto_idof(o), "o()"));

    corto_object p = corto_resolve(NULL, "o()");
    test_assert(p != NULL);
    test_assert(o == p);

    corto_delete(o);
    corto_release(p);

}

void test_Resolver_tc_resolveParenthesesNoFunctionArgs(
    test_Resolver this)
{

    corto_object o = corto_voidCreateChild(NULL, "o(uint32 a)");
    test_assert(o != NULL);
    test_assert(!strcmp(corto_idof(o), "o(uint32 a)"));

    corto_object p = corto_resolve(NULL, "o(uint32 a)");
    test_assert(p != NULL);
    test_assert(o == p);

    corto_delete(o);
    corto_release(p);

}

void test_Resolver_tc_resolveParenthesesNoFunctionArgsScoped(
    test_Resolver this)
{
    corto_object parent = corto_voidCreateChild(NULL, "parent");
    test_assert(parent != NULL);

    corto_object o = corto_voidCreateChild(parent, "o(uint32 a)");
    test_assert(o != NULL);
    test_assert(!strcmp(corto_idof(o), "o(uint32 a)"));
    test_assert(!strcmp(corto_fullpath(NULL, o), "/parent/o(uint32 a)"));

    corto_object p = corto_resolve(NULL, "/parent/o(uint32 a)");
    test_assert(p != NULL);
    test_assert(o == p);

    corto_delete(o);
    corto_delete(parent);
    corto_release(p);

}

void test_Resolver_tc_resolveParenthesesNoFunctionMatchingArgs(
    test_Resolver this)
{

    corto_object o = corto_voidCreateChild(NULL, "o(uint32 a)");
    test_assert(o != NULL);
    test_assert(!strcmp(corto_idof(o), "o(uint32 a)"));

    corto_object p = corto_resolve(NULL, "o(uint16 a)");
    test_assert(p == NULL);

    corto_delete(o);

}

void test_Resolver_tc_resolveParenthesesNoFunctionMatchingArgsScoped(
    test_Resolver this)
{
    corto_object parent = corto_voidCreateChild(NULL, "parent");
    test_assert(parent != NULL);

    corto_object o = corto_voidCreateChild(parent, "o(uint32 a)");
    test_assert(o != NULL);
    test_assert(!strcmp(corto_idof(o), "o(uint32 a)"));
    test_assert(!strcmp(corto_fullpath(NULL, o), "/parent/o(uint32 a)"));

    corto_object p = corto_resolve(NULL, "/parent/o(uint16 a)");
    test_assert(p == NULL);

    corto_delete(o);
    corto_delete(parent);

}

void test_Resolver_tc_resolveParenthesesNoFunctionScoped(
    test_Resolver this)
{
    corto_id id;

    corto_object parent = corto_voidCreateChild(NULL, "parent");
    test_assert(parent != NULL);

    corto_object o = corto_voidCreateChild(parent, "o()");
    test_assert(o != NULL);
    test_assert(!strcmp(corto_idof(o), "o()"));
    test_assert(!strcmp(corto_fullpath(id, o), "/parent/o()"));

    corto_object p = corto_resolve(NULL, "/parent/o()");
    test_assert(p != NULL);
    test_assert(p == o);

    corto_delete(o);
    corto_delete(parent);
    corto_release(p);

}

void test_Resolver_tc_resolveParentInExpr(
    test_Resolver this)
{

    corto_object o = corto_resolve(corto_lang_o, "class/../method");
    test_assert(o == corto_method_o);
    corto_release(o);

}

void test_Resolver_tc_resolveRoot(
    test_Resolver this)
{

    corto_object o = corto_resolve(NULL, "/");
    test_assert (o == root_o);
    corto_release(o);

}

void test_Resolver_tc_resolveString(
    test_Resolver this)
{

    corto_object o = corto_resolve(NULL, "string");
    test_assert (o == corto_string_o);
    corto_release(o);

}

void test_Resolver_tc_resolveThis(
    test_Resolver this)
{

    corto_object o = corto_resolve(corto_lang_o, ".");
    test_assert(o == corto_lang_o);
    corto_release(o);

}

void test_Resolver_tc_resolveThisAfterExpr(
    test_Resolver this)
{

    corto_object o = corto_resolve(NULL, "corto/core/.");
    test_assert(o == corto_core_o);
    corto_release(o);

}

void test_Resolver_tc_resolveThisBeforeExpr(
    test_Resolver this)
{

    corto_object o = corto_resolve(corto_core_o, "./mount");
    test_assert(o == corto_mount_o);
    corto_release(o);

}

void test_Resolver_tc_resolveThisInExpr(
    test_Resolver this)
{

    corto_object o = corto_resolve(corto_o, "core/./mount");
    test_assert(o == corto_mount_o);
    corto_release(o);

}

void test_Resolver_teardown(
    test_Resolver this)
{

}

