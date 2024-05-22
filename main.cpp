//#include "address.h"
//#include "log.h"
//#include "singleton.h"
//#include "socket.h"
//#include "iomanager.h"
//
//using namespace AddressSpace;
//using namespace LogSpace;
//using namespace SingletonSpace;
//using namespace SocketSpace;
//using namespace IOManagerSpace;
//
//
//template<class T>
////生成bits位掩码
//static T CreateMask(uint32_t bits)
//{
//	return (1 << (sizeof(T) * 8 - bits)) - 1;
//}
//
//int main(int argc, char** argv)
//{
//	//将当前地址与掩码取与，得到网段地址
//	shared_ptr<IPv4Address> address;
//	uint32_t prefix_len = 1;
//	auto x = CreateMask<uint32_t>(prefix_len);
//
//	cout << x;
//	return 0;
//}