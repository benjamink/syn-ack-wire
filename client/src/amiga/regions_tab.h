/* GENERATED from server/servers.fly.toml and fly.toml by tools/gen_regions.py -- do not edit. */
#ifndef REGIONS_TAB_H
#define REGIONS_TAB_H

#define REGION_HOST "syn-ack-wire.fly.dev"
#define REGION_SEATS_PORT 80
#define NREGIONS 4

struct region_entry { const char *name, *pace; unsigned short port; };
static const struct region_entry regions[NREGIONS] = {
    { "CHIBA-GRID-7", "relaxed pace", 6502 },
    { "NEO-KANSAI-1", "relaxed pace", 6503 },
    { "OSAKA-SPRAWL", "quicker pace", 6504 },
    { "SEOUL-NODE-2", "fast pace", 6505 },
};

#endif
