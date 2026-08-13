static int load_program(const char name[11])
{
    int result = load_file(name, user_memory(), 0xFFFFFFFFUL);
    if (result < 0) {
        return result;
    }
    if ((u32)result > USER_SIZE) {
        return -3;
    }
    return 0;
}
static int load_file(const char name[11], u8 *destination, u32 capacity)
{
    unsigned entry;
    unsigned i;
    const u8 *item;

    for (entry = 0; entry < 16; ++entry) {
        u32 cluster;
        u32 size;
        item = DISK_CACHE + entry * 32;
        if (!item[0]) {
            break;
        }
        for (i = 0; i < 11 && item[i] == (u8)name[i]; ++i) {}
        if (i != 11) {
            continue;
        }
        cluster=(u32)item[26]|((u32)item[27]<<8)|(((u32)item[20]|((u32)item[21]<<8))<<16);
        size=(u32)item[28]|((u32)item[29]<<8)|((u32)item[30]<<16)|((u32)item[31]<<24);
        if (cluster < 2) {
            return -2;
        }
        if (size > capacity) {
            return -3;
        }
        item=DISK_CACHE+(cluster-2)*512UL;
        {
            u32 words = size >> 2;
            u32 *to32 = (u32 *)destination;
            const u32 *from32 = (const u32 *)item;

            for (i = 0; i < words; ++i)
                to32[i] = from32[i];
            for (i = words << 2; i < size; ++i)
                destination[i] = item[i];
        }
        return (int)size;
    }
    return -1;
}

static int file_app_type(const char name[11])
{
    unsigned entry;
    unsigned i;

    for (entry = 0; entry < 16; ++entry) {
        const u8 *item = DISK_CACHE + entry * 32;

        if (!item[0]) {
            break;
        }
        for (i = 0; i < 11 && item[i] == (u8)name[i]; ++i) {}
        if (i == 11) {
            return item[12] == APP_GUI ? APP_GUI : APP_CONSOLE;
        }
    }
    return -1;
}

static int list_files(char *destination, unsigned capacity)
{
    unsigned entry;
    unsigned length = 0;

    for (entry = 0; entry < 16; ++entry) {
        const u8 *item = DISK_CACHE + entry * 32;
        unsigned i;
        unsigned base_end = 8;
        unsigned extension_end = 11;

        if (!item[0]) {
            break;
        }
        while (base_end && item[base_end - 1] == ' ') {
            --base_end;
        }
        while (extension_end > 8 && item[extension_end - 1] == ' ') {
            --extension_end;
        }
        if (length && length < capacity) {
            destination[length++] = '\n';
        }
        for (i = 0; i < base_end && length < capacity; ++i) {
            destination[length++] = (char)item[i];
        }
        if (extension_end > 8 && length < capacity) {
            destination[length++] = '.';
        }
        for (i = 8; i < extension_end && length < capacity; ++i) {
            destination[length++] = (char)item[i];
        }
    }
    return (int)length;
}
