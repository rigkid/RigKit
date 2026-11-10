cmake_minimum_required(VERSION 3.19)

if(NOT TARGET rigEspnow)
	add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../../packs/rigEspnow ${CMAKE_BINARY_DIR}/rigEspnow)
endif()

add_library(rigesp STATIC
	src/App.cpp
	src/Json.cpp
	src/Scene.cpp
	src/Prefs.cpp
	src/HalStub.cpp
	src/HalArduino.cpp
	src/Web.cpp
	src/Audio.cpp
	src/A2dp.cpp
	src/Airplay.cpp
	src/I2s.cpp
	src/Pcm.cpp
)
target_include_directories(rigesp
	PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
	PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src
)
target_include_directories(rigEspnow PRIVATE ${CMAKE_CURRENT_LIST_DIR}/include)
target_link_libraries(rigesp PUBLIC rigEspnow)
target_compile_features(rigesp PUBLIC cxx_std_20)
if(MSVC)
	target_compile_options(rigesp PRIVATE /W4)
	target_compile_options(rigEspnow PRIVATE /W4)
else()
	target_compile_options(rigesp PRIVATE -Wall -Wextra)
	target_compile_options(rigEspnow PRIVATE -Wall -Wextra)
endif()
