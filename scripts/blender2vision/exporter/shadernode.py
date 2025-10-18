import bpy
import os
import json
import shutil
from bpy.props import (
    BoolProperty,
    CollectionProperty,
    EnumProperty,
    FloatProperty,
    IntProperty,
    PointerProperty,
    StringProperty,
)


def slot_data(node_name, output_key, channels="x"):
    ret = {
        "channels": channels,
        "output_key": output_key,
        "node": node_name,
    }
    return ret

def try_add_tab(node_tab, key, val):
    if not (key in node_tab):
        node_tab[key] = val
        

def parse_input(exporter, input, dim, node_tab):
    if not input.is_linked:
        return None
    else:
        return parse_node(exporter, input, 2, node_tab)


def parse_image_node(exporter, link, dim, node_tab):
    from_node = link.from_node
    exporter.try_make_tex_dir()
    
    # 检查图像节点是否有有效的图像数据
    if not from_node.image:
        print(f"Warning: Image node has no image data")
        return None
    
    # 处理嵌入式纹理（packed images）
    if from_node.image.packed_file:
        # 如果是嵌入式纹理，先保存到磁盘
        image_name = from_node.image.name
        if not image_name.lower().endswith(('.png', '.jpg', '.jpeg', '.exr', '.hdr')):
            image_name += '.png'  # 添加默认扩展名
        dst_path = exporter.texture_path(image_name)
        
        try:
            # 保存嵌入的图像到文件
            from_node.image.save_render(dst_path)
            print(f"Info: Saved packed texture to: {dst_path}")
        except Exception as e:
            print(f"Warning: Failed to save packed texture: {e}")
            return None
    else:
        # 处理外部文件纹理
        src_path = bpy.path.abspath(from_node.image.filepath)
        dst_path = exporter.texture_path(from_node.image.name)
        
        # 检查源文件是否存在
        if not os.path.exists(src_path):
            print(f"Warning: Texture file not found: {src_path}")
            return None
        
        try:
            shutil.copyfile(src_path, dst_path)
        except (FileNotFoundError, PermissionError, OSError) as e:
            print(f"Warning: Failed to copy texture file from {src_path} to {dst_path}: {e}")
            return None
    
    r_path = exporter.tex_dir + "/" + from_node.image.name
    _, extension = os.path.splitext(r_path)
    cs = from_node.image.colorspace_settings.name
    cs = cs.lower()
    
    if "linear" in cs or "non-color" == cs:
        color_space = "linear"
    else:
        color_space = "srgb"
    
    fs = link.from_socket
    if fs.name == "Color":
        channels = "xyz" if dim == 3 else "x"
    elif fs.name == "Alpha":
        channels = "w" if dim == 1 else "www"

    node_name = str(from_node)
        
    vector = parse_input(exporter, from_node.inputs["Vector"], 3, node_tab)

    val = {
        "type": "image",
        "param": {
            "fn": r_path,
            "vector": vector,
            "color_space": color_space,
        },
    }

    ret = slot_data(node_name, fs.name, channels)
    try_add_tab(node_tab, node_name, val)
    return ret


def parse_tex_coord(exporter, link, dim, node_tab):
    from_node = link.from_node
    fs = link.from_socket
    output_key = fs.name
    node_name = str(from_node)
    ret = slot_data(node_name, output_key)
    val = {
        "type": "src_node",
        "construct_name": "tex_coord",
        "param": {},
    }
    try_add_tab(node_tab, node_name, val)
    return ret


def parse_new_geometry(exporter, link, dim, node_tab):
    from_node = link.from_node
    fs = link.from_socket
    output_key = fs.name
    node_name = str(from_node)
    ret = slot_data(node_name, output_key)
    val = {
        "type": "src_node",
        "construct_name": "geometry",
        "param": {},
    }
    try_add_tab(node_tab, node_name, val)
    return ret


def parse_camera_data(exporter, link, dim, node_tab):
    from_node = link.from_node
    fs = link.from_socket
    output_key = fs.name
    node_name = str(from_node)
    ret = slot_data(node_name, output_key)
    val = {
        "type": "src_node",
        "construct_name": "camera",
        "param": {},
    }
    try_add_tab(node_tab, node_name, val)
    return ret

def parse_vector_mapping(exporter, link, dim, node_tab):
    from_node = link.from_node
    fs = link.from_socket
    output_key = fs.name
    node_name = str(from_node)
    ret = slot_data(node_name, output_key)
    vector = parse_node(exporter, from_node.inputs["Vector"], 3, node_tab)
    scale = parse_node(exporter, from_node.inputs["Scale"], 3, node_tab)
    rotation = parse_node(exporter, from_node.inputs["Rotation"], 3, node_tab)
    location = None
    if "Location" in from_node.inputs:
        location = parse_node(exporter, from_node.inputs["Location"], 3, node_tab)
    
    val = {
        "type": "converter",
        "construct_name": "vector_mapping",
        "param": {
            "type" : from_node.vector_type,
            "vector" : vector,
            "scale" : scale,
            "rotation" : rotation,
            "location" : location,
        },
    }
    try_add_tab(node_tab, node_name, val)
    return ret
    

def parse_fresnel(exporter, link, dim, node_tab):
    from_node = link.from_node
    fs = link.from_socket
    output_key = fs.name
    node_name = str(from_node)
    ret = slot_data(node_name, output_key)
    
    normal = parse_node(exporter, from_node.inputs["Normal"], 3, node_tab)
    ior = parse_node(exporter, from_node.inputs["IOR"], 1, node_tab)
    
    val = {
        "type": "converter",
        "construct_name": "fresnel",
        "param": {
            "normal" : normal,
            "ior" : ior,    
        },
    }
    try_add_tab(node_tab, node_name, val)
    return ret


def parse_normal_map(exporter, link, dim, node_tab):
    from_node = link.from_node
    fs = link.from_socket
    output_key = fs.name
    node_name = str(from_node)
    ret = slot_data(node_name, output_key, "xyz")
    color = parse_node(exporter, from_node.inputs["Color"], 3, node_tab)
    strength = parse_node(exporter, from_node.inputs["Strength"], 1, node_tab)
    
    val = {
        "type": "converter",
        "construct_name": "normal_map",
        "param": {
            "color" : color,
            "strength" : strength,    
        },
    }
    try_add_tab(node_tab, node_name, val)
    return ret


def parse_clamp(exporter, link, dim, node_tab):
    from_node = link.from_node
    fs = link.from_socket
    output_key = fs.name
    node_name = str(from_node)
    ret = slot_data(node_name, output_key, "")
    val = {
        "type": "converter",
        "construct_name": "clamp",
        "param": {
            "min" : parse_node(exporter, from_node.inputs["Min"], 1, node_tab),
            "max" : parse_node(exporter, from_node.inputs["Max"], 1, node_tab),
            "value" : parse_node(exporter, from_node.inputs["Value"], 1, node_tab),
        },
    }
    try_add_tab(node_tab, node_name, val)
    return ret


def parse_color(exporter, link, dim, node_tab):
    from_node = link.from_node
    fs = link.from_socket
    output_key = fs.name
    node_name = str(from_node)
    ret = slot_data(node_name, output_key, "")
    color = from_node.color
    val = {
        "type": "number",
        "param": {
            "value": [
                color.r,
                color.g,
                color.b,
            ],
        },
    }
    try_add_tab(node_tab, node_name, val)
    return ret


def parse_gamma(exporter, link, dim, node_tab):
    from_node = link.from_node
    output_key = link.from_socket.name
    node_name = str(from_node)
    ret = slot_data(node_name, output_key, "")
    val = {
        "type" : "converter",
        "construct_name" : "gamma",
        "param" : {
            "color" : parse_node(exporter, from_node.inputs["Color"], 3, node_tab),
            "gamma" : parse_node(exporter, from_node.inputs["Gamma"], 1, node_tab),
        },
    }
    try_add_tab(node_tab, node_name, val)
    return ret


def parse_combine_color(exporter, link, dim, node_tab):
    from_node = link.from_node
    output_key = link.from_socket.name
    node_name = str(from_node)
    ret = slot_data(node_name, output_key, "")
    val = {
        "type" : "converter",
        "construct_name" : "combine_color",
        "param" : {
            "channel0" : parse_node(exporter, from_node.inputs["Red"], 1, node_tab),
            "channel1" : parse_node(exporter, from_node.inputs["Green"], 1, node_tab),
            "channel2" : parse_node(exporter, from_node.inputs["Blue"], 1, node_tab),
        },
    }
    try_add_tab(node_tab, node_name, val)
    return ret
    

def parse_combine_xyz(exporter, link, dim, node_tab):
    from_node = link.from_node
    output_key = link.from_socket.name
    node_name = str(from_node)
    ret = slot_data(node_name, output_key, "")
    val = {
        "type" : "converter",
        "construct_name" : "combine_xyz",
        "param" : {
            "x" : parse_node(exporter, from_node.inputs["X"], 1, node_tab),
            "y" : parse_node(exporter, from_node.inputs["Y"], 1, node_tab),
            "z" : parse_node(exporter, from_node.inputs["Z"], 1, node_tab),
        },
    }
    try_add_tab(node_tab, node_name, val)
    return ret


def parse_mix(exporter, link, dim, node_tab):
    from_node = link.from_node
    fs = link.from_socket
    output_key = fs.name
    node_name = str(from_node)
    ret = slot_data(node_name, output_key)
    val = {
        "type": "mix",
        "param": {},
    }
    if not (node_name in node_tab):
        node_tab[node_name] = val
    return ret


def parse_add(exporter, link, dim, node_tab):
    from_node = link.from_node
    fs = link.from_socket
    output_key = fs.name
    node_name = str(from_node)
    ret = slot_data(node_name, fs.name)
    val = {
        "type": "add",
        "param": {
            "mat0": "",
        },
    }
    if not (node_name in node_tab):
        node_tab[node_name] = val
    return ret

    
def parse_math(exporter, link, dim, node_tab):
    from_node = link.from_node
    fs = link.from_socket
    output_key = fs.name
    node_name = str(from_node)
    ret = slot_data(node_name, output_key, "x")
    
    # 获取数学节点的操作类型和输入
    operation = from_node.operation
    value0 = parse_node(exporter, from_node.inputs[0], 1, node_tab)
    value1 = parse_node(exporter, from_node.inputs[1], 1, node_tab) if len(from_node.inputs) > 1 else None
    value2 = parse_node(exporter, from_node.inputs[2], 1, node_tab) if len(from_node.inputs) > 2 else None
    
    val = {
        "type": "converter",
        "construct_name": "math",
        "param": {
            "operation": operation,
            "value0": value0,
            "value1": value1,
            "value2": value2,
            "use_clamp": from_node.use_clamp,
        },
    }
    try_add_tab(node_tab, node_name, val)
    return ret
    
    
def parse_separate_color(exporter, link, dim, node_tab):
    from_node = link.from_node
    fs = link.from_socket
    output_key = fs.name
    node_name = str(from_node)
    ret = slot_data(node_name, fs.name)
    val = {
        "type" : "separate_color",
        "param" : {
            "value" : parse_node(exporter, from_node.inputs["Color"], 3, node_tab),
        }
    }
    if not (node_name in node_tab):
        node_tab[node_name] = val
    return ret


def parse_separate_xyz(exporter, link, dim, node_tab):
    from_node = link.from_node
    fs = link.from_socket
    output_key = fs.name
    node_name = str(from_node)
    ret = slot_data(node_name, fs.name)
    val = {
        "type" : "separate_xyz",
        "param" : {
            "value" : parse_node(exporter, from_node.inputs["Vector"], 3, node_tab),
        }
    }
    if not (node_name in node_tab):
        node_tab[node_name] = val
    return ret


func_dict = {
    "TEX_IMAGE": parse_image_node,
    "TEX_ENVIRONMENT": parse_image_node,
    "MIX_SHADER": parse_mix,
    "ADD_SHADER": parse_add,
    "TEX_COORD": parse_tex_coord,
    "NEW_GEOMETRY" : parse_new_geometry,
    "CAMERA" : parse_camera_data,
    "MAPPING" : parse_vector_mapping,
    "FRESNEL" : parse_fresnel,
    "NORMAL_MAP" : parse_normal_map,
    "CLAMP" : parse_clamp,
    "RGB" : parse_color,
    "GAMMA" : parse_gamma,
    "COMBINE_COLOR" : parse_combine_color,
    "COMBINE_XYZ" : parse_combine_xyz,
    "SEPARATE_COLOR" : parse_separate_color,
    "SEPARATE_XYZ" : parse_separate_xyz,
    "MATH" : parse_math,
}


def parse_node(exporter, socket, dim, node_tab):
    if socket.is_linked:
        link = socket.links[0]
        key = link.from_node.type
        if key in func_dict:
            func = func_dict[key]
            result = func(exporter, link, dim, node_tab)
            # 如果节点解析函数返回None（例如纹理文件不存在），使用默认值
            if result is not None:
                return result
            # 如果返回None，继续使用下面的默认值逻辑
        else:
            print(f"Warning: Unsupported node type '{key}', using default value")
            # 继续使用默认值逻辑
    
    if dim == 1:
        value = socket.default_value
        return {
            "channels": "x",
            "node": {
                "type": "number",
                "param": {
                    "value": value,
                },
            },
        }
    else:
        value = list(socket.default_value)[0:dim]
        return {
            "channels": "xyz",
            "node": {
                "type": "number",
                "param": {
                    "value": value,
                },
            },
        }
