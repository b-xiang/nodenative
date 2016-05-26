#include "native/http/ServerResponse.hpp"
#include "native/http/HttpUtils.hpp"
#include "native/http/Transaction.hpp"

namespace native {
namespace http {

ServerResponse::ServerResponse(std::shared_ptr<Transaction> iTransaction)
    : OutgoingMessage(), _statusCode(200), _transaction(iTransaction) {
  _headers["Content-Type"] = "text/html";
}

ServerResponse::~ServerResponse() {}

void ServerResponse::setHeaderFirstLine(std::stringstream &ioMessageRaw) const {
  ioMessageRaw << "HTTP/1.1 " << _statusCode << " " << GetStatusText(_statusCode) << "\r\n";
}

void ServerResponse::setStatus(int status_code) { _statusCode = status_code; }

Future<void> ServerResponse::sendData(const std::string &str) { return _transaction.lock()->_socket->write(str); }

Future<void> ServerResponse::endData(const std::string &data) {
  std::weak_ptr<Transaction> transactionWeak = _transaction;

  return OutgoingMessage::endData(data).then([transactionWeak]() {
    NNATIVE_DEBUG("close transaction socket");
    return transactionWeak.lock()->close();
  });
}

} /* namespace http */
} /* namespace native */
