cmake_minimum_required(VERSION 3.15)

project(LIBSLEEPPRJ LANGUAGES C)

set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD 99)

set(COMMON_FLAGS
    -Wall
    -Werror
    -Wextra
    -fvisibility=hidden
)

set(EV_SRC
    src/libev/ae_epoll.c
    src/libev/anet.c
    src/libev/ev.c
)

set(INTERCEPT_LIB_SRC
    src/vector.c
    src/siphash.c
    src/dict.c
    src/intc_dict.c
    src/intc_ll.c
    src/intc_cli.c
    src/libev/anet.c
    src/log.c
)

set(INTERCEPT_CLI_SRC
    src/intercept.c
)

set(PROXY_SRC
    src/proxy.c
    src/prx_cli.c
    src/fh_proto.c
    src/prx_proto.c
    src/prx_ht.c
    src/ini.c
    src/gen_buf.c
    src/vector.c
    src/gen_sock.c
    src/unix_sock.c
    src/net_ex.c
    src/log.c
    src/md5.c
    src/aes.c
    src/cipher.c
    src/bitmap.c
    src/util.c
    src/spt.c
    ${EV_SRC}
)

set(SERVER_SRC
    src/remote.c
    src/log.c
    src/ini.c
    src/aes.c
    src/md5.c
    src/cJSON.c
    src/cipher.c
    src/redis_cli.c
    src/util.c
    src/vector.c
    src/gen_sock.c
    src/fh_proto.c
    src/rmt_exec.c
    src/rmt_proto.c
    src/rmt_agent.c
    src/gen_buf.c
    src/rmt_sock.c
    ${EV_SRC}
)

add_library(intc SHARED ${INTERCEPT_LIB_SRC})
target_include_directories(intc PRIVATE inc)
target_compile_options(intc PRIVATE ${COMMON_FLAGS})

add_executable(intcli ${INTERCEPT_CLI_SRC})
target_link_libraries(intcli intc)
target_include_directories(intcli PRIVATE inc)

add_executable(proxy ${PROXY_SRC})
target_include_directories(proxy PRIVATE inc)
target_link_libraries(proxy pthread pcre)
target_compile_options(proxy PRIVATE ${COMMON_FLAGS})

# Specify the minimum version you require.
find_package (mongoc-1.0 1.7 REQUIRED)

add_executable(sync_server ${SERVER_SRC})
target_include_directories(sync_server PRIVATE inc)
target_link_libraries(sync_server PRIVATE pthread pcre mongo::mongoc_shared)
target_compile_options(sync_server PRIVATE ${COMMON_FLAGS})
