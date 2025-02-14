file(GLOB HEADERS "include/graphene/app/*.hpp")
file(GLOB PLAYCHAIN_HEADERS "include/playchain/app/*.hpp")
file(GLOB EGENESIS_HEADERS "../egenesis/include/graphene/app/*.hpp")

add_library( graphene_app 
             api.cpp
             application.cpp
             util.cpp
             database_api.cpp
             plugin.cpp
             playchain_api.cpp
             ${HEADERS}
             ${PLAYCHAIN_HEADERS}
             ${EGENESIS_HEADERS}
           )

# need to link graphene_debug_witness because plugins aren't sufficiently isolated #246
target_link_libraries( graphene_app graphene_market_history graphene_account_history graphene_grouped_orders graphene_chain fc graphene_db graphene_net graphene_utilities graphene_debug_witness )
target_include_directories( graphene_app
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include"
                            "${CMAKE_CURRENT_SOURCE_DIR}/../egenesis/include" )

if(MSVC)
  set_source_files_properties( application.cpp api.cpp database_api.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
endif(MSVC)

INSTALL( TARGETS
   graphene_app

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
INSTALL( FILES ${HEADERS} DESTINATION "include/graphene/app" )
