import re

with open('src/raylib/key_input.h', 'r') as f:
    h_content = f.read()

# Add f_noarrowten to KeyFlag
if 'f_noarrowten' not in h_content:
    h_content = h_content.replace('f_arrowten    // Depends on UseArrowAs10key config',
                                  'f_arrowten,   // Active only if UseArrowAs10key is true\n        f_noarrowten  // Active only if UseArrowAs10key is false')

with open('src/raylib/key_input.h', 'w') as f:
    f.write(h_content)

with open('src/raylib/key_input.cpp', 'r') as f:
    cpp_content = f.read()

# Add capsLockState and kanaLockState
# Wait, I should add them to KeyInput class in .h first.
