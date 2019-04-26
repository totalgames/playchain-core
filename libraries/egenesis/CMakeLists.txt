
add_library( graphene_egenesis_none
   egenesis_none.cpp
   include/graphene/egenesis/egenesis.hpp
   )

target_link_libraries( graphene_egenesis_none graphene_chain fc )
target_include_directories( graphene_egenesis_none
   PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

add_executable( embed_genesis
   embed_genesis.cpp
   )

target_link_libraries( embed_genesis graphene_chain graphene_app graphene_egenesis_none fc )

set( embed_genesis_args
   -t "${CMAKE_CURRENT_SOURCE_DIR}/egenesis_brief.cpp.tmpl---${CMAKE_CURRENT_BINARY_DIR}/egenesis_brief.cpp"
   -t "${CMAKE_CURRENT_SOURCE_DIR}/egenesis_full.cpp.tmpl---${CMAKE_CURRENT_BINARY_DIR}/egenesis_full.cpp"
   )

MESSAGE( STATUS "egenesis: " ${GRAPHENE_EGENESIS_JSON} )

if( GRAPHENE_EGENESIS_JSON )
   list( APPEND embed_genesis_args --genesis-json "${GRAPHENE_EGENESIS_JSON}" )
endif( GRAPHENE_EGENESIS_JSON )

MESSAGE( STATUS "embed_genesis_args: " ${embed_genesis_args} )

OPTION( PLAYCHAIN_TEST_GENESIS_JSON "Apply test for generated genesis.json (ON OR OFF)" OFF )
OPTION( PLAYCHAIN_SUPPRESS_CREATION_OF_GENESIS "Don't launch create_genesis util for test only (ON OR OFF)" OFF )

set(playchain_input_genesis_root_dir ${CMAKE_CURRENT_SOURCE_DIR}/../../genesis)
set(playchain_input_genesis_testnet_dir ${playchain_input_genesis_root_dir}/testnet)
set(playchain_input_genesis_mainnet_dir ${playchain_input_genesis_root_dir}/mainnet)

if (GENESIS_TESTNET)
    set(playchain_input_genesis_dir ${playchain_input_genesis_testnet_dir})
else()
    set(playchain_input_genesis_dir ${playchain_input_genesis_mainnet_dir})
endif()

set(playchain_genesis_testnet_template_json ${playchain_input_genesis_testnet_dir}/genesis.tpl.json)
set(playchain_genesis_mainnet_template_json ${playchain_input_genesis_mainnet_dir}/genesis.tpl.json)
set(playchain_genesis_template_json ${playchain_input_genesis_dir}/genesis.tpl.json)
#users.json: use 'python -m json.tool new_raw_users.json users.json' to update this file
set(playchain_users_json ${playchain_input_genesis_dir}/users.json)
add_custom_target( genesis SOURCES
                   ${playchain_genesis_testnet_template_json}
                   ${playchain_genesis_mainnet_template_json}
                   ${playchain_input_genesis_testnet_dir}/users.json
                   ${playchain_input_genesis_mainnet_dir}/users.json)

if ( NOT PLAYCHAIN_SUPPRESS_CREATION_OF_GENESIS )

    set(create_genesis_args)
    list(APPEND create_genesis_args --input-genesis-json "${playchain_genesis_template_json}")
    list(APPEND create_genesis_args --import-json "${playchain_users_json}")
    list(APPEND create_genesis_args --output-genesis-json "${GRAPHENE_EGENESIS_JSON}")
    if (PLAYCHAIN_TEST_GENESIS_JSON)
        list(APPEND create_genesis_args --test-resut-genesis)
    endif()

    MESSAGE(STATUS "create_genesis_args: " ${create_genesis_args})

    add_custom_command(
        OUTPUT
        "${GRAPHENE_EGENESIS_JSON}"
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMAND create_genesis ${create_genesis_args}
        DEPENDS
        ${playchain_genesis_template_json}
        ${playchain_users_json}
    )
endif()

add_custom_command(
   OUTPUT
      "${CMAKE_CURRENT_BINARY_DIR}/egenesis_brief.cpp"
      "${CMAKE_CURRENT_BINARY_DIR}/egenesis_full.cpp"
   WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
   COMMAND embed_genesis ${embed_genesis_args}
   DEPENDS
      "${GRAPHENE_EGENESIS_JSON}"
      "${CMAKE_CURRENT_SOURCE_DIR}/egenesis_brief.cpp.tmpl"
      "${CMAKE_CURRENT_SOURCE_DIR}/egenesis_full.cpp.tmpl"
      embed_genesis )

add_library( graphene_egenesis_brief "${CMAKE_CURRENT_BINARY_DIR}/egenesis_brief.cpp" include/graphene/egenesis/egenesis.hpp )
add_library( graphene_egenesis_full  "${CMAKE_CURRENT_BINARY_DIR}/egenesis_full.cpp" include/graphene/egenesis/egenesis.hpp )

target_link_libraries( graphene_egenesis_brief graphene_chain fc )
target_link_libraries( graphene_egenesis_full graphene_chain fc )

target_include_directories( graphene_egenesis_brief
   PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
target_include_directories( graphene_egenesis_full
   PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

INSTALL( TARGETS
   embed_genesis graphene_egenesis_none graphene_egenesis_brief graphene_egenesis_full

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
