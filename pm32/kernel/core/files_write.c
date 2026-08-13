static int ata_write_sector(u32 lba, const u8 *source)
{
    unsigned i;
    unsigned timeout = 1000000;

    while ((in8(0x1F7) & 0x80) && --timeout) {}
    if (!timeout) {
        return -1;
    }
    out8(0x1F6, (u8)(0xE0 | ((lba >> 24) & 0x0F)));
    out8(0x1F2, 1);
    out8(0x1F3, (u8)lba);
    out8(0x1F4, (u8)(lba >> 8));
    out8(0x1F5, (u8)(lba >> 16));
    out8(0x1F7, 0x30);
    timeout = 1000000;
    while (!(in8(0x1F7) & 0x08) && --timeout) {}
    if (!timeout || (in8(0x1F7) & 1)) {
        return -1;
    }
    for (i = 0; i < 256; ++i) {
        out16(0x1F0, ((const u16 *)source)[i]);
    }
    out8(0x1F7, 0xE7);
    timeout = 1000000;
    while ((in8(0x1F7) & 0x80) && --timeout) {}
    return timeout ? 0 : -1;
}

static int ata_read_sector(u32 lba, u8 *destination)
{
    unsigned i;
    unsigned timeout = 1000000;

    while ((in8(0x1F7) & 0x80) && --timeout) {}
    if (!timeout) {
        return -1;
    }
    out8(0x1F6, (u8)(0xE0 | ((lba >> 24) & 0x0F)));
    out8(0x1F2, 1);
    out8(0x1F3, (u8)lba);
    out8(0x1F4, (u8)(lba >> 8));
    out8(0x1F5, (u8)(lba >> 16));
    out8(0x1F7, 0x20);
    timeout = 1000000;
    while (!(in8(0x1F7) & 0x08) && --timeout) {}
    if (!timeout || (in8(0x1F7) & 1)) {
        return -1;
    }
    for (i = 0; i < 256; ++i) {
        ((u16 *)destination)[i] = in16(0x1F0);
    }
    return 0;
}

static void set_u32(u8 *destination, u32 value)
{
    destination[0] = (u8)value;
    destination[1] = (u8)(value >> 8);
    destination[2] = (u8)(value >> 16);
    destination[3] = (u8)(value >> 24);
}

static int write_file(const char name[11], const u8 *source, u32 size)
{
    u8 *root = (u8 *)DISK_CACHE;
    unsigned entry;
    unsigned i;
    unsigned clusters;
    u32 first_cluster = 0;
    unsigned allocated_clusters = 8;

    if (size > 4096) {
        return -3;
    }
    for (entry = 0; entry < 32; ++entry) {
        u8 *item = root + entry * 32;
        u32 cluster;

        if (!item[0]) {
            break;
        }
        cluster = (u32)item[26] | ((u32)item[27] << 8) |
                  (((u32)item[20] | ((u32)item[21] << 8)) << 16);
        for (i = 0; i < 11 && item[i] == (u8)name[i]; ++i) {}
        if (i == 11) {
            first_cluster = cluster;
            break;
        }
    }
    if (entry == 32) {
        return -4;
    }
    clusters = (unsigned)((size + 511) / 512);
    if (!clusters) {
        clusters = 1;
    }
    if (!first_cluster) {
        first_cluster = 400UL + entry * 8UL;
        for (i = 0; i < 32; ++i) {
            root[entry * 32 + i] = 0;
        }
        for (i = 0; i < 11; ++i) {
            root[entry * 32 + i] = (u8)name[i];
        }
        root[entry * 32 + 11] = 0x20;
        root[entry * 32 + 20] = (u8)(first_cluster >> 16);
        root[entry * 32 + 21] = (u8)(first_cluster >> 24);
        root[entry * 32 + 26] = (u8)first_cluster;
        root[entry * 32 + 27] = (u8)(first_cluster >> 8);
    }
    set_u32(root + entry * 32 + 28, size);
    for (i = 0; i < allocated_clusters; ++i) {
        unsigned j;
        u32 cluster = first_cluster + i;
        u32 remaining = size > i * 512UL ? size - i * 512UL : 0;
        u32 count = remaining > 512 ? 512 : remaining;
        u32 fat_sector = cluster / 128;
        u32 fat_offset = (cluster % 128) * 4;

        for (j = 0; j < 512; ++j) {
            sector_buffer[j] = j < count ? source[i * 512UL + j] : 0;
        }
        if (ata_write_sector(FAT_DATA_LBA + cluster - 2, sector_buffer) < 0) {
            return -5;
        }
        if (ata_read_sector(FAT_LBA + fat_sector, sector_buffer) < 0) {
            return -5;
        }
        set_u32(sector_buffer + fat_offset,
                i + 1 == allocated_clusters
                    ? 0x0FFFFFFFUL
                    : cluster + 1);
        if (ata_write_sector(FAT_LBA + fat_sector, sector_buffer) < 0 ||
            ata_write_sector(FAT_LBA + 1009UL + fat_sector, sector_buffer) < 0) {
            return -5;
        }
    }
    if (ata_write_sector(FAT_DATA_LBA + entry / 16,
                         root + (entry / 16) * 512) < 0) {
        return -5;
    }
    return (int)size;
}
