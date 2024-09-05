cmake_minimum_required(VERSION 3.10) 
# Set the project name 
project(Orion)
# Specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Find GLFW package
find_package(glfw3 REQUIRED)

# Find GLM package
find_package(glm REQUIRED)

find_package(Vulkan REQUIRED)
find_package(Threads REQUIRED)
find_package(X11 REQUIRED)
# Add the executable

# Link the engine library
add_subdirectory(engine)
add_executable(${PROJECT_NAME} main.cpp)
target_link_libraries(Orion PRIVATE Engine)


include_directories(${CMAKE_SOURCE_DIR}/engine/include)


target_include_directories(Orion PRIVATE 
  ${CMAKE_SOURCE_DIR}/engine/include
)

# Link the libraries
target_link_libraries(${PROJECT_NAME} 
  PRIVATE 
  glfw
  glm::glm
  Vulkan::Vulkan
  ${CMAKE_DL_LIBS}
  Threads::Threads
  Xxf86vm
  Xrandr
  Xi
)

# Include directories
target_include_directories(${PROJECT_NAME} 
  PRIVATE 
  ${GLFW_INCLUDE_DIRS}
  ${GLM_INCLUDE_DIRS}
)

add_custom_target(format
  COMMAND clang-format -i -style=file ${CMAKE_SOURCE_DIR}/engine/src/*.cpp ${CMAKE_SOURCE_DIR}/engine/src/vulkan/*.cpp ${CMAKE_SOURCE_DIR}engine/include/engine/vulkan/*.h {CMAKE_SOURCE_DIR}engine/include/engine/*.h
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

add_custom_command(TARGET Orion POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
  "${CMAKE_BINARY_DIR}/engine/libEngine.so"
  "$<TARGET_FILE_DIR:Orion>")
