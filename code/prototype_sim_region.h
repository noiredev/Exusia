#if !defined(PROTOTYPE_SIM_REGION_H)

struct move_spec {
    bool32 UnitMaxAccelVector;
    real32 Speed;
    real32 Drag;
};

enum entity_type {
    EntityType_Null,

    EntityType_Space,

    EntityType_Hero,
    EntityType_Wall,
    EntityType_Familiar,
    EntityType_Monster,
    EntityType_Sword,
    EntityType_Stairwell,
};

#define HIT_POINT_SUB_COUNT 4
struct hit_point {
    // TODO: Bake this down into one variable
    uint8 Flags;
    uint8 FilledAmount;
};

// TODO: Rename sim_entity to entity
struct sim_entity;
union entity_reference {
    sim_entity *Ptr;
    uint32 Index;
};

enum sim_entity_flags {
    EntityFlag_Collides = (1 << 0),
    EntityFlag_Nonspatial = (1 << 1),
    EntityFlag_Moveable = (1 << 2),
    EntityFlag_ZSupported = (1 << 3),
    EntityFlag_Traversable = (1 << 4),

    EntityFlag_Simming = (1 << 30),
};

struct sim_entity_collision_volume {
    v3 OffsetP;
    v3 Dim;
};

struct sim_entity_collision_volume_group {
    sim_entity_collision_volume TotalVolume;

    uint32 VolumeCount;
    sim_entity_collision_volume *Volumes;
};

struct sim_entity {
    // NOTE: These are only for the sim region
    world_chunk *OldChunk;
    uint32 StorageIndex;
    bool32 Updatable;

    entity_type Type;
    uint32 Flags;

    v3 P;
    v3 dP;

    real32 Z;
    real32 dZ;

    real32 DistanceLimit;

    sim_entity_collision_volume_group *Collision;

    real32 Width, Height;

    uint32 FacingDirection;
    real32 tBob;

    int32 dAbsTileZ;

    uint32 HighEntityIndex;

    // TODO: Should hitpoints themselves be entities?
    uint32 HitPointMax;
    hit_point HitPoint[16];

    entity_reference Sword;

    // TODO: Only for stairs
    v2 WalkableDim;
    real32 WalkableHeight;
};

struct sim_entity_hash {
    sim_entity *Ptr;
    uint32 Index;
};

struct sim_region {
    world *World;
    real32 MaxEntityRadius;
    real32 MaxEntityVelocity;

    world_position Origin;
    rectangle3 Bounds;
    rectangle3 UpdatableBounds;

    uint32 MaxEntityCount;
    uint32 EntityCount;
    sim_entity *Entities;

    sim_entity_hash Hash[4096];
};

#define PROTOTYPE_SIM_REGION_H
#endif
