# Collect all source files
file(GLOB_RECURSE ENGINE_SOURCES "src/*.cpp")
file(GLOB_RECURSE ENGINE_HEADERS "include/*.h")

# Create the engine library as a shared library (DLL)
add_library(Engine SHARED ${ENGINE_SOURCES} ${ENGINE_HEADERS})


# Find GLFW package
find_package(glfw3 REQUIRED)

# Find GLM package
find_package(glm REQUIRED)

find_package(Vulkan REQUIRED)
find_package(Threads REQUIRED)
find_package(X11 REQUIRED)

# Specify include directories
target_include_directories(Engine 
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${VULKAN_INCLUDE_DIRS}
        ${GLFW_INCLUDE_DIRS}
        ${GLM_INCLUDE_DIRS}
)

# Link libraries
target_link_libraries(Engine 
    PRIVATE 
        ${VULKAN_LIBRARIES}
        glfw
        glm::glm
)

# Set properties for Windows DLL
set_target_properties(Engine PROPERTIES
    WINDOWS_EXPORT_ALL_SYMBOLS ON
    OUTPUT_NAME "Engine"
)
