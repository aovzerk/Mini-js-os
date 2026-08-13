static void clear_value(JsValue *value)
{
    value->type = VALUE_NUMBER;
    value->number = 0;
    value->text = 0;
    value->object = 0;
    value->array = 0;
}

static ObjectProperty *find_object_property(Object *object, const char *name)
{
    ObjectProperty *property = object ? object->first : 0;
    while (property) {
        if (text_equal(property->name, name)) return property;
        property = property->next;
    }
    return 0;
}

static Object *create_object(void)
{
    Object *object = (Object *)sys_malloc(sizeof(Object));
    if (object) object->first = 0;
    return object;
}

static int object_set_number(Object *object, const char *name, long number)
{
    ObjectProperty *property = (ObjectProperty *)sys_malloc(sizeof(ObjectProperty));
    ObjectProperty **tail;
    unsigned i;
    if (!property) return 0;
    for (i = 0; i + 1 < MAX_NAME && name[i]; ++i) property->name[i] = name[i];
    property->name[i] = 0;
    clear_value(&property->value);
    property->value.number = number;
    property->next = 0;
    tail = &object->first;
    while (*tail) tail = &(*tail)->next;
    *tail = property;
    return 1;
}

static void release_value(JsValue *value)
{
    if (value->type == VALUE_STRING && value->text) sys_free(value->text);
    else if (value->type == VALUE_OBJECT && value->object)
        release_object(value->object);
    else if (value->type == VALUE_ARRAY && value->array)
        release_array(value->array);
    clear_value(value);
}

static void release_object(Object *object)
{
    ObjectProperty *property;
    ObjectProperty *next;
    if (!object) return;
    property = object->first;
    while (property) {
        next = property->next;
        release_value(&property->value);
        sys_free(property);
        property = next;
    }
    sys_free(object);
}

static void release_array(JsArray *array)
{
    unsigned i;
    if (!array) return;
    for (i = 0; i < array->length; ++i) release_value(&array->items[i]);
    if (array->items) sys_free(array->items);
    sys_free(array);
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

static int clone_value(JsValue *source, JsValue *destination)
{
    unsigned i;
    clear_value(destination);
    destination->type = source->type;
    destination->number = source->number;
    if (source->type == VALUE_STRING) {
        if (!copy_heap_text(&destination->text, source->text)) return 0;
    } else if (source->type == VALUE_OBJECT) {
        ObjectProperty *from = source->object->first;
        ObjectProperty **tail;
        destination->object = (Object *)sys_malloc(sizeof(Object));
        if (!destination->object) return 0;
        destination->object->first = 0;
        tail = &destination->object->first;
        while (from) {
            ObjectProperty *to = (ObjectProperty *)sys_malloc(sizeof(ObjectProperty));
            if (!to) return 0;
            for (i = 0; i < MAX_NAME; ++i) to->name[i] = from->name[i];
            clear_value(&to->value);
            to->next = 0;
            *tail = to;
            tail = &to->next;
            if (!clone_value(&from->value, &to->value)) return 0;
            from = from->next;
        }
    } else if (source->type == VALUE_ARRAY) {
        destination->array = (JsArray *)sys_malloc(sizeof(JsArray));
        if (!destination->array) return 0;
        destination->array->length = source->array->length;
        destination->array->items =
            (JsValue *)sys_malloc(sizeof(JsValue) * MAX_ARRAY_ITEMS);
        if (!destination->array->items) return 0;
        for (i = 0; i < MAX_ARRAY_ITEMS; ++i)
            clear_value(&destination->array->items[i]);
        for (i = 0; i < source->array->length; ++i)
            if (!clone_value(&source->array->items[i],
                             &destination->array->items[i])) return 0;
    }
    return 1;
}

static int clone_variable_value(Variable *source, JsValue *destination)
{
    JsValue value;
    value.type = source->type;
    value.number = source->value;
    value.text = source->text;
    value.object = source->object;
    value.array = source->array;
    return clone_value(&value, destination);
}

static void move_value_to_variable(JsValue *source, Variable *destination)
{
    release_variable_value(destination);
    destination->type = source->type;
    destination->value = source->number;
    destination->text = source->text;
    destination->object = source->object;
    destination->array = source->array;
    clear_value(source);
}

static Object *parse_object_value(void);
static JsArray *parse_array_value(void);

static int parse_js_value(JsValue *value)
{
    char string_value[MAX_LINE];
    clear_value(value);
    if (read_string(string_value, sizeof(string_value))) {
        value->type = VALUE_STRING;
        if (!copy_heap_text(&value->text, string_value)) error_text = "out of memory";
    } else if (accept("{")) {
        value->type = VALUE_OBJECT;
        value->object = parse_object_value();
    } else if (accept("[")) {
        value->type = VALUE_ARRAY;
        value->array = parse_array_value();
    } else {
        value->number = parse_expression();
    }
    return !error_text;
}

static Object *parse_object_value(void)
{
    Object *object = (Object *)sys_malloc(sizeof(Object));
    ObjectProperty **tail;
    char property_name[MAX_NAME];
    if (!object) { error_text = "out of memory"; return 0; }
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
            if (!property) { error_text = "out of memory"; break; }
            for (i = 0; i + 1 < MAX_NAME && property_name[i]; ++i)
                property->name[i] = property_name[i];
            property->name[i] = 0;
            clear_value(&property->value);
            property->next = 0;
            *tail = property;
            tail = &property->next;
            if (!parse_js_value(&property->value)) break;
            if (accept("}")) break;
            if (!accept(",")) { error_text = "expected ',' or '}'"; break; }
        }
    }
    if (error_text) { release_object(object); return 0; }
    return object;
}

static JsArray *parse_array_value(void)
{
    JsArray *array = (JsArray *)sys_malloc(sizeof(JsArray));
    unsigned i;
    if (!array) { error_text = "out of memory"; return 0; }
    array->length = 0;
    array->items = (JsValue *)sys_malloc(sizeof(JsValue) * MAX_ARRAY_ITEMS);
    if (!array->items) { sys_free(array); error_text = "out of memory"; return 0; }
    for (i = 0; i < MAX_ARRAY_ITEMS; ++i) clear_value(&array->items[i]);
    if (!accept("]")) {
        for (;;) {
            if (array->length >= MAX_ARRAY_ITEMS) {
                error_text = "array too large";
                break;
            }
            if (!parse_js_value(&array->items[array->length])) break;
            ++array->length;
            if (accept("]")) break;
            if (!accept(",")) { error_text = "expected ',' or ']'"; break; }
        }
    }
    if (error_text) { release_array(array); return 0; }
    return array;
}

static int parse_object_literal(Variable *variable)
{
    Object *object;
    if (!accept("{")) return 0;
    object = parse_object_value();
    if (!object) return 1;
    release_variable_value(variable);
    variable->type = VALUE_OBJECT;
    variable->object = object;
    return 1;
}

static int parse_array_literal(Variable *variable)
{
    JsArray *array;
    if (!accept("[")) return 0;
    array = parse_array_value();
    if (!array) return 1;
    release_variable_value(variable);
    variable->type = VALUE_ARRAY;
    variable->array = array;
    return 1;
}

static int execute_object_assignment(void)
{
    const char *saved = cursor;
    char variable_name[MAX_NAME];
    char property_name[MAX_NAME];
    Variable *variable;
    ObjectProperty *property;
    ObjectProperty **tail;
    unsigned i;
    if (!read_name(variable_name) || !accept(".") ||
        !read_name(property_name) || !accept("=")) {
        cursor = saved;
        return 0;
    }
    variable = find_variable(variable_name, 0);
    if (!variable || variable->type != VALUE_OBJECT) {
        error_text = "object variable expected";
        return 1;
    }
    property = find_object_property(variable->object, property_name);
    if (!property) {
        property = (ObjectProperty *)sys_malloc(sizeof(ObjectProperty));
        if (!property) { error_text = "out of memory"; return 1; }
        for (i = 0; i + 1 < MAX_NAME && property_name[i]; ++i)
            property->name[i] = property_name[i];
        property->name[i] = 0;
        clear_value(&property->value);
        property->next = 0;
        tail = &variable->object->first;
        while (*tail) tail = &(*tail)->next;
        *tail = property;
    } else release_value(&property->value);
    parse_js_value(&property->value);
    return 1;
}
