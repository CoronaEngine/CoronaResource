from . import base
from .base import *

dic = {
    "fixed": {
        "label": "fixed",
        "description": "fixed pipeline.",
        "parameters": {
            "obfuscation": {
                "type": "Bool",
                "args": {
                    "default": False,
                },
            },
            "valid_check": {
                "type": "Bool",
                "args": {
                    "default": False,
                },
            },
        },
    },
}

class VISION_RENDER_PT_Pipeline(bpy.types.Panel, VISION_RENDER_PT_VisionBasePanel):
    bl_idname = "VISION_RENDER_PT_Pipeline"
    bl_label = "Pipeline"
    attr_type = "pipeline"
    dic = dic