#pragma once
#include "FastBVH/BBox.h"
#include "FastBVH/BVH.h"
#include "FastBVH/BuildStrategy.h"
#include "FastBVH/BuildStrategy1.h"
#include "FastBVH/Config.h"
#include "FastBVH/Intersection.h"
#include "FastBVH/Iterable.h"
#include "FastBVH/Ray.h"
#include "FastBVH/Traverser.h"
#include "FastBVH/Vector3.h"
#include "GeometricPrimitives.h"

// Cuboid BVH API.
namespace
{
    using std::vector;
    using namespace FastBVH;

    // Used to calculate the axis-aligned bounding boxes of cuboids.
    class CuboidBoxConverter final
    {
        public:
            BBox<float> operator()(const Cuboid& C) const noexcept
            {
                return BBox<float>(
                    Vector3<float>{C.AABBMin.x, C.AABBMin.y, C.AABBMin.z},
                    Vector3<float>{C.AABBMax.x, C.AABBMax.y, C.AABBMax.z});
            }
    };
    
    // Used to calculate the intersection between rays and cuboids.
    class CuboidIntersector final 
    {
        public:
            Intersection<float> operator()(
                const Cuboid& C,
                const OptSegment& Segment) const noexcept
            {
                float Time = IntersectionTime(&C, Segment.Start, Segment.Delta);
                if (Time > 0)
                {
                    return Intersection<float> { Time, &C };
                }
                else
                {
                    return Intersection<float> {};
                }
            }
    };
}
