#include "CullingController.h"
#include <chrono> 
#include <iostream>
#include "CullingIO.h"

#include "../extension.h"

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
    for (auto c: FileToCuboids(mapName))
    {
        Cuboids.emplace_back(c);
    }

    // Build the cuboid BVH.
    if (Cuboids.size() > 0)
    {
        FastBVH::BuildStrategy<float, 1> Builder;
        CuboidBoxConverter Converter;
        CuboidBVH = std::make_unique
            <FastBVH::BVH<float, Cuboid>>
            (Builder(Cuboids, Converter));
        CuboidTraverser = std::make_unique
            <Traverser<float, decltype(Intersector)>>
            (*CuboidBVH.get(), Intersector);
    }

    // TODO: Add occluding spheres, ma
}

void CullingController::Tick()
{
    TotalTicks++;
    Cull();
}

void CullingController::BenchmarkCull()
{
    auto Start = std::chrono::high_resolution_clock::now();
    Cull();
    auto Stop = std::chrono::high_resolution_clock::now();
    int Delta = int(std::chrono::duration_cast<std::chrono::microseconds>(Stop - Start).count());
    TotalTime += Delta;
    RollingTotalTime += Delta;
    RollingMaxTime = std::max(RollingMaxTime, Delta);
    if ((TotalTicks % RollingWindowLength) == 0)
    {
        RollingAverageTime = RollingTotalTime / RollingWindowLength;
        std::cout << "Average time to cull (microseconds): " << int(TotalTime / TotalTicks) << "\n";
        std::cout << "Rolling average time to cull (microseconds): " << RollingAverageTime << "\n";
        std::cout << "Rolling max time to cull (microseconds): " << RollingMaxTime << "\n";
        RollingTotalTime = 0;
        RollingMaxTime = 0;
    }
}

void CullingController::Cull()
{
    PopulateBundles();
    CullWithCache();
    //CullWithSpheres();
    CullWithCuboids();
    UpdateVisibility();
}

void CullingController::PopulateBundles()
{
    BundleQueue.clear();
    for (auto i = 0U; i < Characters.size(); i++)
    {
        // Staggers culling across each CullingPeriod
        if (!IsAlive[i] || (((i + TotalTicks) % CullingPeriod) != 0))
        {
            continue;
        }
        // Amount of lookahead to account for latency (milliseconds).
        const int lookahead = std::min(GetLatency(i), maxLookahead);
        // Maximum player speed in units/millisecond.
        float speed = 0.001f * std::min(
            Characters[i].Speed + 0.5f * lookahead,
            MAX_PLAYER_SPEED);
        float MaxHorizontalDisplacement = lookahead * speed;
        float MaxVerticalDisplacement = 20;
        for (auto j = 0U; j < Characters.size(); j++)
        {
            if (VisibilityTimers[i][j] <= CullingPeriod
                && IsAlive[j]
                && !sameTeam(i, j))
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
    //std::cout << BundleQueue.size() << "\n";
}

// TODO:
//   Integrate with server latency estimation tools.
int CullingController::GetLatency(int i)
{
    IGamePlayer *pPlayer = playerhelpers->GetGamePlayer(i);
    if (!pPlayer)
	{
		return 200;
	}
	else if (!pPlayer->IsConnected())
	{
		return 200;
	}
	else if (pPlayer->IsFakeClient())
	{
		return 0;
	};

    return 200;
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
        // Traverse the BVH to search for a cuboid that intersects the bundle.
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
    // They represent unblocked sightlines to enemies that should be revealed.
    for (Bundle B : BundleQueue)
    {
        VisibilityTimers[B.PlayerI][B.EnemyI] = VisibilityTimerMax;
    }
    BundleQueue.clear();
    // Reveal
    for (auto i = 0U; i < Characters.size(); i++)
    {
        if (IsAlive[i])
        {
            for (auto j = 0U; j < Characters.size(); j++)
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
    float* Pitches,
    float* Speeds)
{
    for (auto i = 0U; i < Characters.size(); i++)
    {
        IsAlive[i] = (Teams[i] > 1);
        if (IsAlive[i])
        {
            Characters[i] = CharacterBounds(
                Teams[i],
                vec3(EyesFlat[i * 3], EyesFlat[i * 3 + 1], EyesFlat[i * 3 + 2]),
                vec3(BasesFlat[i * 3], BasesFlat[i * 3 + 1], BasesFlat[i * 3 + 2]),
                Yaws[i],
                Pitches[i],
                Speeds[i]);
        }
    }
}
