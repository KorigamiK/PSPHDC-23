cmake_minimum_required(VERSION 3.14)

project(
	jokr
	VERSION 0.1.0
	DESCRIPTION "PSP cURL Jokes app"
	LANGUAGES CXX
)

add_executable(${PROJECT_NAME} main.cpp)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)

if (PSP)
    set(BUILD_PRX 1)
    set(LIBS
        stdc++
        psphttp
        pspgum
        pspgu
        curl
        polarssl
        z
    )
else()
    set(LIBS
        stdc++
        curl
        polarssl
        z
    )
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE ${LIBS})

if (PSP)
    create_pbp_file(
		TARGET ${PROJECT_NAME}
		ICON_PATH NULL
		BACKGROUND_PATH NULL
		PREVIEW_PATH NULL
		TITLE ${PROJECT_DESCRIPTION}
	)

	add_custom_target( 
		RUN_PSP
		DEPENDS ${PROJECT_NAME}
		COMMAND PPSSPPSDL ./EBOOT.PBP
	)
else()
	message(STATUS "Building native version")

	find_package(CURL REQUIRED)
endif()




