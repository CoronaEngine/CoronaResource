//
// Created by 25473 on 25-10-1.
//

#pragma once

#include "ktm/ktm.h"

struct transform {
    ktm::fvec3 position;
    ktm::fquat rotation;
    ktm::fvec3 scale;
};