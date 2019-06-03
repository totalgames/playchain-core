add_executable( playchaind main.cpp )
if( UNIX AND NOT APPLE )
  set(rt_library rt )
endif()

if( USE_GPERFTOOLS )
    find_package( Gperftools QUIET )
    if( GPERFTOOLS_FOUND )
        message( STATUS "Found gperftools; compiling playchaind with TCMalloc")
        list( APPEND PLATFORM_SPECIFIC_LIBS tcmalloc )
    endif()
endif()

# We have to link against graphene_debug_witness because deficiency in our API infrastructure doesn't allow plugins to be fully abstracted #246
target_link_libraries( playchaind

PRIVATE graphene_app graphene_delayed_node graphene_account_history graphene_elasticsearch graphene_market_history graphene_grouped_orders graphene_witness graphene_chain graphene_debug_witness graphene_egenesis_full graphene_snapshot graphene_backup_snapshot graphene_es_objects fc ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} )

install( TARGETS
   playchaind

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
