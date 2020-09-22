#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <glm/vec3.hpp>
#include <cstring>
using glm::vec3;

// Returns a Cuboid's vertices from a vertex representation.
// Assumes that input is a filestrem pointing
// to the first line of a cuboid representation:
// CenterX      CenterY     CenterZ
// ScaleX       ScaleY      ScaleZ
// RotationX    RotationY   RotationZ
// Extent1X     Extent1Y    Extent1Z
// ...          ...         ...
// Extent8X     Extent8Y    Extent8Z
inline std::vector<vec3> CuboidVerticesFromVertices(std::ifstream& input)
{
    float center[3];
    float scales[3];
    float rotations[3];
    float extents[8][3] = {{0}};
    std::string line;
    std::getline(input, line);
    std::istringstream{ line } >> center[0] >> center[1] >> center[2];
    std::getline(input, line);
    std::istringstream{ line } >> scales[0] >> scales[1] >> scales[2];
    std::getline(input, line);
    std::istringstream{ line } >> rotations[0] >> rotations[1] >> rotations[2];
    for (int i = 0; i < 8; i++)
    {
        std::getline(input, line);
        std::istringstream{ line } >> extents[i][0] >> extents[i][1] >> extents[i][2];
    }

    // Convert to radians
    rotations[0] *= PI / 180;
    rotations[1] *= PI / 180;
    rotations[2] *= PI / 180;
    // Scale extents
    for (int i = 0; i < 8; i++)
    {
        extents[i][0] *= scales[0];
        extents[i][1] *= scales[1];
        extents[i][2] *= scales[2];
    }
    // Rotate around X axis
    for (int i = 0; i < 8; i++)
    {
        float a = rotations[0];
        float tmp = cos(a) * extents[i][1] - sin(a) * extents[i][2];
        extents[i][2] = sin(a) * extents[i][1] + cos(a) * extents[i][2];
        extents[i][1] = tmp;
    }
    // Rotate around Y axis
    for (int i = 0; i < 8; i++)
    {
        float a = rotations[1];
        float tmp = cos(a) * extents[i][0] + sin(a) * extents[i][2];
        extents[i][2] = -sin(a) * extents[i][0] + cos(a) * extents[i][2];
        extents[i][0] = tmp;
    }
    // Rotate around Z axis
    for (int i = 0; i < 8; i++)
    {
        float a = rotations[2];
        float tmp = cos(a) * extents[i][0] - sin(a) * extents[i][1];
        extents[i][1] = sin(a) * extents[i][0] + cos(a) * extents[i][1];
        extents[i][0] = tmp;
    }
    std::vector<vec3> vertices;
    for (int i = 0; i < 8; i++)
    {
        vertices.push_back(vec3(
            center[0] + extents[i][0],
            center[1] + extents[i][1],
            center[2] + extents[i][2]));
    }
    return vertices;
}

// Returns a cuboid's vertices from an AABB representation.
// Assumes that input is a filestrem already pointing
// to the first line of a cuboid representation:
// Vertex1X     Vertex1Y    Vertex1Z
// Vertex2X     Vertex2Y    Vertex2Z
inline std::vector<vec3> CuboidVerticesFromAABB(std::ifstream& input)
{
    float v1[3];
    float v2[3];
    float min[3];
    float max[3];
    std::string line;
    std::getline(input, line);
    std::istringstream{ line }  >> v1[0] >> v1[1] >> v1[2];
    std::getline(input, line);
    std::istringstream{ line } >> v2[0] >> v2[1] >> v2[2];
    for (int i = 0; i < 3; i++) {
        min[i] = std::min(v1[i], v2[i]);
        max[i] = std::max(v1[i], v2[i]);
    }
    std::vector<vec3> vertices =
    {
        vec3(max[0], max[1], max[2]),
        vec3(min[0], max[1], max[2]),
        vec3(min[0], min[1], max[2]),
        vec3(max[0], min[1], max[2]),
        vec3(max[0], max[1], min[2]),
        vec3(min[0], max[1], min[2]),
        vec3(min[0], min[1], min[2]),
        vec3(max[0], min[1], min[2]),
    };
    return vertices;
}

// Returns a Cuboid from a face representation.
// Assumes that input is a filestrem already pointing
// to the first line of a cuboid representation:
inline Cuboid CuboidFromFaces(std::ifstream& input)
{
    std::string line;

    vec3 min;
    std::getline(input, line);
    std::istringstream{ line } >> min.x >> min.y >> min.z;

    vec3 max;
    std::getline(input, line);
    std::istringstream{ line } >> max.x >> max.y >> max.z;

    std::vector<Face> faces;
    for (int i = 0; i < 6; i++)
    {
        vec3 point;
        std::getline(input, line);
        std::istringstream{ line } >> point.x >> point.y >> point.z;

        vec3 normal;
        std::getline(input, line);
        std::istringstream{ line } >> normal.x >> normal.y >> normal.z;

        faces.emplace_back(Face(point, normal));
    }

    return Cuboid(min, max, faces);
}

// Returns a list of cuboid vertices from a text representation in a file
inline std::vector<Cuboid> FileToCuboids(char* mapName)
{
    std::vector<Cuboid> cuboids;

    char fileName[128];
    strncpy(fileName, "csgo/maps/culling_", 20);
    strncat(fileName, mapName, 60);
    strncat(fileName, ".txt", 10);

    std::ifstream in;
    in.open(fileName);

    if (!in)
    {
        printf("%s\n", fileName);
        printf(" not found\n");
        std::vector<vec3> empty =
        {
            vec3(1, 1, 1),
            vec3(0, 1, 1),
            vec3(0, 1, 1),
            vec3(1, 0, 1),
            vec3(1, 1, 0),
            vec3(0, 1, 0),
            vec3(0, 1, 0),
            vec3(1, 0, 0),
        };
        cuboids.push_back(Cuboid(empty));
        return cuboids;
    }

    std::string line;
    while (std::getline(in, line))
    {
        std::string token;
        std::istringstream{ line } >> token;
        if (token == "Cuboid")
        {
            cuboids.push_back(Cuboid(CuboidVerticesFromVertices(in)));
        }
        else if (token == "AABB")
        {
            cuboids.push_back(Cuboid(CuboidVerticesFromAABB(in)));
        }
        else if (token == "CuboidFaces")
        {
            cuboids.push_back(CuboidFromFaces(in));
        }
    }
    in.close();
    return cuboids;
}

// Returns a list of cuboid vertices from a text representation in a file
inline std::vector<vec3> GetFirstCuboidVertices(char* mapName)
{
    auto empty = std::vector<vec3> {
        vec3(1, 1, 1),
        vec3(0, 1, 1),
        vec3(0, 1, 1),
        vec3(1, 0, 1),
        vec3(1, 1, 0),
        vec3(0, 1, 0),
        vec3(0, 1, 0),
        vec3(1, 0, 0),
    };

    char fileName[128];
    strncpy(fileName, "csgo/maps/culling_", 20);
    strncat(fileName, mapName, 60);
    strncat(fileName, ".txt", 10);

    std::ifstream in;
    in.open(fileName);

    if (!in)
    {
        printf("%s\n", fileName);
        printf(" not found\n");
        return empty;
    }

    std::string line;
    while (std::getline(in, line))
    {
        std::string token;
        std::istringstream{ line } >> token;
        if (token == "Cuboid")
        {
            return CuboidVerticesFromVertices(in);
        }
        if (token == "AABB")
        {
            return CuboidVerticesFromAABB(in);
        }
    }

    return empty;
}
