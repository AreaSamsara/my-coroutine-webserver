// #include <iostream>
// #include "http/parser/uri.h"
// #include "tcp-ip/address.h"
//
// using namespace UriSpace;
// using std::cout;
// using std::endl;
//
// int main(int argc, char** argv)
//{
//
//	shared_ptr<Uri> uri = Uri::Create("http://www.baidu.com/test/uri?id=100&name=sylar#frg");
//	cout << uri->toString() << endl;
//
//	auto address = uri->createAddress();
//	cout << address->toString() << endl;
//
//	return 0;
// }