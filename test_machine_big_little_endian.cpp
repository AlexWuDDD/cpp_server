#include <iostream>
bool isNetByteOrder() {
  unsigned short model = 0x1234;
  char* pmode = (char*)&model;
  if (pmode[0] == 0x12) {
    return true;
  } else {
    return false;
  }
}

int main()
{
  std::cout << std::boolalpha << "isNetByteOrder: " << isNetByteOrder() << std::endl;
  return 0;
}