#pragma once
#include "io/socket-stream.h"
#include "http/http.h"

namespace HttpSessionSpace
{
	using namespace SocketStreamSpace;
	using namespace HttpSpace;

	class HttpSession : public SocketStream
	{
	public:
		HttpSession(shared_ptr<Socket> socket, const bool owner = true);

		shared_ptr<HttpRequest> receiveRequest();
		int sendResponse(shared_ptr<HttpResponse> response);
	};
}