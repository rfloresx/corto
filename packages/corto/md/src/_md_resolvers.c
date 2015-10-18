#include "_md_resolvers.h"

/*
 * Returns TRUE if `ancestor` is recursively a parent of `o`, except if
 * `ancestor` is root_o.
 */
static cx_bool md_isAncestor(cx_object ancestor, cx_object o) {
    cx_bool isAncestor = FALSE;
    cx_object p = o;
    while (p && !isAncestor) {
        p = cx_parentof(p);
        if (p == ancestor) {
            isAncestor = TRUE;
        }
    }
    return isAncestor;
}

cx_object md_resolve(int level, cx_string name, cx_object *parent, md_parseData* data) {
    cx_assert(level >= 1 && level <= 6, "Level must be between 1 and 6");
    cx_object o = NULL, current = NULL;

    cx_object previous = NULL;
    if (level == 1) {
        previous = data->destination;
    } else {
        cx_uint32 i = level - 1;
        while (i && !(previous = md_Doc(data->headers[i]))) {
            i--;
        }
        previous = data->headers[i];
    }
    if (previous == NULL) {
        goto notFound;
    }

    if (cx_instanceof(md_Doc_o, previous)) {
        current = md_Doc(previous)->o;
    }

    if (parent) {
        *parent = previous;
    }

    if (current || (level == 1)) {
        o = cx_resolve(current, name);
        if (o == NULL) {
            goto notFound;
        }
        if (!md_isAncestor(current, o)) {
            o = NULL;
        }
    }
    
    /* Clean up the lower levels of the doc hierarchy */
    int i = 6;
    do {
        data->headers[i] = NULL;
    } while (i-- > level);

    return o;
notFound:
    return NULL;
}
