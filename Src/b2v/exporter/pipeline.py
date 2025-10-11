import bpy
import os
import json
import math
from bpy.props import (
    BoolProperty,
    CollectionProperty,
    EnumProperty,
    FloatProperty,
    IntProperty,
    PointerProperty,
    StringProperty,
)
from .. import utils

def export(exporter):
    ret = exporter.get_params("pipeline")
    res_x = exporter.context.scene.render.resolution_x
    res_y = exporter.context.scene.render.resolution_y
    
    ret["param"]["frame_buffer"] = exporter.get_params("frame_buffer")
    fb_data = ret["param"]["frame_buffer"]
    fb_data["param"]["tone_mapper"] = exporter.get_params("tone_mapper")
    fb_data["param"]["resolution"] = [res_x, res_y]
    return ret