#include "CullingController.h"
#include <chrono> 
#include <iostream>
#include "CullingIO.h"

CullingController::CullingController() {}

void CullingController::BeginPlay(char* mapName)
{
    MapName = mapName;
    Cuboids.clear();
    memset(
        CuboidCaches,
        0,
        MAX_CHARACTERS * MAX_CHARACTERS * CUBOID_CACHE_SIZE * sizeof(Cuboid*));
    // Add occluding cuboids.
    for (std::vector<vec3> cuboidVertices : FileToCuboidVertices(mapName))
    {
        Cuboids.emplace_back(Cuboid(cuboidVertices));
    }
    if (Cuboids.size() > 0)
    {
        // Build the cuboid BVH.
        FastBVH::BuildStrategy<float, 1> Builder;
        CuboidBoxConverter Converter;
        CuboidBVH = std::make_unique
            <FastBVH::BVH<float, Cuboid>>
            (Builder(Cuboids, Converter));
        CuboidTraverser = std::make_unique
            <Traverser<float, decltype(Intersector)>>
            (*CuboidBVH.get(), Intersector);
    }
    // Add occluding spheres.
}

void CullingController::Tick()
{
    TotalTicks++;
    BenchmarkCull();
}

void CullingController::BenchmarkCull()
{
    auto Start = std::chrono::high_resolution_clock::now();
    Cull();
    auto Stop = std::chrono::high_resolution_clock::now();
    UpdateVisibility();
    int Delta = std::chrono::duration_cast<std::chrono::microseconds>(Stop - Start).count();
    TotalTime += Delta;
    RollingTotalTime += Delta;
    RollingMaxTime = std::max(RollingMaxTime, Delta);
    if ((TotalTicks % RollingWindowLength) == 0)
    {
        RollingAverageTime = RollingTotalTime / RollingWindowLength;

        // "Average time to cull (microseconds): %.2f\n" int(TotalTime / TotalTicks;
        // "Rolling average time to cull (microseconds): "  RollingAverageTime;
        //"Rolling max time to cull (microseconds): " RollingMaxTime;

        RollingTotalTime = 0;
        RollingMaxTime = 0;
    }
}

void CullingController::Cull()
{
    // TODO:
    //   When running multiple servers per CPU, consider staggering
    //   culling periods to avoid lag spikes.
    if ((TotalTicks % CullingPeriod) == 0)
    {
        PopulateBundles();
        CullWithCache();
        //CullWithSpheres();
        CullWithCuboids();
    }
}

void CullingController::PopulateBundles()
{
    BundleQueue.clear();
    for (int i = 0; i < Characters.size(); i++)
    {
        if (IsAlive[i])
        {
            // TODO:
            //   Make displacement a function of game physics and state.
            float Latency = GetLatency(i);
            float MaxHorizontalDisplacement = Latency * 350;
            float MaxVerticalDisplacement = Latency * 200;
            // TEMPORARY
            MaxHorizontalDisplacement = 8;
            MaxVerticalDisplacement = 8;
            for (int j = 0; j < Characters.size(); j++)
            {
                if (VisibilityTimers[i][j] <= CullingPeriod
                    && IsAlive[j]
                    && (!sameTeam(i, j)))
                {
                    BundleQueue.emplace_back(
                        Bundle(
                            i,
                            j,
                            GetPossiblePeeks(
                                Characters[i].Eye,
                                Characters[j].Eye,
                                MaxHorizontalDisplacement,
                                MaxVerticalDisplacement)));
                }
            }
        }
    }
}

// Estimates the latency of the client controlling character i in seconds.
// The estimate should be greater than the expected latency,
// as underestimating latency results in underestimated peeks,
// which could result in popping.
// TODO:
//   Integrate with server latency estimation tools.
float CullingController::GetLatency(int i)
{
    return float(CULLING_SIMULATED_LATENCY) / SERVER_TICKRATE;
}

std::vector<vec3> CullingController::GetPossiblePeeks(
    const vec3& PlayerCameraLocation,
    const vec3& EnemyLocation,
    float MaxDeltaHorizontal,
    float MaxDeltaVertical)
{
    std::vector<vec3> Corners;
    vec3 PlayerToEnemy = glm::normalize(EnemyLocation - PlayerCameraLocation);
    // Displacement parallel to the XY plane and perpendicular to PlayerToEnemy.
    vec3 Horizontal =
        MaxDeltaHorizontal * vec3(-PlayerToEnemy.y, PlayerToEnemy.x, 0);
    vec3 Vertical = vec3(0, 0, MaxDeltaVertical);
    Corners.emplace_back(PlayerCameraLocation + Horizontal + Vertical);
    Corners.emplace_back(PlayerCameraLocation - Horizontal + Vertical);
    Corners.emplace_back(PlayerCameraLocation - Horizontal - Vertical);
    Corners.emplace_back(PlayerCameraLocation + Horizontal - Vertical);
    return Corners;
}

void CullingController::CullWithCache()
{
    std::vector<Bundle> Remaining;
    for (Bundle B : BundleQueue)
    {
        bool Blocked = false;
        for (int k = 0; k < CUBOID_CACHE_SIZE; k++)
        {
        // Note: does not check if pointer are valid--deleted cuboids
            if (CuboidCaches[B.PlayerI][B.EnemyI][k] != NULL)
            {
                if (
                    IsBlocking(
                        B.PossiblePeeks,
                        Characters[B.EnemyI],
                        CuboidCaches[B.PlayerI][B.EnemyI][k]))
                {
                    Blocked = true;
                    CacheTimers[B.PlayerI][B.EnemyI][k] = TotalTicks;
                    break;
                }
            }
        }
        if (!Blocked)
        {
            Remaining.emplace_back(B);
        }
    }
    BundleQueue = Remaining;
}

void CullingController::CullWithSpheres()
{
    std::vector<Bundle> Remaining;
    for (Bundle B : BundleQueue)
    {
        bool Blocked = false;
        for (Sphere S : Spheres)
        {
            if (
                IsBlocking(
                    B.PossiblePeeks,
                    Characters[B.EnemyI],
                    S))
            {
                Blocked = true;
                break;
            }
        }
        if (!Blocked)
        {
            Remaining.emplace_back(B);
        }
    }
    BundleQueue = Remaining;
}

void CullingController::CullWithCuboids()
{
    if (Cuboids.size() == 0)
    {
        return;
    }
    std::vector<Bundle> Remaining;
    for (Bundle B : BundleQueue)
    {
        const Cuboid* CuboidP = CuboidTraverser.get()->traverse(
            OptSegment(
                Characters[B.PlayerI].Eye,
                Characters[B.EnemyI].Eye),
            B.PossiblePeeks,
            Characters[B.EnemyI]);
        if (CuboidP != NULL)
        {
            int MinI = ArgMin(
                CacheTimers[B.PlayerI][B.EnemyI],
                CUBOID_CACHE_SIZE);
            CuboidCaches[B.PlayerI][B.EnemyI][MinI] = CuboidP;
            CacheTimers[B.PlayerI][B.EnemyI][MinI] = TotalTicks;
        }
        else
        {
            Remaining.emplace_back(B);
        }
    }
    BundleQueue = Remaining;
}

// Increments visibility timers of bundles that were not culled,
// and reveals enemies with positive visibility timers.
void CullingController::UpdateVisibility()
{
    // There are bundles remaining from the culling pipeline.
    for (Bundle B : BundleQueue)
    {
        VisibilityTimers[B.PlayerI][B.EnemyI] = VisibilityTimerMax;
    }
    BundleQueue.clear();
    // Reveal
    for (int i = 0; i < Characters.size(); i++)
    {
        if (IsAlive[i])
        {
            for (int j = 0; j < Characters.size(); j++)
            {
                if (IsAlive[j] && VisibilityTimers[i][j] > 0)
                {
                    VisibilityTimers[i][j]--;
                }
            }
        }
    }
}

bool CullingController::IsVisible(int i, int j)
{
    if (sameTeam(i, j))
    {
        return true;
    }
    else
    {
        return VisibilityTimers[i][j] > 0;
    }
}

bool CullingController::sameTeam(int i, int j)
{
    if (i < Characters.size() && j < Characters.size())
    {
        return Characters[i].Team == Characters[j].Team;
    }
    else
    {
        return false;
    }
}

void CullingController::UpdateCharacters(
    int* Teams,
    float* EyesFlat,
    float* BasesFlat,
    float* Yaws,
    float* Pitches)
{
    for (int i = 0; i < Characters.size(); i++)
    {
        IsAlive[i] = (Teams[i] > 1);
        if (IsAlive[i])
        {
            Characters[i] = CharacterBounds(
                Teams[i],
                vec3(EyesFlat[i * 3], EyesFlat[i * 3 + 1], EyesFlat[i * 3 + 2]),
                vec3(BasesFlat[i * 3], BasesFlat[i * 3 + 1], BasesFlat[i * 3 + 2]),
                Yaws[i],
                Pitches[i]);
        }
    }
}
