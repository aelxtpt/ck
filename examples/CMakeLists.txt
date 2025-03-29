
project(teste LANGUAGES C CXX)

add_executable(teste main.cpp)

target_link_libraries(teste ck_static)