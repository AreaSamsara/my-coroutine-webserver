// #include "http.h"
// #include "common/log.h"
// #include "common/singleton.h"
// #include "common/utility.h"
//
// using namespace LogSpace;
// using namespace SingletonSpace;
// using namespace UtilitySpace;
// using namespace HttpSpace;
//
// void test_request()
//{
//	shared_ptr<HttpRequest> request(new HttpRequest);
//	request->setHeader("host", "www.baidu.com");
//	request->setBody("hello world request");
//
//	request->dump(cout) << endl;
// }
//
// void test_response()
//{
//	shared_ptr<HttpResponse> response(new HttpResponse);
//	response->setHeader("X-X", "Mike");
//	response->setBody("hello world response");
//	response->setStatus(HttpStatus::BAD_REQUEST);
//	response->setClose(false);
//
//	response->dump(cout) << endl;
// }
//
// int main(int argc, char** argv)
//{
//	test_request();
//	test_response();
//	return 0;
// }