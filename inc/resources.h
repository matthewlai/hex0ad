#ifndef RESOURCES_H
#define RESOURCES_H

#include <cstdint>

static constexpr const char* kTestActorPaths[] = {
    "structures/britons/civic_centre",

    "structures/mauryas/fortress",
    "structures/britons/fortress",
    "structures/persians/fortress",
    "structures/romans/fortress",
    "structures/spartans/fortress",

    "structures/persians/stable",
    "structures/persians/stable_elephant",
    "units/athenians/hero_infantry_javelinist_iphicrates",
    "units/romans/hero_cavalry_swordsman_maximus_r",
    "units/romans/cavalry_javelinist_a_m",
    "units/spartans/female_citizen",
    };

static constexpr const char* kTestTerrainPaths[] = {
    "biome-alpine/alpine_snow_a",
    "biome-desert/desert_city_tile",
    "biome-desert/desert_grass_a",
    "biome-desert/desert_farmland",
    "biome-polar/polar_ice",
    "biome-polar/polar_snow_a",
    "biome-savanna/savanna_tile_a",
    "biome-mediterranean/medit_sand_messy",
    "biome-tropic/tropic_beach_dry",
    "biome-mediterranean/medit_city_pavement",
    "biome-temperate/temp_road_overgrown",
    };

static constexpr const char* kInputPrefix = "0ad_assets/";
static constexpr const char* kOutputPrefix = "assets/";
static constexpr const char* kActorPathPrefix = "art/actors/";
static constexpr const char* kMeshPathPrefix = "art/meshes/";
static constexpr const char* kTexturePathPrefix = "art/textures/";
static constexpr const char* kActorTexturePathPrefix = "skins/";
static constexpr const char* kVariantPathPrefix = "art/variants/";
static constexpr const char* kTerrainPathPrefix = "art/terrains/";
static constexpr const char* kTerrainTexturePathPrefix = "terrain/";

static constexpr const char* kSkeletonPathPrefix = "art/skeletons/";

// How many joints can influence a vertex.
static constexpr const int kMaxSkinInfluences = 4;

static constexpr const uint8_t kNoInfluenceBoneId = 0xff;

#endif // RESOURCES_H
