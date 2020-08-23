#pragma once

#include <algorithm>
#include <vector>
#include <glm/glm.hpp>
#include <glm/vec3.hpp>
#include <glm/gtx/rotate_vector.hpp>
using glm::vec3;

constexpr float PI = 3.14159265358979323846;
// Number of vertices and faces of a cuboid.
constexpr char CUBOID_V = 8;
constexpr char CUBOID_F = 6;
// Number of vertices in a face of a cuboid.
constexpr char CUBOID_FACE_V = 4;

// Maps a Face with index i's j-th vertex onto a Cuboid vertex index.
constexpr char FaceCuboidMap[6][4] =
{
    0, 1, 2, 3,
    2, 6, 7, 3,
    0, 3, 7, 4,
    0, 4, 5, 1,
    1, 5, 6, 2,
    4, 7, 6, 5
};

// Quadrilateral face of a cuboid.
struct Face
{
	vec3 Normal;
    // Index of the face in its Cuboid;
	//	   .+---------+  
	//	 .' |  0    .'|  
	//	+---+-----+'  |  
	//	|   |    3|   |  
	//	| 4 |     | 2 |  
	//	|   |1    |   |  
	//	|  ,+-----+---+  
	//	|.'    5  | .'   
	//	+---------+'    
	//	1 is in front.
	char Index;
	Face() {}
	Face(int i, vec3 Vertices[])
    {
		Normal = glm::normalize(glm::cross(
			Vertices[FaceCuboidMap[i][1]] - Vertices[FaceCuboidMap[i][0]],
			Vertices[FaceCuboidMap[i][2]] - Vertices[FaceCuboidMap[i][0]]));
	}
	Face(const Face& F)
    {
        Index = F.Index;
		Normal = vec3(F.Normal);
	}
};

// A six-sided polyhedron defined by 8 vertices.
// A valid configuration of vertices is user-enforced.
// For example, all vertices of a face should be coplanar.
struct Cuboid
{
	Face Faces[CUBOID_F];
	vec3 Vertices[CUBOID_V];
	Cuboid () {}
	// Constructs a cuboid from a list of vertices.
	// Vertices are ordered and indexed as such:
	//	    .1------0
	//	  .' |    .'|
	//	 2---+--3'  |
	//	 |   |  |   |
	//	 |  .5--+---4
	//	 |.'    | .'
	//	 6------7'
	Cuboid(std::vector<vec3> V)
    {
		if (V.size() != CUBOID_V)
        {
			return;
		}
		for (int i = 0; i < CUBOID_V; i++)
        {
			Vertices[i] = vec3(V[i]);
		}
		for (int i = 0; i < CUBOID_F; i++)
        {
			Faces[i] = Face(i, Vertices);
		}
	}
	Cuboid(const Cuboid& C)
    {
		for (int i = 0; i < CUBOID_V; i++)
        {
			Vertices[i] = vec3(C.Vertices[i]);
		}
		for (int i = 0; i < CUBOID_F; i++)
        {
			Faces[i] = Face(C.Faces[i]);
		}
	}
	// Return the vertex on face i with perimeter index j.
	const vec3& GetVertex(int i, int j) const
    {
		return Vertices[FaceCuboidMap[i][j]];
	}
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
    // Location of character's eyes.
    vec3 Eye;
    // Center of the lowest part of the character
    vec3 Base;
    // Angle the character is facing
    float Yaw;
    float Pitch;
    int Team;
    // Divide vertices into top and bottom to skip the bottom half when
    // a player peeks it from above, and vice versa for peeks from below.
    // This computational shortcut may result in over-aggressive culling
    // in very rare situations.
    std::vector<vec3> TopVertices;
    std::vector<vec3> BottomVertices;
    // We also precalculate and store representations optimized for SIMD.
    __m256 TopVerticesXs;
    __m256 TopVerticesYs;
    __m256 TopVerticesZs;
    __m256 BottomVerticesXs;
    __m256 BottomVerticesYs;
    __m256 BottomVerticesZs;
    CharacterBounds() : CharacterBounds(0, vec3(), vec3(), 0, 0) {}
    CharacterBounds(int team, vec3 eyes, vec3 base, float yaw, float pitch)
    {
        Eye = eyes;
        Base = base;
        Yaw = yaw;
        Pitch = pitch;
        Team = team;
        const vec3 z = vec3(0, 0, 1);
        const vec3 y = vec3(0, 1, 0);
        float yawR = yaw * PI / 180;
        float pitchR = pitch * PI / 180;
        // Gun barrel. TODO: rotate by pitch.
        vec3 BarrelExtent = glm::rotate(
            glm::rotate(vec3(100, 0, 0), pitchR, y),
            yawR,
            z);
        TopVertices.emplace_back(eyes + BarrelExtent);
        // Body bounding heptahedron.
        TopVertices.emplace_back(eyes + glm::rotate(vec3( 16,   0, 12), yawR, z));
        TopVertices.emplace_back(eyes + glm::rotate(vec3(-10, -15, 5), yawR, z));
        TopVertices.emplace_back(eyes + glm::rotate(vec3(-10,  15, 5), yawR, z));
        BottomVertices.emplace_back(base + glm::rotate(vec3( 25,  25, 0), yawR, z));
        BottomVertices.emplace_back(base + glm::rotate(vec3(-25,  25, 0), yawR, z));
        BottomVertices.emplace_back(base + glm::rotate(vec3(-25, -25, 0), yawR, z));
        BottomVertices.emplace_back(base + glm::rotate(vec3( 25, -25, 0), yawR, z));

        TopVerticesXs = _mm256_set_ps(
            TopVertices[0].x, TopVertices[1].x, TopVertices[2].x, TopVertices[3].x, 
            TopVertices[0].x, TopVertices[1].x, TopVertices[2].x, TopVertices[3].x);
        TopVerticesYs = _mm256_set_ps(
            TopVertices[0].y, TopVertices[1].y, TopVertices[2].y, TopVertices[3].y, 
            TopVertices[0].y, TopVertices[1].y, TopVertices[2].y, TopVertices[3].y);
        TopVerticesZs = _mm256_set_ps(
            TopVertices[0].z, TopVertices[1].z, TopVertices[2].z, TopVertices[3].z, 
            TopVertices[0].z, TopVertices[1].z, TopVertices[2].z, TopVertices[3].z);
        BottomVerticesXs = _mm256_set_ps(
            BottomVertices[0].x, BottomVertices[1].x, BottomVertices[2].x, BottomVertices[3].x, 
            BottomVertices[0].x, BottomVertices[1].x, BottomVertices[2].x, BottomVertices[3].x);
        BottomVerticesYs = _mm256_set_ps(
            BottomVertices[0].y, BottomVertices[1].y, BottomVertices[2].y, BottomVertices[3].y, 
            BottomVertices[0].y, BottomVertices[1].y, BottomVertices[2].y, BottomVertices[3].y);
        BottomVerticesZs = _mm256_set_ps(
            BottomVertices[0].z, BottomVertices[1].z, BottomVertices[2].z, BottomVertices[3].z, 
            BottomVertices[0].z, BottomVertices[1].z, BottomVertices[2].z, BottomVertices[3].z);
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
        float Num = glm::dot(Normal, (C->GetVertex(i, 0) - Start));
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
    __m256 StartXs,
    __m256 StartYs,
    __m256 StartZs,
    __m256 EndXs,
    __m256 EndYs,
    __m256 EndZs)
{
    const __m256 Zero = _mm256_set1_ps(0);
    __m256 EnterTimes = Zero;
    __m256 ExitTimes = _mm256_set1_ps(1);
    for (int i = 0; i < CUBOID_F; i++)
    {
        const vec3& Normal = C->Faces[i].Normal;
        __m256 NormalXs = _mm256_set1_ps(Normal.x);
        __m256 NormalYs = _mm256_set1_ps(Normal.y);
        __m256 NormalZs = _mm256_set1_ps(Normal.z);
        const vec3& Vertex = C->GetVertex(i, 0);
        __m256 Nums =
            _mm256_fmadd_ps(
                _mm256_sub_ps(_mm256_set1_ps(Vertex.x), StartXs),
                NormalXs,
                _mm256_fmadd_ps(
                    _mm256_sub_ps(_mm256_set1_ps(Vertex.y), StartYs),
                    NormalYs,
                    _mm256_mul_ps(
                        _mm256_sub_ps(_mm256_set1_ps(Vertex.z), StartZs),
                        NormalZs)));
        __m256 Denoms =
            _mm256_fmadd_ps(
                _mm256_sub_ps(EndXs, StartXs),
                NormalXs,
                _mm256_fmadd_ps(
                    _mm256_sub_ps(EndYs, StartYs),
                    NormalYs,
                    _mm256_mul_ps(_mm256_sub_ps(EndZs, StartZs), NormalZs)));
        // A line segment is parallel to and outside of a face.
        if (0 !=
            _mm256_movemask_ps(
                _mm256_and_ps(
                    _mm256_cmp_ps(Denoms, Zero, _CMP_EQ_OQ),
                    _mm256_cmp_ps(Nums, Zero, _CMP_LE_OQ))))
        {
            return false;
        }
        __m256 Times = _mm256_div_ps(Nums, Denoms);
        __m256 PositiveMask = _mm256_cmp_ps(Denoms, Zero, _CMP_GT_OS);
        __m256 NegativeMask = _mm256_cmp_ps(Denoms, Zero, _CMP_LT_OS);
        EnterTimes = _mm256_blendv_ps(
            EnterTimes,
            _mm256_max_ps(EnterTimes, Times),
            NegativeMask);
        ExitTimes = _mm256_blendv_ps(
            ExitTimes,
            _mm256_min_ps(ExitTimes, Times),
            PositiveMask);
        if (0 !=
            _mm256_movemask_ps(_mm256_cmp_ps(EnterTimes, ExitTimes, _CMP_GT_OS)))
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
    __m256 StartXs = _mm256_set_ps(
        Peeks[0].x, Peeks[0].x, Peeks[0].x, Peeks[0].x,
        Peeks[1].x, Peeks[1].x, Peeks[1].x, Peeks[1].x);
    __m256 StartYs = _mm256_set_ps(
        Peeks[0].y, Peeks[0].y, Peeks[0].y, Peeks[0].y,
        Peeks[1].y, Peeks[1].y, Peeks[1].y, Peeks[1].y);
    __m256 StartZs = _mm256_set_ps(
        Peeks[0].z, Peeks[0].z, Peeks[0].z, Peeks[0].z,
        Peeks[1].z, Peeks[1].z, Peeks[1].z, Peeks[1].z);
    if (
        !IntersectsAll(
            C,
            StartXs, StartYs, StartZs,
            Bounds.TopVerticesXs, Bounds.TopVerticesYs, Bounds.TopVerticesZs))
    {
        return false;
    }
    else
    {
        StartXs = _mm256_set_ps(
            Peeks[2].x, Peeks[2].x, Peeks[2].x, Peeks[2].x,
            Peeks[3].x, Peeks[3].x, Peeks[3].x, Peeks[3].x);
        StartYs = _mm256_set_ps(
            Peeks[2].y, Peeks[2].y, Peeks[2].y, Peeks[2].y,
            Peeks[3].y, Peeks[3].y, Peeks[3].y, Peeks[3].y);
        StartZs = _mm256_set_ps(
            Peeks[2].z, Peeks[2].z, Peeks[2].z, Peeks[2].z,
            Peeks[3].z, Peeks[3].z, Peeks[3].z, Peeks[3].z);
        return IntersectsAll(
            C,
            StartXs, StartYs, StartZs,
            Bounds.BottomVerticesXs, Bounds.BottomVerticesYs, Bounds.BottomVerticesZs);
    }
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
    for (int i = 0; i < Peeks.size(); i++)
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
