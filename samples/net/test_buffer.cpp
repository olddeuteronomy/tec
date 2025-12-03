
#include <iostream>

#include "tec/tec_bytes.hpp"


void print_buffer(const tec::Bytes& buf) {
    std::cout
        << "Block=" << buf.block_size() << " "
        << "Capacity=" << buf.capacity() << " "
        << "Size=" << buf.size() << " "
        << "Pos=" << buf.tell()
        << "\n";
}


int main() {
    tec::Bytes buf(4);

    // 1)
    print_buffer(buf);

    // 2)
    unsigned int32{1234};
    buf.write(&int32, sizeof(unsigned));
    print_buffer(buf);

    // 4)
    const char str[] = "Hello, world!";
    buf.write(str, strlen(str));
    print_buffer(buf);

    // 5)
    buf.rewind();
    print_buffer(buf);

    // 6)
    unsigned int32a{0};
    buf.read(&int32a, sizeof(unsigned));
    std::cout << int32a << "\n";
    print_buffer(buf);

    // 7)
    int len = strlen(str);
    char s[BUFSIZ];
    buf.read(s, len);
    s[len] = '\0';
    std::cout << s << "\n";
    print_buffer(buf);

    // 8)
    buf.seek(0, SEEK_SET);
    print_buffer(buf);
    unsigned int32b{0};
    buf.read(&int32b, sizeof(unsigned));
    std::cout << int32b << "\n";
    print_buffer(buf);

    // 9)
    buf.seek(0, SEEK_END);
    print_buffer(buf);
    buf.seek(-13, SEEK_CUR);
    print_buffer(buf);
    char s2[BUFSIZ];
    buf.read(s2, len);
    s2[len] = '\0';
    std::cout << s2 << "\n";
    print_buffer(buf);

    return 0;
}
