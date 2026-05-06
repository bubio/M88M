import re

with open('src/raylib/disk_dialog.cpp', 'r') as f:
    content = f.read()

# Find the start of System tab block
start_str = "    if (activeTab == 0) { // System"
end_str = "    else if (activeTab == 1) { // Audio"

start_idx = content.find(start_str)
end_idx = content.find(end_str)

if start_idx != -1 and end_idx != -1:
    block = content[start_idx:end_idx]
    
    # We want to replace { x + 20, pY, 120, 20 } with { x + 20, pY, labelW, 20 }
    # and x + 150 with x + 180
    # and x + 340 with x + 370
    # and x + 395 with x + 425
    
    # First inject labelW
    block = block.replace("    if (activeTab == 0) { // System\n", "    if (activeTab == 0) { // System\n        float labelW = 160;\n")
    
    block = block.replace("{ x + 20, pY, 120, 20 }", "{ x + 20, pY, labelW, 20 }")
    block = block.replace("x + 150", "x + 180")
    block = block.replace("x + 340", "x + 370")
    block = block.replace("x + 395", "x + 425")
    
    new_content = content[:start_idx] + block + content[end_idx:]
    with open('src/raylib/disk_dialog.cpp', 'w') as f:
        f.write(new_content)
    print("Fixed layout.")
else:
    print("Could not find block.")
