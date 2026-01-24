import bpy
import math
from mathutils import Matrix, Euler, Vector

# Conversion rotation: -90 degrees around X (Z-up -> Y-up)
conv_rot = Matrix.Rotation(-math.pi / 2.0, 4, 'X')

# Configuration
COLLECTION_NAME = "Collision"
TEXT_NAME = "Collision_Output.txt"

# Get the collection
collection = bpy.data.collections.get(COLLECTION_NAME)
if collection is None:
    raise ValueError(f'Collection "{COLLECTION_NAME}" not found')

# Create or overwrite the text datablock
if TEXT_NAME in bpy.data.texts:
    text = bpy.data.texts[TEXT_NAME]
    text.clear()
else:
    text = bpy.data.texts.new(TEXT_NAME)

# Write header
text.write("[Collision]\n\n")

# Iterate over objects in the collection
sorted_objs = sorted(collection.objects, key=lambda o: o.name.lower())
for index, obj in enumerate(sorted_objs):
    
    # Get world matrix
    world_mat = obj.matrix_world.copy()

    # Apply Z-up -> Y-up conversion
    converted_mat = conv_rot @ world_mat
    
    # Decompose converted transform
    loc, rot, scale = converted_mat.decompose()
    eulerrot = rot.to_euler('XYZ')
    converted_scale = Vector((
        scale.x,   # X stays X
        scale.z,   # Z -> Y
        scale.y    # Y -> Z
    ))
    
    # World-space position
    pos = loc
    # Object scale
    scl = converted_scale
    # Visibility state
    enabled = obj.visible_get()
    
    text.write(
        f'colenb{index} = {str(enabled).lower()}\n'
    )
    text.write(
        f'colpos{index} = "{pos.x:.6f} {pos.y:.6f} {pos.z:.6f}"\n'
    )
    text.write(
        f'colscl{index} = "{scl.x:.6f} {scl.y:.6f} {scl.z:.6f}"\n'
    )
