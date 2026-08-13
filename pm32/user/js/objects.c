static ObjectProperty *find_object_property(Object *object, const char *name)
{
    ObjectProperty *property = object ? object->first : 0;
    while (property) {
        if (text_equal(property->name, name)) return property;
        property = property->next;
    }
    return 0;
}

static void release_object(Object *object)
{
    ObjectProperty *property;
    ObjectProperty *next;
    if (!object) return;
    property = object->first;
    while (property) {
        next = property->next;
        if (property->type == VALUE_STRING && property->text)
            sys_free(property->text);
        sys_free(property);
        property = next;
    }
    sys_free(object);
}

static int copy_heap_text(char **destination, const char *text)
{
    unsigned length = myos_text_length(text);
    unsigned i;
    *destination = (char *)sys_malloc(length + 1);
    if (!*destination) return 0;
    for (i = 0; i <= length; ++i) (*destination)[i] = text[i];
    return 1;
}

static int parse_object_literal(Variable *variable)
{
    Object *object;
    ObjectProperty **tail;
    char property_name[MAX_NAME];
    char string_value[MAX_LINE];

    if (!accept("{")) return 0;
    object = (Object *)sys_malloc(sizeof(Object));
    if (!object) {
        error_text = "out of memory";
        return 1;
    }
    object->first = 0;
    tail = &object->first;
    if (!accept("}")) {
        for (;;) {
            ObjectProperty *property;
            unsigned i;
            if (!read_name(property_name) || !accept(":")) {
                error_text = "expected object property";
                break;
            }
            if (find_object_property(object, property_name)) {
                error_text = "duplicate object property";
                break;
            }
            property = (ObjectProperty *)sys_malloc(sizeof(ObjectProperty));
            if (!property) {
                error_text = "out of memory";
                break;
            }
            for (i = 0; i + 1 < MAX_NAME && property_name[i]; ++i)
                property->name[i] = property_name[i];
            property->name[i] = 0;
            property->type = VALUE_NUMBER;
            property->number = 0;
            property->text = 0;
            property->next = 0;
            *tail = property;
            tail = &property->next;
            if (read_string(string_value, sizeof(string_value))) {
                property->type = VALUE_STRING;
                if (!copy_heap_text(&property->text, string_value)) {
                    error_text = "out of memory";
                    break;
                }
            } else {
                property->number = parse_expression();
                if (error_text) break;
            }
            if (accept("}")) break;
            if (!accept(",")) {
                error_text = "expected ',' or '}'";
                break;
            }
        }
    }
    if (error_text) {
        release_object(object);
        return 1;
    }
    release_variable_value(variable);
    variable->type = VALUE_OBJECT;
    variable->object = object;
    return 1;
}
