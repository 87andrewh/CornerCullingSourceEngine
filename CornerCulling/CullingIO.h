#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <glm/vec3.hpp>
#include <cstring>
using glm::vec3;

constexpr float PI = 3.141592653;

// Returns a cuboid's vertices from a text representation of a Cuboid.
// Assumes that input is a filestrem already pointing
// to the first line of a cuboid representation:
// CenterX      CenterY     CenterZ
// ScaleX       ScaleY      ScaleZ
// RotationX    RotationY   RotationZ
// Extent1X     Extent1Y    Extent1Z
// ...          ...         ...
// Extent8X     Extent8Y    Extent8Z
inline std::vector<vec3> TextToCuboidVertices(std::ifstream& input)
{
    float center[3];
    float scales[3];
    float rotations[3];
    float extents[8][3] = { 0 };
    std::string line;
    std::getline(input, line);
    std::istringstream centerStream(line);
    centerStream >> center[0] >> center[1] >> center[2];
    std::getline(input, line);
    std::istringstream scaleStream(line);
    scaleStream >> scales[0] >> scales[1] >> scales[2];
    std::getline(input, line);
    std::istringstream rotationStream(line);
    rotationStream >> rotations[0] >> rotations[1] >> rotations[2];
    for (int i = 0; i < 8; i++)
    {
        std::getline(input, line);
        std::istringstream extentStream(line);
        extentStream >> extents[i][0] >> extents[i][1] >> extents[i][2];
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

// Returns a cuboid's vertices from a text representation of an AABB.
// Assumes that input is a filestrem already pointing
// to the first line of a cuboid representation:
// Vertex1X     Vertex1Y    Vertex1Z
// Vertex2X     Vertex2Y    Vertex2Z
inline std::vector<vec3> TextToAABBVertices(std::ifstream& input)
{
    float v1[3];
    float v2[3];
    float min[3];
    float max[3];
    std::string line;
    std::getline(input, line);
    std::istringstream v1Stream(line);
    v1Stream >> v1[0] >> v1[1] >> v1[2];
    std::getline(input, line);
    std::istringstream v2Stream(line);
    v2Stream >> v2[0] >> v2[1] >> v2[2];
    for (int i = 0; i < 3; i++) {
        min[i] = std::min(v1[i], v2[i]);
        max[i] = std::max(v1[i], v2[i]);
    }
    std::vector<vec3> out =
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
    return out;
}

// Returns a list of cuboid vertices from a text representation in a file
inline std::vector<std::vector<vec3>> FileToCuboidVertices(char* mapName)
{
    std::vector<std::vector<vec3>> cuboidVertices;
    char fileName[64];
    strncpy(fileName, "culling_", 10);
    strncat(fileName, mapName, 40);
    strncat(fileName, ".txt", 10);
    std::ifstream in;
    in.open(fileName);
    if (!in)
    {
        printf(fileName);
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
        cuboidVertices.push_back(empty);
        return cuboidVertices;
    }
    std::string line;
    while (std::getline(in, line))
    {
        std::string token;
        std::istringstream stream(line);
        stream >> token;
        if (token == "Cuboid")
        {
            cuboidVertices.push_back(TextToCuboidVertices(in));
        }
        else if (token == "AABB")
        {
            cuboidVertices.push_back(TextToAABBVertices(in));
        }
    }
    in.close();
    return cuboidVertices;
}
