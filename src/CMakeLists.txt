
add_executable(command main.c)
add_executable(stream stream_encoder.c fir_predictive_filter.c)
target_link_libraries(stream fixmath)

add_executable(test_fir test_fir.c fir_predictive_filter.c)
target_link_libraries(test_fir fixmath)
