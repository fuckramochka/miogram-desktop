import os, sys

cld3_cmake = os.path.join(os.path.dirname(__file__), '..', '..', 'cmake', 'external', 'cld3', 'CMakeLists.txt')
if not os.path.isfile(cld3_cmake):
    print(f"File not found: {cld3_cmake}")
    sys.exit(0)

with open(cld3_cmake, 'r', encoding='utf-8') as f:
    content = f.read()

target = '    find_package(protobuf REQUIRED CONFIG)\n    set(protoc_executable protobuf::protoc)\n    set(protobuf_lib protobuf::libprotobuf-lite)'
replacement = '''    find_package(protobuf CONFIG QUIET)
    if (NOT protobuf_FOUND)
        find_package(Protobuf REQUIRED)
        set(protoc_executable ${Protobuf_PROTOC_EXECUTABLE})
        if (Protobuf_LITE_LIBRARIES)
            set(protobuf_lib ${Protobuf_LITE_LIBRARIES})
        else()
            set(protobuf_lib ${Protobuf_LIBRARIES})
        endif()
    else()
        set(protoc_executable protobuf::protoc)
        set(protobuf_lib protobuf::libprotobuf-lite)
    endif()'''

if target in content:
    content = content.replace(target, replacement)
    target_inc = '    ${cld3_src}\n    ${gen_loc}'
    replacement_inc = '    ${cld3_src}\n    ${gen_loc}\n    ${Protobuf_INCLUDE_DIRS}'
    content = content.replace(target_inc, replacement_inc)
    with open(cld3_cmake, 'w', encoding='utf-8') as f:
        f.write(content)
    print("Successfully patched cld3 CMakeLists.txt with Protobuf fallback")
else:
    print("Target string already replaced or not found")
