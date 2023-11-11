add_executable(bflt-objdump main.c objdump.c)

find_package(ZLIB)
if(NOT ZLIB_FOUND)
message(FATAL_ERROR "Zlib not found")
endif()

target_link_libraries(bflt-objdump ZLIB::ZLIB)

include(CheckCSourceCompiles)
check_c_source_compiles("
#include <arpa/inet.h>
int main() {
    return ntohl(0x1000);
}
" INET_NTOHL)
check_c_source_compiles("
#include <Winsock2.h>
int main() {
    return ntohl(0x1000);
}" WINSOCK_NTOHL)
if(INET_NTOHL)
target_compile_definitions(bflt-objdump PRIVATE USE_INET_NTOHL)
elseif(WINSOCK_NTOHL)
target_compile_definitions(bflt-objdump PRIVATE USE_WINSOCK_NTOHL)
else()
target_compile_definitions(bflt-objdump PRIVATE USE_CUSTOM_NTOHL)
endif()

install(TARGETS bflt-objdump DESTINATION bin)
