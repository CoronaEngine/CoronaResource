//
// Created by 25473 on 25-9-14.
//

#ifndef SCENE_H
#define SCENE_H
#include <IResource.h>
#include "ktm/type_vec.h"

namespace Corona {

class Scene final : public IResource {
  public:

    struct Camera
    {
        float fov = 45.0f;
        ktm::fvec3 pos = ktm::fvec3(1.0f, 1.0f, 1.0f);
        ktm::fvec3 forward = ktm::fvec3(-1.0f, -1.0f, -1.0f);
        ktm::fvec3 worldUp = ktm::fvec3(0.0f, 1.0f, 0.0f);
    };

    Camera camera;
    void *displaySurface = nullptr;
    ktm::fvec3 sunDirection = ktm::fvec3(0.0f, 1.0f, 0.0f);
};

} // Corona

#endif //SCENE_H
