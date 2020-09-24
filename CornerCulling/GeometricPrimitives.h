#pragma once

#include <immintrin.h>
#include <algorithm>
#include <vector>
#include <glm/glm.hpp>
#include <glm/vec3.hpp>
#include <glm/gtx/rotate_vector.hpp>
using glm::vec3;

constexpr float PI = 3.141592653589793f;
// Number of vertices and faces of a cuboid.
constexpr char CUBOID_V = 8;
constexpr char CUBOID_F = 6;
// Number of vertices in a face of a cuboid.
constexpr char CUBOID_FACE_V = 4;

// Maps a Face with index i's j-th vertex onto a Cuboid vertex index.
// Faces are indexed as such:
//	   .+---------+  
//	 .' |  0    .'|  
//	+---+-----+'  |  
//	|   |    3|   |  
//	| 4 |     | 2 |  
//	|   |1    |   |  
//	|  ,+-----+---+  
//	|.'    5  | .'   
//	+---------+'    
constexpr char FaceCuboidMap[6][4] =
{
    {0, 1, 2, 3},
    {2, 6, 7, 3},
    {0, 3, 7, 4},
    {0, 4, 5, 1},
    {1, 5, 6, 2},
    {4, 7, 6, 5}
};

// Face of a convex polyhedron.
struct Face
{
    // Point on the face
    vec3 Point = vec3{ 0, 0, 0};
    // Outward normal of the face
	vec3 Normal = vec3{0, 0, 1};

    Face(vec3 Point, vec3 Normal)
    {
        this->Point = Point;
        this->Normal = Normal;
    }
};

// A six-sided polyhedron defined by 8 vertices.
// A valid configuration of vertices is user-enforced.
// For example, all vertices of a face should be coplanar.
struct Cuboid
{
    // Min and max of AABB surrounding the Cuboid
    vec3 AABBMin;
    vec3 AABBMax;
    // Faces that define the cuboid
	std::vector<Face> Faces;

	// Constructs a cuboid from a list of vertices.
	// Vertices are ordered and indexed as such:
	//	    .1------0
	//	  .' |    .'|
	//	 2---+--3'  |
	//	 |   |  |   |
	//	 |  .5--+---4
	//	 |.'    | .'
	//	 6------7'
	Cuboid(std::vector<vec3> Vertices)
    {
		if (Vertices.size() != CUBOID_V)
        {
			return;
		}

        float MinX = std::numeric_limits<float>::infinity();
        float MinY = std::numeric_limits<float>::infinity();
        float MinZ = std::numeric_limits<float>::infinity();
        float MaxX = - std::numeric_limits<float>::infinity();
        float MaxY = - std::numeric_limits<float>::infinity();
        float MaxZ = - std::numeric_limits<float>::infinity();
        for (auto Vertex : Vertices)
        {
            MinX = std::min(MinX, Vertex.x);
            MinY = std::min(MinY, Vertex.y);
            MinZ = std::min(MinZ, Vertex.z);
            MaxX = std::max(MaxX, Vertex.x);
            MaxY = std::max(MaxY, Vertex.y);
            MaxZ = std::max(MaxZ, Vertex.z);
        }
        AABBMin = vec3{ MinX, MinY, MinZ };
        AABBMax = vec3{ MaxX, MaxY, MaxZ };

		for (int i = 0; i < CUBOID_F; i++)
        {
            vec3 Point = Vertices[FaceCuboidMap[i][0]];
            vec3 Normal = glm::normalize(glm::cross(
                Vertices[FaceCuboidMap[i][1]] - Vertices[FaceCuboidMap[i][0]],
                Vertices[FaceCuboidMap[i][2]] - Vertices[FaceCuboidMap[i][0]]));
			Faces.emplace_back(Face(Point, Normal));
		}
	}

	Cuboid(vec3 Min, vec3 Max, std::vector<Face> Faces)
	{
        this->AABBMin = Min;
        this->AABBMax = Max;
        for (auto F : Faces)
        {
            this->Faces.emplace_back(F);
        }
	}
	//Cuboid(const Cuboid& C)
    //{
	//	for (int i = 0; i < CUBOID_V; i++)
    //    {
	//		Vertices[i] = vec3(C.Vertices[i]);
	//	}
	//	for (int i = 0; i < CUBOID_F; i++)
    //    {
	//		Faces[i] = Face(C.Faces[i]);
	//	}
	//}
};

struct Sphere
{
    vec3 Center;
    float Radius;
    Sphere() {}
    Sphere(vec3 Loc, float R)
    {
        Center = Loc;
        Radius = R;
    }
    Sphere(const Sphere& S)
    {
        Center = S.Center;
        Radius = S.Radius;
    }
};

// Bundle representing lines of sight between a player's possible peeks
// and an enemy's bounds. Bounds are stored in a field of
// the CullingController to prevent data duplication.
struct Bundle
{
	unsigned char PlayerI;
	unsigned char EnemyI;
    std::vector<vec3> PossiblePeeks;
	Bundle(int i, int j, const std::vector<vec3>& Peeks)
    {
		PlayerI = i;
		EnemyI = j;
        PossiblePeeks = Peeks;
	}
};

// Data that defines the character in space
struct CharacterBounds
{
    // Player's team.
    int Team;
    // Location of character's eyes.
    vec3 Eye;
    // Center of the lowest part of the character
    vec3 Base;
    // Angle the character is facing
    float Yaw;
    float Pitch;
    float Speed;
    // Divide vertices into top and bottom to skip the bottom half when
    // a player peeks it from above, and vice versa for peeks from below.
    // This computational shortcut may result in over-aggressive culling
    // in very rare situations.
    std::vector<vec3> TopVertices;
    std::vector<vec3> BottomVertices;
    // We also precalculate and store representations optimized for SIMD.
    CharacterBounds() : CharacterBounds(0, vec3(), vec3(), 0, 0, 0.0) {}
    CharacterBounds(
        int team, vec3 eyes, vec3 base, float yaw, float pitch, float speed)
    {
        Team = team;
        Eye = eyes;
        Base = base;
        Yaw = yaw;
        Pitch = pitch;
        Speed = speed;
        const vec3 z = vec3(0, 0, 1);
        const vec3 y = vec3(0, 1, 0);
        float yawR = yaw * PI / 180;
        float pitchR = pitch * PI / 180;
        // Gun barrel. TODO: rotate by pitch.
        vec3 BarrelExtent = glm::rotate(
            glm::rotate(vec3(40, 0, 0), pitchR, y),
            yawR,
            z);
        TopVertices.emplace_back(eyes + BarrelExtent);
        // Body bounding heptahedron.
        TopVertices.emplace_back(eyes + glm::rotate(vec3( 16,   0, 12), yawR, z));
        TopVertices.emplace_back(eyes + glm::rotate(vec3(-10, -15, 5), yawR, z));
        TopVertices.emplace_back(eyes + glm::rotate(vec3(-10,  15, 5), yawR, z));
        // Radius of the base of the player. Wider when legs are moving.
        float r = (speed > 0.1f) ? 24.0f : 16.0f;
        BottomVertices.emplace_back(base + glm::rotate(vec3( r,  r, 0), yawR, z));
        BottomVertices.emplace_back(base + glm::rotate(vec3(-r,  r, 0), yawR, z));
        BottomVertices.emplace_back(base + glm::rotate(vec3(-r, -r, 0), yawR, z));
        BottomVertices.emplace_back(base + glm::rotate(vec3( r, -r, 0), yawR, z));

    }
};

// Checks if a Cuboid intersects a line segment between Start and
// Start + Direction * MaxTime.
// If there is an intersection, returns the time of the point of intersection,
// measured as a the fractional distance along the line segment.
// Otherwise, returns NaN.
// Implements Cyrus-Beck line clipping algorithm from:
// http://geomalgorithms.com/a13-_intersect-4.html
inline float IntersectionTime(
    const Cuboid* C,
    const vec3& Start,
    const vec3& Direction,
    const float MaxTime = 1)
{
    float TimeEnter = 0;
    float TimeExit = MaxTime;
    for (int i = 0; i < CUBOID_F; i++)
    {
        // Numerator of a plane/line intersection test.
        const vec3& Normal = C->Faces[i].Normal;
        float Num = glm::dot(Normal, (C->Faces[i].Point - Start));
        float Denom = glm::dot(Normal, Direction);
        if (Denom == 0)
        {
            // Start is outside of the plane,
            // so it cannot intersect the Cuboid.
            if (Num < 0)
            {
                return std::numeric_limits<float>::quiet_NaN();
            }
        }
        else
        {
            float t = Num / Denom;
            // The segment is entering the face.
            if (Denom < 0)
            {
                TimeEnter = std::max(TimeEnter, t);
            }
            else
            {
                TimeExit = std::min(TimeExit, t);
            }
            // The segment exits before entering,
            // so it cannot intersect the cuboid.
            if (TimeEnter > TimeExit)
            {
                return std::numeric_limits<float>::quiet_NaN();
            }
        }
    }
    return TimeEnter;
}

// Checks if a Cuboid intersects all line segments between Starts[i]
// and Ends[i]
// Implements Cyrus-Beck line clipping algorithm from:
// http://geomalgorithms.com/a13-_intersect-4.html
// Uses SIMD for 8x throughput.
inline bool IntersectsAll(
    const Cuboid* C,
    __m128 StartXs,
    __m128 StartYs,
    __m128 StartZs,
    __m128 EndXs,
    __m128 EndYs,
    __m128 EndZs)
{
    const __m128 Zero = _mm_set1_ps(0);
    __m128 EnterTimes = Zero;
    __m128 ExitTimes = _mm_set1_ps(1);
    for (int i = 0; i < CUBOID_F; i++)
    {
        const vec3& Normal = C->Faces[i].Normal;
        __m128 NormalXs = _mm_set1_ps(Normal.x);
        __m128 NormalYs = _mm_set1_ps(Normal.y);
        __m128 NormalZs = _mm_set1_ps(Normal.z);
        const vec3& Vertex = C->Faces[i].Point;
#ifdef __FMA__  
        __m128 Nums =
            _mm_fmadd_ps(
                _mm_sub_ps(_mm_set1_ps(Vertex.x), StartXs),
                NormalXs,
                _mm_fmadd_ps(
                    _mm_sub_ps(_mm_set1_ps(Vertex.y), StartYs),
                    NormalYs,
                    _mm_mul_ps(
                        _mm_sub_ps(_mm_set1_ps(Vertex.z), StartZs),
                        NormalZs)));
        __m128 Denoms =
            _mm_fmadd_ps(
                _mm_sub_ps(EndXs, StartXs),
                NormalXs,
                _mm_fmadd_ps(
                    _mm_sub_ps(EndYs, StartYs),
                    NormalYs,
                    _mm_mul_ps(_mm_sub_ps(EndZs, StartZs), NormalZs)));
#else 
        __m128 Nums =
            _mm_add_ps(
                _mm_mul_ps(
                    _mm_sub_ps(_mm_set1_ps(Vertex.x), StartXs),
                    NormalXs
                ),
                _mm_add_ps(
                    _mm_mul_ps(
                        _mm_sub_ps(_mm_set1_ps(Vertex.y), StartYs),
                        NormalYs
                    ),
                    _mm_mul_ps(
                        _mm_sub_ps(_mm_set1_ps(Vertex.z), StartZs),
                        NormalZs)));
        __m128 Denoms =
            _mm_add_ps(
                _mm_mul_ps(
                    _mm_sub_ps(EndXs, StartXs),
                    NormalXs
                ),
                _mm_add_ps(
                    _mm_mul_ps(
                        _mm_sub_ps(EndYs, StartYs),
                        NormalYs
                    ),
                    _mm_mul_ps(_mm_sub_ps(EndZs, StartZs), NormalZs)));
#endif
        // A line segment is parallel to and outside of a face.
        if (0 !=
            _mm_movemask_ps(
                _mm_and_ps(
                    _mm_cmp_ps(Denoms, Zero, _CMP_EQ_OQ),
                    _mm_cmp_ps(Nums, Zero, _CMP_LE_OQ))))
        {
            return false;
        }
        __m128 Times = _mm_div_ps(Nums, Denoms);
        __m128 PositiveMask = _mm_cmp_ps(Denoms, Zero, _CMP_GT_OS);
        __m128 NegativeMask = _mm_cmp_ps(Denoms, Zero, _CMP_LT_OS);
        EnterTimes = _mm_blendv_ps(
            EnterTimes,
            _mm_max_ps(EnterTimes, Times),
            NegativeMask);
        ExitTimes = _mm_blendv_ps(
            ExitTimes,
            _mm_min_ps(ExitTimes, Times),
            PositiveMask);
        if (0 !=
            _mm_movemask_ps(_mm_cmp_ps(EnterTimes, ExitTimes, _CMP_GT_OS)))
        {
            return false;
        }
    }
    return true;
}

// Checks if the Cuboid blocks visibility between a player and enemy,
// returning true if and only if all lines of sights from the player's possible
// peeks are blocked.
// Assumes that the BottomVerticies of the enemy bounding box are directly below
// the TopVerticies.
inline bool IsBlocking(
    const std::vector<vec3>& Peeks,
    const CharacterBounds& Bounds,
    const Cuboid* C)
{
    auto TopVerticesXs = _mm_set_ps(
        Bounds.TopVertices[0].x, 
        Bounds.TopVertices[1].x,
        Bounds.TopVertices[2].x,
        Bounds.TopVertices[3].x);
    auto TopVerticesYs = _mm_set_ps(
        Bounds.TopVertices[0].y, 
        Bounds.TopVertices[1].y,
        Bounds.TopVertices[2].y,
        Bounds.TopVertices[3].y);
    auto TopVerticesZs = _mm_set_ps(
        Bounds.TopVertices[0].z, 
        Bounds.TopVertices[1].z,
        Bounds.TopVertices[2].z,
        Bounds.TopVertices[3].z);
    auto BottomVerticesXs = _mm_set_ps(
        Bounds.BottomVertices[0].x, 
        Bounds.BottomVertices[1].x,
        Bounds.BottomVertices[2].x,
        Bounds.BottomVertices[3].x);
    auto BottomVerticesYs = _mm_set_ps(
        Bounds.BottomVertices[0].y, 
        Bounds.BottomVertices[1].y,
        Bounds.BottomVertices[2].y,
        Bounds.BottomVertices[3].y);
    auto BottomVerticesZs = _mm_set_ps(
        Bounds.BottomVertices[0].z, 
        Bounds.BottomVertices[1].z,
        Bounds.BottomVertices[2].z,
        Bounds.BottomVertices[3].z);

    auto StartXs = _mm_set_ps1(Peeks[0].x);
    auto StartYs = _mm_set_ps1(Peeks[0].y);
    auto StartZs = _mm_set_ps1(Peeks[0].z);
    if (!IntersectsAll(
        C,
        StartXs, StartYs, StartZs,
       TopVerticesXs, TopVerticesYs, TopVerticesZs))
    {
        return false;
    }

    StartXs = _mm_set_ps1(Peeks[1].x);
    StartYs = _mm_set_ps1(Peeks[1].y);
    StartZs = _mm_set_ps1(Peeks[1].z);
    if (!IntersectsAll(
        C,
        StartXs, StartYs, StartZs,
       TopVerticesXs, TopVerticesYs, TopVerticesZs))
    {
        return false;
    }

    StartXs = _mm_set_ps1(Peeks[2].x);
    StartYs = _mm_set_ps1(Peeks[2].y);
    StartZs = _mm_set_ps1(Peeks[2].z);
    if (!IntersectsAll(
        C,
        StartXs, StartYs, StartZs,
       BottomVerticesXs, BottomVerticesYs, BottomVerticesZs))
    {
        return false;
    }

    StartXs = _mm_set_ps1(Peeks[3].x);
    StartYs = _mm_set_ps1(Peeks[3].y);
    StartZs = _mm_set_ps1(Peeks[3].z);
    if (!IntersectsAll(
        C,
        StartXs, StartYs, StartZs,
       BottomVerticesXs, BottomVerticesYs, BottomVerticesZs))
    {
        return false;
    }

    return true;
}

// Checks sphere intersection for all line segments between
// a player's possible peeks and the vertices of an enemy's bounding box.
// Uses sphere and line segment intersection with formula from:
// http://paulbourke.net/geometry/circlesphere/index.html#linesphere
inline bool IsBlocking(
    const std::vector<vec3>& Peeks,
    const CharacterBounds& Bounds,
    const Sphere& OccludingSphere)
{
    // Unpack constant variables outside of loop for performance.
    const vec3 SphereCenter = OccludingSphere.Center;
    const float RadiusSquared = OccludingSphere.Radius * OccludingSphere.Radius;
    for (auto i = 0U; i < Peeks.size(); i++)
    {
        vec3 PlayerToSphere = SphereCenter - Peeks[i];
        std::vector<vec3> Vertices;
        if (i < 2)
        {
            Vertices = Bounds.TopVertices;
        }
        else
        {
            Vertices = Bounds.BottomVertices;
        }
        for (vec3 V : Vertices)
        {
            const vec3 PlayerToEnemy = V - Peeks[i];
            const float u = (
                glm::dot(PlayerToEnemy, PlayerToSphere)
                / glm::dot(PlayerToEnemy, PlayerToEnemy));
            // The point on the line between player and enemy that is closest to
            // the center of the occluding sphere lies between player and enemy.
            // Thus the sphere might intersect the line segment.
            if ((0 < u) && (u < 1))
            {
                const vec3 ClosestPoint = Peeks[i] + u * PlayerToEnemy;
                const vec3 ClosestToCenter = SphereCenter - ClosestPoint;
                // The point lies within the radius of the sphere,
                // so the sphere intersects the line segment.
                if (glm::dot(ClosestToCenter, ClosestToCenter)  > RadiusSquared)
                {
                    return false;
                }
            }
            // The sphere does not intersect the line segment.
            else
            {
                return false;
            }
        }
    }
    return true;
}

// Optimized line segment that stores:
//   Start: The start position of the line segment.
//   Delta: The displacement vector from Start to End.
//   Reciprocal: The element-wise reciprocal of the displacement vector.
struct OptSegment
{
    vec3 Start;
    vec3 Reciprocal;
    vec3 Delta;
    OptSegment() {}
    OptSegment(vec3 Start, vec3 End)
    {
        this->Start = Start;
        Delta = End - Start;
        Reciprocal = vec3(1/Delta.x, 1/Delta.y, 1/Delta.z);
    }
};
