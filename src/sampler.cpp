#include <tracer/sampler.h>

#include <functional>

#include "util.h"

namespace tracer
{

glm::vec3 transformLocalSampleToWorld(const glm::vec3& normal, const glm::vec3& sample)
{
    using namespace glm;

    vec3 axis1, axis2;
    createCoordSystemWithUpVec(normal, axis1, axis2);
    mat3 localToWorld(axis1, normal, axis2);

    return normalize(localToWorld * sample);
}

glm::vec3 transformWorldSampleToLocal(const glm::vec3& normal, const glm::vec3& sample)
{
    using namespace glm;

    vec3 axis1, axis2;
    createCoordSystemWithUpVec(normal, axis1, axis2);
    mat3 localToWorld(axis1, normal, axis2);

    return normalize(inverse(localToWorld) * sample);
}

}