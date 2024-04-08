cmake_minimum_required(VERSION 3.27)
project(b09 C)

set(CMAKE_C_STANDARD 11)
add_executable(b09a1
        #        main.c
        #        pipe3.c
        #        pipe4.c
        #        pipe5.c
        #        pipe6.c
        a1.c
        stats_functions.h

        #        pipe.c
        #        pipe1.c
        #        pipe2.c
)


add_executable(b09a3
#        main.c
#        pipe3.c
#        pipe4.c
#        pipe5.c
#        pipe6.c
        main.c
        stats_functions.c
        #        pipe.c
#        pipe1.c
#        pipe2.c
)
