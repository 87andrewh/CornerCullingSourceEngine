#if defined _culling_included
  #endinput
#endif
#define _culling_included

// Tells the culling extension which map to load data from
native void SetCullingMap(
    const char[] name,
    int tickRate,
    int maxLookahead);
// Allows the extension to calculate and update pairwise
// visibility between clients.
native void UpdateVisibility(
    int[] clientTeams,
    float[] eyesFlat,
    float[] basesFlat,
    float[] yaws,
    float[] pitches,
    float[] speeds,
    bool[] visibilityFlat);
// Returns the information needed to render the 12 edges
// of an occluding cuboid.
// Each edge is represented as [v1.x, v1.y, v1.z, v2.x, v2.y, v2.z]
// Yes, I realize that this uses extra bits.
native void GetRenderedCuboid(char[] mapName, float[] edges);
