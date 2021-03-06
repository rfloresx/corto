/* This is a managed file. Do not delete this comment. */

#include <corto/corto.h>


#include "_enum.h"

corto_int16 corto__enum_bindConstant(corto_enum this, corto_constant* c) {
    if (corto_checkState(corto_type_o, CORTO_DEFINED)) {
        *c = corto_scopeSize(this) - 1;
    }
    this->constants.buffer = corto_realloc(this->constants.buffer, (this->constants.length+1) * sizeof(corto_constant*));
    this->constants.buffer[this->constants.length] = c;
    this->constants.length++;

    corto_claim(c);

    return 0;
}


struct corto_enum_findConstant_t {
    corto_int32 value;
    corto_constant* o;
};

static int corto_enum_findConstant(corto_object o, void* ctx) {
    struct corto_enum_findConstant_t* userData;

    userData = ctx;
    if (*(corto_constant*)o == userData->value) {
        userData->o = o;
    }
    return userData->o == NULL;
}

corto_object corto_enum_constant(
    corto_enum this,
    int32_t value)
{
    struct corto_enum_findConstant_t walkData;

    /* Walk scope */
    walkData.value = value;
    walkData.o = NULL;
    corto_scopeWalk(this, corto_enum_findConstant, &walkData);

    return walkData.o;
}

int16_t corto_enum_construct(
    corto_enum this)
{
    corto_uint32 i;

    /* Define constants */
    for(i=0; i<this->constants.length; i++) {
        corto_define(this->constants.buffer[i]);
    }

    return corto_primitive_construct(corto_primitive(this));
}

void corto_enum_destruct(
    corto_enum this)
{
    int i;
    for (i = 0; i < this->constants.length; i++) {
      corto_release(this->constants.buffer[i]);
    }
    this->constants.length = 0;
    corto_dealloc(this->constants.buffer);
    this->constants.buffer = NULL;
    
    corto_type_destruct(corto_type(this));
}

int16_t corto_enum_init(
    corto_enum this)
{
    corto_primitive(this)->kind = CORTO_ENUM;
    corto_primitive(this)->width = CORTO_WIDTH_32;
    return corto_primitive_init((corto_primitive)this);
}

