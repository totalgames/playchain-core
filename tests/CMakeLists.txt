file(GLOB COMMON_SOURCES "common/*.cpp")

if( USE_GPERFTOOLS )
    find_package( Gperftools QUIET )
    if( GPERFTOOLS_FOUND )
        message( STATUS "Found gperftools; compiling tests with TCMalloc")
        list( APPEND PLATFORM_SPECIFIC_LIBS tcmalloc )
    endif()
endif()

option(UTESTS_DISABLE_ALL_TESTS "(ON OR OFF)" OFF)
option(UTESTS_ENABLE_CHAIN_TESTS "(ON OR OFF)" ON)
option(UTESTS_ENABLE_PERFORMANCE_TESTS "(ON OR OFF)" OFF)
option(UTESTS_ENABLE_BENCHMARKS_TESTS "(ON OR OFF)" OFF)
option(UTESTS_ENABLE_APP_TESTS "(ON OR OFF)" OFF)
option(UTESTS_ENABLE_CLI_TESTS "(ON OR OFF)" ON)
option(UTESTS_ENABLE_PLAYCHAIN_TESTS "(ON OR OFF)" ON)
option(UTESTS_ENABLE_ES_TESTS "(ON OR OFF)" ON)

if( UTESTS_ENABLE_CHAIN_TESTS AND NOT UTESTS_DISABLE_ALL_TESTS )
    file(GLOB UNIT_TESTS "tests/*.cpp")
    add_executable( chain_test ${UNIT_TESTS} ${COMMON_SOURCES} )
    target_link_libraries( chain_test graphene_chain graphene_app graphene_witness graphene_account_history graphene_elasticsearch graphene_es_objects graphene_egenesis_none fc graphene_wallet ${PLATFORM_SPECIFIC_LIBS} )
    if(MSVC)
      set_source_files_properties( tests/serialization_tests.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
    endif(MSVC)
endif()

if( UTESTS_ENABLE_PLAYCHAIN_TESTS AND NOT UTESTS_DISABLE_ALL_TESTS )
    file(GLOB PLAYCHAIN_TESTS "playchain/*.cpp")
    add_executable( playchain_test ${PLAYCHAIN_TESTS} ${COMMON_SOURCES} )
    target_link_libraries( playchain_test graphene_chain graphene_app graphene_account_history graphene_elasticsearch graphene_es_objects graphene_egenesis_none fc graphene_wallet ${PLATFORM_SPECIFIC_LIBS} )
endif()

if( UTESTS_ENABLE_PERFORMANCE_TESTS AND NOT UTESTS_DISABLE_ALL_TESTS )
    file(GLOB PERFORMANCE_TESTS "performance/*.cpp")
    add_executable( performance_test ${PERFORMANCE_TESTS} ${COMMON_SOURCES} )
    target_link_libraries( performance_test graphene_chain graphene_app graphene_account_history graphene_elasticsearch graphene_es_objects graphene_egenesis_none fc ${PLATFORM_SPECIFIC_LIBS} )
endif()

if( UTESTS_ENABLE_BENCHMARKS_TESTS AND NOT UTESTS_DISABLE_ALL_TESTS )
    file(GLOB BENCH_MARKS "benchmarks/*.cpp")
    add_executable( chain_bench ${BENCH_MARKS} ${COMMON_SOURCES} )
    target_link_libraries( chain_bench graphene_chain graphene_app graphene_account_history graphene_elasticsearch graphene_es_objects graphene_egenesis_none fc ${PLATFORM_SPECIFIC_LIBS} )
endif()

if( UTESTS_ENABLE_APP_TESTS AND NOT UTESTS_DISABLE_ALL_TESTS )
    file(GLOB APP_SOURCES "app/*.cpp")
    add_executable( app_test ${APP_SOURCES} )
    target_link_libraries( app_test graphene_app graphene_account_history graphene_elasticsearch graphene_net graphene_chain graphene_egenesis_none fc ${PLATFORM_SPECIFIC_LIBS} )
endif()

if( UTESTS_ENABLE_CLI_TESTS AND NOT UTESTS_DISABLE_ALL_TESTS )
    file(GLOB CLI_SOURCES "cli/*.cpp")
    add_executable( cli_test ${CLI_SOURCES} "cli/test_applications.hpp" "cli/cli_tests_fixture.hpp" )
    target_link_libraries( cli_test graphene_app graphene_wallet graphene_witness graphene_account_history graphene_elasticsearch graphene_net graphene_chain graphene_egenesis_none fc ${PLATFORM_SPECIFIC_LIBS} )
endif()

if( UTESTS_ENABLE_ES_TESTS AND NOT UTESTS_DISABLE_ALL_TESTS )
    file(GLOB ES_SOURCES "elasticsearch/*.cpp")
    add_executable( es_test ${COMMON_SOURCES} ${ES_SOURCES} )
    target_link_libraries( es_test graphene_chain graphene_app graphene_account_history graphene_elasticsearch graphene_es_objects graphene_egenesis_none fc ${PLATFORM_SPECIFIC_LIBS} )
endif()

add_subdirectory( generate_empty_blocks )
