enable_testing()

# Test executable
add_executable(b2c_tests
    test_main.c
    test_image.c
    test_disasm.c
    test_elf.c
    test_label.c
    test_data.c
)

target_link_libraries(b2c_tests PRIVATE b2c_decompiler)

# Add tests
add_test(NAME test_image COMMAND b2c_tests --test-image)
add_test(NAME test_disasm COMMAND b2c_tests --test-disasm)
add_test(NAME test_elf COMMAND b2c_tests --test-elf)
add_test(NAME test_label COMMAND b2c_tests --test-label)
add_test(NAME test_data COMMAND b2c_tests --test-data) 