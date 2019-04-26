file(GLOB HEADERS "include/graphene/backup_snapshot/*.hpp")

add_library( graphene_backup_snapshot
             backup_snapshot.cpp
           )

target_link_libraries( graphene_backup_snapshot graphene_chain graphene_app )
target_include_directories( graphene_backup_snapshot
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   graphene_backup_snapshot

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
