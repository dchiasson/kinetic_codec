
add_executable(command main.c)
add_executable(stream stream_encoder.c fir_predictive_filter.c)
target_link_libraries(stream libfix)
