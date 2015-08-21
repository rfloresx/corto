#include "c_api.h"
#include "c_common.h"
#include "ctype.h"

/* Translate members to function parameters. */
static cx_int16 c_apiAssignMember(cx_serializer s, cx_value* v, void* userData) {
    c_apiWalk_t* data;
    cx_member m;
    cx_id memberIdTmp, memberParamId, memberId;

    CX_UNUSED(s);

    if (v->kind == CX_MEMBER) {
        m = v->is.member.t;
        data = userData;

        cx_genMemberName(data->g, data->memberCache, m, memberIdTmp);
        g_id(data->g, memberIdTmp, memberParamId);
        g_id(data->g, cx_nameof(m), memberId);

        /* If member is of array-type, use memcpy */
        if ((m->type->kind == CX_COLLECTION) && (cx_collection(m->type)->kind == CX_ARRAY)) {
            cx_id typeId, postfix;
            /* Get typespecifier */
            if (c_specifierId(data->g, m->type, typeId, NULL, postfix)) {
                goto error;
            }
            g_fileWrite(data->source, "memcpy(");

            /* Cast object to right type */
            if (data->current == cx_parentof(m)) {
                g_fileWrite(data->source, "_this->%s",
                        memberId);
            } else {
                cx_id typeId;
                g_fileWrite(data->source, "%s(_this)->%s",
                        g_fullOid(data->g, cx_parentof(m), typeId), memberId);
            }

            g_fileWrite(data->source, ", %s, sizeof(%s%s));\n", memberParamId, typeId, postfix);
        } else {
            if (m->type->reference) {
                g_fileWrite(data->source, "cx_setref(&");
            } else if ((m->type->kind == CX_PRIMITIVE) && (cx_primitive(m->type)->kind == CX_TEXT)) {
                g_fileWrite(data->source, "cx_setstr(&");
            }

            /* Cast object to right type */
            if (data->current == cx_parentof(m)) {
                g_fileWrite(data->source, "_this->%s",
                        memberId);
            } else {
                cx_id typeId;
                g_fileWrite(data->source, "%s(_this)->%s",
                        g_fullOid(data->g, cx_parentof(m), typeId), memberId);
            }

            /* Strdup strings */
            if ((m->type->kind == CX_PRIMITIVE) && (cx_primitive(m->type)->kind == CX_TEXT)) {
                g_fileWrite(data->source, ", %s);\n", memberParamId);
            } else if (m->type->reference) {
                if (m->type->kind != CX_VOID) {
                    cx_id id;
                    g_fileWrite(data->source, ", %s(%s));\n", g_fullOid(data->g, m->type, id), memberParamId);
                } else {
                    g_fileWrite(data->source, ", %s);\n", memberParamId);
                }
            } else {
                g_fileWrite(data->source, " = %s;\n", memberParamId);  
            }
        }

        data->parameterCount++;
    }

    return 0;
error:
    return -1;
}

/* Translate members to function parameters. */
static cx_int16 c_apiParamMember(cx_serializer s, cx_value* v, void* userData) {
    c_apiWalk_t* data;
    cx_member m;
    cx_id typeSpec, typePostfix, memberIdTmp, memberId;

    CX_UNUSED(s);

    if (v->kind == CX_MEMBER) {
        m = v->is.member.t;
        data = userData;

        if (data->parameterCount) {
            g_fileWrite(data->header, ", ");
            g_fileWrite(data->source, ", ");
        }

        /* Get type-specifier */
        c_specifierId(data->g, m->type, typeSpec, NULL, typePostfix);
        cx_genMemberName(data->g, data->memberCache, m, memberIdTmp);

        g_fileWrite(data->header, "%s%s %s",
               typeSpec, typePostfix, g_id(data->g, memberIdTmp, memberId));

        g_fileWrite(data->source, "%s%s %s",
                typeSpec, typePostfix, g_id(data->g, memberIdTmp, memberId));

        data->parameterCount++;
    }

    return 0;
}

/* Member parameter serializer */
static struct cx_serializer_s c_apiParamSerializer(void) {
    struct cx_serializer_s s;

    cx_serializerInit(&s);
    s.metaprogram[CX_MEMBER] = c_apiParamMember;
    s.access = CX_LOCAL|CX_READONLY|CX_PRIVATE;
    s.accessKind = CX_NOT;

    return s;
}

/* Member parameter serializer */
static struct cx_serializer_s c_apiAssignSerializer(void) {
    struct cx_serializer_s s;

    cx_serializerInit(&s);
    s.metaprogram[CX_MEMBER] = c_apiAssignMember;
    s.access = CX_LOCAL|CX_READONLY|CX_PRIVATE;
    s.accessKind = CX_NOT;

    return s;
}

cx_int16 c_apiTypeCreateIntern(cx_type t, c_apiWalk_t *data, cx_string func, cx_bool scoped, cx_bool define) {
    cx_id id;
    struct cx_serializer_s s;

    data->parameterCount = 0;
    g_fullOid(data->g, t, id);

    /* Function declaration */
    g_fileWrite(data->header, "%s%s %s__%s(", id, t->reference ? "" : "*", id, func);

    /* Function implementation */
    g_fileWrite(data->source, "%s%s %s__%s(", id, t->reference ? "" : "*", id, func);

    if (scoped) {
        g_fileWrite(data->header, "cx_object _parent, cx_string _name");
    	g_fileWrite(data->source, "cx_object _parent, cx_string _name");
    	data->parameterCount = 2;
    }

    /* Write public members as arguments for source and header */
    if (define) {
	    if (t->kind == CX_COMPOSITE) {
		    s = c_apiParamSerializer();
		    cx_metaWalk(&s, cx_type(t), data);
		}
	}

    /* If there are no parameters, write 'void' */
    if (!data->parameterCount) {
        g_fileWrite(data->header, "void");
        g_fileWrite(data->source, "void");
    }

    /* Write closing brackets for argumentlist in source and header */
    g_fileWrite(data->header, ");\n");
    g_fileWrite(data->source, ") {\n");

    g_fileIndent(data->source);
    g_fileWrite(data->source, "%s%s _this;\n", id, t->reference ? "" : "*");
    if (scoped) {
		g_fileWrite(data->source, "_this = cx_declareChild(_parent, _name, %s_o);\n", id);
    } else {
	    g_fileWrite(data->source, "_this = cx_declare(%s_o);\n", id);
	}

    if (define) {
	    /* Member assignments */
	    s = c_apiAssignSerializer();
	    cx_metaWalk(&s, cx_type(t), data);

	    /* Define object */
	    g_fileWrite(data->source, "if (cx_define(_this)) {\n");
	    g_fileIndent(data->source);
	    g_fileWrite(data->source, "cx_release(_this);\n");
	    g_fileWrite(data->source, "_this = NULL;\n");
	    g_fileDedent(data->source);
	    g_fileWrite(data->source, "}\n");
	}

    g_fileWrite(data->source, "return _this;\n");
    g_fileDedent(data->source);
    g_fileWrite(data->source, "}\n\n");

    return 0;
}

cx_int16 c_apiTypeCreate(cx_type t, c_apiWalk_t *data) {
	return c_apiTypeCreateIntern(t, data, "create", FALSE, TRUE);
}

cx_int16 c_apiTypeCreateChild(cx_type t, c_apiWalk_t *data) {
	return c_apiTypeCreateIntern(t, data, "createChild", TRUE, TRUE);
}

cx_int16 c_apiTypeDeclare(cx_type t, c_apiWalk_t *data) {
	return c_apiTypeCreateIntern(t, data, "declare", FALSE, FALSE);
}

cx_int16 c_apiTypeDeclareChild(cx_type t, c_apiWalk_t *data) {
	return c_apiTypeCreateIntern(t, data, "declareChild", TRUE, FALSE);
}

cx_int16 c_apiTypeDefineIntern(cx_type t, c_apiWalk_t *data, cx_bool isUpdate, cx_bool doUpdate) {
    cx_id id;
    struct cx_serializer_s s;
    cx_string func = isUpdate ? doUpdate ? "update" : "set" : "define";

    data->parameterCount = 1;
    g_fullOid(data->g, t, id);

    if (isUpdate) {
    	g_fileWrite(data->header, "void ");
    	g_fileWrite(data->source, "void ");
    } else {
    	g_fileWrite(data->header, "cx_int16 ");
    	g_fileWrite(data->source, "cx_int16 ");    	
    }

    /* Function declaration */
    g_fileWrite(data->header, "%s__%s(%s%s _this", id, func, id, t->reference ? "" : "*");

    /* Function implementation */
    g_fileWrite(data->source, "%s__%s(%s%s _this", id, func, id, t->reference ? "" : "*");

    /* Write public members as arguments for source and header */
    if (t->kind == CX_COMPOSITE) {
	    s = c_apiParamSerializer();
	    cx_metaWalk(&s, cx_type(t), data);
	}

    if ((data->parameterCount == 1) && (t->kind != CX_COMPOSITE)) {
        g_fileWrite(data->source, ", %s value", id);
        g_fileWrite(data->header, ", %s value", id);
    }

    /* Write closing brackets for argumentlist in source and header */
    g_fileWrite(data->header, ");\n");
    g_fileWrite(data->source, ") {\n");
    g_fileIndent(data->source);

    if (isUpdate && doUpdate && (data->parameterCount > 1)) {
    	g_fileWrite(data->source, "cx_updateBegin(_this);\n");
    }

    /* Member assignments */
    if (data->parameterCount > 1) {
        s = c_apiAssignSerializer();
        cx_metaWalk(&s, cx_type(t), data);
    } else if (t->kind != CX_COMPOSITE) {
        g_fileWrite(data->source, "*_this = value;\n");
    } else if (isUpdate && !doUpdate) {
        g_fileWrite(data->source, "CX_UNUSED(_this);\n");
    }

    /* Define object */
    if (isUpdate && doUpdate) {
        if (data->parameterCount > 1) {
            g_fileWrite(data->source, "cx_updateEnd(_this);\n");
        } else {
            g_fileWrite(data->source, "cx_update(_this);\n");
        }
    } else if (!isUpdate) {
	    g_fileWrite(data->source, "return cx_define(_this);\n");
	}

    g_fileDedent(data->source);
    g_fileWrite(data->source, "}\n\n");
    
    return 0;
}

cx_int16 c_apiTypeDefine(cx_type t, c_apiWalk_t *data) {
	return c_apiTypeDefineIntern(t, data, FALSE, FALSE);
}

cx_int16 c_apiTypeUpdate(cx_type t, c_apiWalk_t *data) {
	return c_apiTypeDefineIntern(t, data, TRUE, TRUE);
}

cx_int16 c_apiTypeSet(cx_type t, c_apiWalk_t *data) {
    return c_apiTypeDefineIntern(t, data, TRUE, FALSE);
}

cx_int16 c_apiTypeStr(cx_type t, c_apiWalk_t *data) {
    cx_id id;
    cx_bool pointer = (t->kind == CX_COMPOSITE) && !t->reference;

    g_fullOid(data->g, t, id);

    /* Function declaration */
    g_fileWrite(data->header, "cx_string %s__str(%s%s value);\n", id, id, pointer ? "*" : "");

    /* Function implementation */
    g_fileWrite(data->source, "cx_string %s__str(%s%s value) {\n", id, id, pointer ? "*" : "");

    g_fileIndent(data->source);
    g_fileWrite(data->source, "cx_string result;\n", id);

    g_fileWrite(data->source, "cx_value v;\n", id);
    if (t->reference) {
        g_fileWrite(data->source, "cx_valueObjectInit(&v, value, cx_type(%s_o));\n", id);
    } else {
        g_fileWrite(data->source, "cx_valueValueInit(&v, NULL, cx_type(%s_o), &value);\n", id);
    }
    g_fileWrite(data->source, "result = cx_strv(&v, 0);\n");

    g_fileWrite(data->source, "return result;\n");
    g_fileDedent(data->source);
    g_fileWrite(data->source, "}\n\n");

    return 0;
}

static cx_int16 c_apiTypeInitIntern(cx_type t, c_apiWalk_t *data, cx_string func) {
    cx_id id;

    g_fullOid(data->g, t, id);

    /* Function declaration */
    g_fileWrite(data->header, "cx_int16 %s__%s(%s%s value);\n", id, func, id, t->reference ? "" : "*");

    /* Function implementation */
    g_fileWrite(data->source, "cx_int16 %s__%s(%s%s value) {\n", id, func, id, t->reference ? "" : "*");

    g_fileIndent(data->source);
    g_fileWrite(data->source, "cx_int16 result;\n", id);

    if (!strcmp(func, "init")) {
        g_fileWrite(data->source, "memset(value, 0, cx_type(%s_o)->size);\n", id);
    }

    if (t->reference) {
        g_fileWrite(data->source, "result = cx_%s(value, 0);\n", func);
    } else {
        g_fileWrite(data->source, "cx_value v;\n", id);
        g_fileWrite(data->source, "cx_valueValueInit(&v, NULL, cx_type(%s_o), value);\n", id);
        g_fileWrite(data->source, "result = cx_%sv(&v);\n", func);
    }

    g_fileWrite(data->source, "return result;\n");
    g_fileDedent(data->source);
    g_fileWrite(data->source, "}\n\n");

    return 0;
}

cx_int16 c_apiTypeInit(cx_type t, c_apiWalk_t *data) {
	return c_apiTypeInitIntern(t, data, "init");
}

cx_int16 c_apiTypeDeinit(cx_type t, c_apiWalk_t *data) {
	return c_apiTypeInitIntern(t, data, "deinit");
}

cx_int16 c_apiTypeFromStr(cx_type t, c_apiWalk_t *data) {
    cx_id id;

    g_fullOid(data->g, t, id);

    /* Function declaration */
    g_fileWrite(data->header, "%s%s %s__fromStr(%s%s value, cx_string str);\n", 
    	id, t->reference ? "" : "*", id, id, t->reference ? "" : "*");

    /* Function implementation */
    g_fileWrite(data->source, "%s%s %s__fromStr(%s%s value, cx_string str) {\n", 
    	id, t->reference ? "" : "*", id, id, t->reference ? "" : "*");

    g_fileIndent(data->source);
    g_fileWrite(data->source, "cx_fromStrp(&value, cx_type(%s_o), str);\n", id);
    g_fileWrite(data->source, "return value;\n");
    g_fileDedent(data->source);
    g_fileWrite(data->source, "}\n\n");

    return 0;
}

cx_int16 c_apiTypeCopyIntern(cx_type t, c_apiWalk_t *data, cx_string func, cx_bool outParam) {
    cx_id id;

    g_fullOid(data->g, t, id);

    /* Function declaration */
    g_fileWrite(data->header, "cx_int16 %s__%s(%s%s %sdst, %s%s src);\n", 
    	id, func, id, t->reference ? "" : "*", outParam ? "*" : "", id, t->reference ? "" : "*");

    /* Function implementation */
    g_fileWrite(data->source, "cx_int16 %s__%s(%s%s %sdst, %s%s src) {\n", 
    	id, func, id, t->reference ? "" : "*", outParam ? "*" : "", id, t->reference ? "" : "*");

    g_fileIndent(data->source);

	if (t->reference) {
		g_fileWrite(data->source, "return cx_%s(%sdst, src);\n", func, outParam ? "(cx_object*)" : "");
	} else {
		g_fileWrite(data->source, "cx_value v1, v2;\n", id);
	    g_fileWrite(data->source, "cx_valueValueInit(&v1, NULL, cx_type(%s_o), dst);\n", id);
	    g_fileWrite(data->source, "cx_valueValueInit(&v2, NULL, cx_type(%s_o), src);\n", id);
	    g_fileWrite(data->source, "return cx_%sv(&v1, &v2);\n", func);
	}

	g_fileDedent(data->source);
	g_fileWrite(data->source, "}\n\n");

	return 0;
}

cx_int16 c_apiTypeCopy(cx_type t, c_apiWalk_t *data) {
	return c_apiTypeCopyIntern(t, data, "copy", TRUE);
}

cx_int16 c_apiTypeCompare(cx_type t, c_apiWalk_t *data) {
	return c_apiTypeCopyIntern(t, data, "compare", FALSE);	
}