/**
    @author Andrew Huang (87andrewh)
*/

#pragma once
#include "GeometricPrimitives.h"
#include "FastBVH.h"
#include <vector>
#include <deque>
#include <memory>
#include <glm/vec3.hpp>
using glm::vec3;

constexpr int SERVER_TICKRATE = 120;
// Simulated latency in ticks.
constexpr int CULLING_SIMULATED_LATENCY = 12;

// Number of peeks in each Bundle.
constexpr int NUM_PEEKS = 4;
// Maximum number of characters in a game.
// Must equal 65 to align with SourceMod plugin.
constexpr int MAX_CHARACTERS = 65;
// Number of cuboids in each entry of the cuboid cache array.
constexpr int CUBOID_CACHE_SIZE = 3;

/**
 *  Controls all occlusion culling logic.
 */
class CullingController
{
    std::vector<vec3> CharacterLocations = std::vector<vec3>(MAX_CHARACTERS + 1);
    std::vector<float> CharacterYaws = std::vector<float>(MAX_CHARACTERS + 1);
    // Bounding volumes of all characters.
    std::vector<CharacterBounds> Bounds = std::vector<CharacterBounds>(MAX_CHARACTERS + 1);
    std::vector<bool> IsAlive = std::vector<bool>(MAX_CHARACTERS + 1);
    // Tracks team of each character.
    std::vector<int> CharacterTeams = std::vector<int>(MAX_CHARACTERS + 1);
    // Cache of pointers to cuboids that recently blocked LOS from
    // player i to enemy j. Accessed by CuboidCaches[i][j].
    const Cuboid* CuboidCaches[MAX_CHARACTERS][MAX_CHARACTERS][CUBOID_CACHE_SIZE] = { 0 };
    // Timers that track the last time a cuboid in the cache blocked LOS.
    int CacheTimers[MAX_CHARACTERS][MAX_CHARACTERS][CUBOID_CACHE_SIZE] = { 0 };
    // All occluding cuboids in the map.
    std::vector<Cuboid> Cuboids;
    // Bounding volume hierarchy containing cuboids.
    std::unique_ptr<FastBVH::BVH<float, Cuboid>> CuboidBVH{};
    CuboidIntersector Intersector;
    // Note: Could be nice to use std::optional with C++17.
    std::unique_ptr
        <Traverser<float, decltype(Intersector)>>
        CuboidTraverser{};
    // All occluding spheres in the map.
    std::vector<Sphere> Spheres;
    // Queues of line-of-sight bundles needing to be culled.
    std::vector<Bundle> BundleQueue;

    // How many frames pass between each cull.
    int CullingPeriod = 4;
    // Stores how many ticks character j remains visible to character i for.
    int VisibilityTimers[MAX_CHARACTERS][MAX_CHARACTERS] = { 0 };
    // How many ticks an enemy stays visible for after being revealed.
    int VisibilityTimerMax = CullingPeriod * 3;
    // Used to calculate short rolling average of frame times.
    float RollingTotalTime = 0;
    float RollingAverageTime = 0;
    // Stores maximum culling time in rolling window.
    int RollingMaxTime = 0;
    // Number of ticks in the rolling window.
    int RollingWindowLength = SERVER_TICKRATE;
    // Total ticks since game start.
    int TotalTicks = 0;
    // Stores total culling time to calculate an overall average.
    int TotalTime = 0;

    // Cull while gathering and reporting runtime statistics.
    void BenchmarkCull();
    // Cull visibility for all player, enemy pairs.
    void Cull();
    // Calculates all bundles of lines of sight between characters,
    // adding them to the BundleQueue for culling.
    void PopulateBundles();
    // Culls all bundles with each player's cache of occluders.
    void CullWithCache();
    // Culls queued bundles with occluding spheres.
    void CullWithSpheres();
    // Culls queued bundles with occluding cuboids.
    void CullWithCuboids();
    // Gets corners of the rectangle encompassing a player's possible peeks
    // on an enemy--in the plane normal to the line of sight.
    // When facing along the vector from player to enemy, Corners are indexed
    // starting from the top right, proceeding counter-clockwise.
    // NOTE:
    //   Inaccurate on very wide enemies, as the most aggressive angle to peek
    //   the left of an enemy is actually perpendicular to the leftmost point
    //   of the enemy, not its center.
    static std::vector<vec3> GetPossiblePeeks(
        const vec3& PlayerCameraLocation,
        const vec3& EnemyLocation,
        float MaxDeltaHorizontal,
        float MaxDeltaVertical);
    // Gets the estimated latency of player i in seconds.
    float GetLatency(int i);
    // Converts culling results into changes in in-game visibility.
    void UpdateVisibility();

public:
    CullingController();
    void BeginPlay();
    void Tick();
    // Returns if player i can see player j
    bool IsVisible(int i, int j);
    void UpdateCharacters(int* Teams, float* CentersFlat, float* Yaws);

    // Mark a vector. For debugging.
    static inline void Markvec3(const vec3& v)
    {
    }

    // Draw a line between two vectors. For debugging.	
    static inline void ConnectVectors(
        const vec3& v1,
        const vec3& v2,
        bool Persist = false,
        float Lifespan = 0.1f,
        float Thickness = 2.0f)
    {
    }

    // Get the index of the minimum element in an array.
    static inline int ArgMin(int input[], int length)
    {
        int minVal = INT_MAX;
        int minI = 0;
        for (int i = 0; i < length; i++)
        {
            if (input[i] < minVal)
            {
                minVal = input[i];
                minI = i;
            }
        }
        return minI;
    }
};
