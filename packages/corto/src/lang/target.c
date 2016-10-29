/* $CORTO_GENERATED
 *
 * target.c
 *
 * Only code written between the begin and end tags will be preserved
 * when the file is regenerated.
 */

#include <corto/lang/lang.h>

corto_int16 _corto_target_construct(
    corto_target this)
{
/* $begin(corto/lang/target/construct) */

    corto_member target = corto_declareChild(this, "target", this->type);
    if (!target) {
        goto error;
    }
    if (!corto_checkState(target, CORTO_DEFINED)) {
        corto_setref(&target->type, this->type);
        if (corto_define(target)) {
            goto error;
        }
    }

    corto_member actual = corto_declareChild(this, "actual", this->type);
    if (!actual) {
        goto error;
    }
    if (!corto_checkState(actual, CORTO_DEFINED)) {
        corto_setref(&actual->type, this->type);
        if (corto_define(actual)) {
            goto error;
        }
    }

    corto_type(this)->reference = TRUE;

    return 0;
error:
    return -1;
/* $end */
}