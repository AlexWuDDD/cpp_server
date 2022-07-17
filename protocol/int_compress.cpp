#include <stdint.h>
#include <string>
#include <iostream>
#include <bitset>
//change uint32_t to 1~5 bytes
std::string write7BitEncode(uint32_t value){
  std::cout << std::bitset<sizeof(value)*8>(value) << std::endl;
  std::string ret;
  do{
    unsigned char c = (unsigned char)(value & 0x7F);
    value >>= 7;
    //如果有剩余，则将当前字节的最高bit设置为1，这样在得到一字节后，将其放入字节流容器_ostr中，
    //字节流的容器的类型只要有连续的内存存储序列即可
    if(value){
      c |= 0x80;
    }
    ret.append(1, c);
    std::cout << "c: " << std::bitset<sizeof(c)*8>(c) << ", value: " << std::bitset<sizeof(value)*8>(value) << std::endl;
  }
  while(value);

  return ret;
}

uint32_t read7BitEncode(std::string &str){
  uint32_t ret = 0;
  for(int i = 0; i < str.size(); i++){
    unsigned char c = str[i];
    std::cout << "c: " << std::bitset<sizeof(c)*8>(c) << std::endl;
    uint32_t value = c & 0x7F;
    value <<= (i * 7);
    ret += value;
    if(c & 0x80 == 0){
      break;
    }
  }
  return ret;
}


int main(){
  std::string ret = write7BitEncode(125678);
  std::cout << "ret: " << ret << std::endl;
  std::cout << "read: " << read7BitEncode(ret) << std::endl;
  return 0;
}