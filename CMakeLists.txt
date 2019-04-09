cmake_minimum_required (VERSION 2.6)

project(Kinematic_Codec)

set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/bin)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR})

set(GCC_CUSTOM_FLAGS "-Wall -ggdb -Wextra")

add_subdirectory(${CMAKE_SOURCE_DIR}/src)


message("      ___                       ___           ___                                   ___     ")
message("     /__/|        ___          /__/\\         /  /\\          ___       ___          /  /\\    ")
message("    |  |:|       /  /\\         \\  \\:\\       /  /:/_        /  /\\     /  /\\        /  /:/    ")
message("    |  |:|      /  /:/          \\  \\:\\     /  /:/ /\\      /  /:/    /  /:/       /  /:/     ")
message("  __|  |:|     /__/::\\      _____\\__\\:\\   /  /:/ /:/_    /  /:/    /__/::\\      /  /:/  ___ ")
message(" /__/\\_|:|____ \\__\\/\\:\\__  /__/::::::::\\ /__/:/ /:/ /\\  /  /::\\    \\__\\/\\:\\__  /__/:/  /  /\\")
message(" \\  \\:\\/:::::/    \\  \\:\\/\\ \\  \\:\\~~\\~~\\/ \\  \\:\\/:/ /:/ /__/:/\\:\\      \\  \\:\\/\\ \\  \\:\\ /  /:/")
message("  \\  \\::/~~~~      \\__\\::/  \\  \\:\\  ~~~   \\  \\::/ /:/  \\__\\/  \\:\\      \\__\\::/  \\  \\:\\  /:/ ")
message("   \\  \\:\\          /__/:/    \\  \\:\\        \\  \\:\\/:/        \\  \\:\\     /__/:/    \\  \\:\\/:/  ")
message("    \\  \\:\\         \\__\\/      \\  \\:\\        \\  \\::/          \\__\\/     \\__\\/      \\  \\::/   ")
message("     \\__\\/                     \\__\\/         \\__\\/                                 \\__\\/    ")
message("      ___           ___           ___           ___           ___     ")
message("     /\\  \\         /\\  \\         /\\  \\         /\\  \\         /\\  \\    ")
message("    /::\\  \\       /::\\  \\       /::\\  \\       /::\\  \\       /::\\  \\   ")
message("   /:/\\:\\  \\     /:/\\:\\  \\     /:/\\:\\  \\     /:/\\:\\  \\     /:/\\:\\  \\  ")
message("  /:/  \\:\\  \\   /:/  \\:\\  \\   /:/  \\:\\__\\   /::\\~\\:\\  \\   /:/  \\:\\  \\ ")
message(" /:/__/ \\:\\__\\ /:/__/ \\:\\__\\ /:/__/ \\:|__| /:/\\:\\ \\:\\__\\ /:/__/ \\:\\__\\")
message(" \\:\\  \\  \\/__/ \\:\\  \\ /:/  / \\:\\  \\ /:/  / \\:\\~\\:\\ \\/__/ \\:\\  \\  \\/__/")
message("  \\:\\  \\        \\:\\  /:/  /   \\:\\  /:/  /   \\:\\ \\:\\__\\    \\:\\  \\      ")
message("   \\:\\  \\        \\:\\/:/  /     \\:\\/:/  /     \\:\\ \\/__/     \\:\\  \\     ")
message("    \\:\\__\\        \\::/  /       \\::/__/       \\:\\__\\        \\:\\__\\    ")
message("     \\/__/         \\/__/         ~~            \\/__/         \\/__/    ")