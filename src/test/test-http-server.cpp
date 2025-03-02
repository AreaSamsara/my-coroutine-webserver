// #include "http/server/http-server.h"
// #include "tcp-ip/tcp-server.h"
// #include "common/log.h"
// #include "common/singleton.h"
// #include "common/utility.h"
// #include "http/server/servlet.h"
//
// using namespace LogSpace;
// using namespace SingletonSpace;
// using namespace UtilitySpace;
// using namespace TcpServerSpace;
// using namespace ByteArraySpace;
// using namespace HttpServerSpace;
//
// void run()
//{
//	shared_ptr<HttpServer> server(new HttpServer());
//	shared_ptr<Address> address = Address::LookupAny("0.0.0.0:8020");
//	while (!server->bind(address))
//	{
//		sleep(2);
//	}
//
//	auto servlet_dispatch = server->getServlet_dispatch();
//	servlet_dispatch->addServlet("/sylar/xx",
//		[](shared_ptr<HttpRequest> request,shared_ptr<HttpResponse> response,shared_ptr<HttpSession> session)
//		{
//			response->setBody(request->toString());
//			return 0;
//		}
//	);
//
//	servlet_dispatch->addGlobServlet("/sylar/*",
//		[](shared_ptr<HttpRequest> request, shared_ptr<HttpResponse> response, shared_ptr<HttpSession> session)
//		{
//			response->setBody("Glob:\r\n"+request->toString());
//			return 0;
//		}
//	);
//
//	server->start();
// }
//
// int main(int argc, char** argv)
//{
//	IOManager iomanager(2);
//	iomanager.schedule(run);
//	return 0;
// }