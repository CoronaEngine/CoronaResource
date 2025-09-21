from . import base
from .base import *

dic = {
    "normal": {
        "label": "normal",
        "description": "normal frame buffer.",
        "parameters": {
            "accumulation": {
                "type": "Bool",
                "args": {
                    "default": True,
                },
            },
            "exposure": {"type": "Float", "args": {"default": 1, "max": 10, "min": 0}},
        },
    },
    "lightfield": {
        "label": "lightfield",
        "description": "lightfield frame buffer.",
        "parameters": {
            "accumulation": {
                "type": "Bool",
                "args": {
                    "default": True,
                },
            },
            "exposure": {"type": "Float", "args": {"default": 1, "max": 10, "min": 0}},
        },
    },
}

class VISION_RENDER_PT_FrameBuffer(bpy.types.Panel, VISION_RENDER_PT_VisionBasePanel):
    bl_idname = "VISION_RENDER_PT_FrameBuffer"
    bl_label = "FrameBuffer"
    attr_type = "frame_buffer"
    bl_parent_id = "VISION_RENDER_PT_Pipeline"
    dic = dic