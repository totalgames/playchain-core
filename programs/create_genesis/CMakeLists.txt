file(GLOB_RECURSE HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp")

set( SOURCES
   main.cpp
   genesis_parser.cpp
   genesis_mapper.cpp
   file_parser.cpp
   genesis_tester.cpp
   )

add_executable(create_genesis
              ${SOURCES}
              ${HEADERS})
target_link_libraries(create_genesis
                      graphene_app
                      graphene_egenesis_none
                      graphene_chain
                      fc
                       ${CMAKE_DL_LIBS}
                       ${PLATFORM_SPECIFIC_LIBS}
                      )

target_include_directories(create_genesis PUBLIC
                           "${CMAKE_CURRENT_SOURCE_DIR}")

install( TARGETS
    create_genesis

    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
