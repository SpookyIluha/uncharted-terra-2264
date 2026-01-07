import bpy

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
for index, obj in enumerate(collection.objects):
    # World-space position
    pos = obj.matrix_world.translation
    # Object scale
    scl = obj.scale
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