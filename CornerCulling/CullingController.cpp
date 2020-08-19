#include "CullingController.h"
#include <chrono> 
#include <iostream>

CullingController::CullingController()
{
}

std::vector<vec3> testVertices = {
    vec3(1000, 2500, 1000),
    vec3(800, 2500, 1000),
    vec3(800, 2200, 1000),
    vec3(1000, 2200, 1000),
    vec3(1000, 2500, -1000),
    vec3(800, 2500, -1000),
    vec3(800, 2200, -1000),
    vec3(1000, 2200, -1000)
};
std::vector<vec3> testVertices2 = {
    vec3(2000, 2500, 1000),
    vec3(1800, 2500, 1000),
    vec3(1800, 2200, 1000),
    vec3(2000, 2200, 1000),
    vec3(2000, 2500, -1000),
    vec3(1800, 2500, -1000),
    vec3(1800, 2200, -1000),
    vec3(2000, 2200, -1000)
};

void CullingController::BeginPlay()
{
    // Add characters.
    // Add occluding cuboids.
    std::vector<Cuboid> MapCuboids = { Cuboid(testVertices), Cuboid(testVertices2) };
    for (Cuboid C : MapCuboids)
    {
        Cuboids.emplace_back(C);
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
    for (int i = 0; i < CharacterLocations.size(); i++)
    {
        if (IsAlive[i])
        {
            // TODO:
            //   Make displacement a function of game physics and state.
            float Latency = GetLatency(i);
            float MaxHorizontalDisplacement = Latency * 350;
            float MaxVerticalDisplacement = Latency * 200;
            // TEMPORARY
            MaxHorizontalDisplacement = 100;
            MaxVerticalDisplacement = 200;
            for (int j = 0; j < CharacterLocations.size(); j++)
            {
                if (VisibilityTimers[i][j] <= CullingPeriod
                    && IsAlive[j]
                    && (CharacterTeams[i] != CharacterTeams[j]))
                {
                    BundleQueue.emplace_back(
                        Bundle(
                            i,
                            j,
                            GetPossiblePeeks(
                                Bounds[i].CameraLocation,
                                Bounds[j].Center,
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
            if (CuboidCaches[B.PlayerI][B.EnemyI][k] != NULL)
            {
                if (
                    IsBlocking(
                        B.PossiblePeeks,
                        Bounds[B.EnemyI],
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
                    Bounds[B.EnemyI],
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
    std::vector<Bundle> Remaining;
    for (Bundle B : BundleQueue)
    {
        const Cuboid* CuboidP = CuboidTraverser.get()->traverse(
            OptSegment(
                Bounds[B.PlayerI].CameraLocation,
                Bounds[B.EnemyI].Center),
            B.PossiblePeeks,
            Bounds[B.EnemyI]);
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
    for (int i = 0; i < CharacterLocations.size(); i++)
    {
        if (IsAlive[i])
        {
            for (int j = 0; j < CharacterLocations.size(); j++)
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
    if (CharacterTeams[i] == CharacterTeams[j])
    {
        return true;
    }
    else
    {
        return VisibilityTimers[i][j] > 0;
    }
}

void CullingController::UpdateCharacters(int* Teams, float* CentersFlat, float* Yaws)
{
    for (int i = 0; i < CharacterLocations.size(); i++)
    {
        IsAlive[i] = (Teams[i] > 1);
        if (IsAlive[i])
        {
            CharacterTeams[i] = Teams[i];
            CharacterLocations[i].x = CentersFlat[i * 3];
            CharacterLocations[i].y = CentersFlat[i * 3 + 1];
            CharacterLocations[i].z = CentersFlat[i * 3 + 2];
            CharacterYaws[i] = Yaws[i];
            Bounds[i] = CharacterBounds(CharacterLocations[i], CharacterYaws[i]);
        }
    }
}
