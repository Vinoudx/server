aux_source_directory(. SRC_LISTS)

add_library(gameserver SHARED ${SRC_LISTS})
target_compile_features(gameserver PUBLIC cxx_std_20)