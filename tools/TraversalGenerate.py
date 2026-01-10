import bpy
import math
from mathutils import Matrix, Euler, Vector

# Conversion rotation: -90 degrees around X (Z-up -> Y-up)
conv_rot = Matrix.Rotation(-math.pi / 2.0, 4, 'X')

# Configuration
COLLECTION_NAME = "Traversal"
TEXT_NAME = "Traversal_Output.ini"

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

# Iterate over objects in the collection
for obj in collection.objects:

    # Skip child objects (only non-child / root objects)
    if obj.parent is not None:
        continue

    # World-space position and scale
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

    pos = loc
    scl = converted_scale

    # Visibility (effective visibility: object + collection + viewport)
    enabled = obj.visible_get()

    # Write section header
    text.write(f"[{obj.name}]\n\n")

    text.write(
        f'enabled = {str(enabled).lower()}\n'
    )
    # Write transform data
    text.write(
        f'position = "{pos.x:.6f} {pos.y:.6f} {pos.z:.6f}"\n'
    )
    text.write(
        f'scale = "{scl.x:.6f} {scl.y:.6f} {scl.z:.6f}"\n'
    )

    # Look for a child Empty object
    for child in obj.children:
        if child.type == 'EMPTY':
            exit_pos = child.matrix_world.translation

            converted_exit_pos = Vector((
                exit_pos.x,   # X stays X
                exit_pos.z,   # Z -> Y
                -exit_pos.y    # Y -> -Z
            ))

            text.write(
                f'exitposition = "{converted_exit_pos.x:.6f} {converted_exit_pos.y:.6f} {converted_exit_pos.z:.6f}"\n'
            )
            break  # only the first child Empty is used
        
    text.write("\n")
    
    # Write custom properties
    for prop_name, prop_value in obj.items():
        # Skip Blender-internal custom property storage
        if prop_name == "_RNA_UI":
            continue
        if prop_name == "fast64":
            continue

        # Write value directly; strings are quoted
        if isinstance(prop_value, str):
            text.write(f'{prop_name} = "{prop_value}"\n')
        else:
            text.write(f'{prop_name} = {str(prop_value).lower()}\n')

    # Blank line between objects
    text.write("\n\n")