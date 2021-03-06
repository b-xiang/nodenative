#ifndef __NATIVE_HTTP_OUTGOINGMESSAGE_HPP__
#define __NATIVE_HTTP_OUTGOINGMESSAGE_HPP__

#include "../text.hpp"
#include "MessageBase.hpp"

#include <map>
#include <memory>
#include <string>

namespace native {

namespace http {

class IncomingMessage;

class OutgoingMessage : public MessageBase {
protected:
  OutgoingMessage();
  OutgoingMessage(const IncomingMessage &incomming);
  virtual void setHeaderFirstLine(std::stringstream &ioMessageRaw) const = 0;
  virtual Future<void> sendData(const std::string &data) = 0;

  virtual Future<void> writeData(const std::string &str);
  virtual Future<void> endData(const std::string &body);

public:
  virtual ~OutgoingMessage();

  void setHeader(const std::string &key, const std::string &value);
  bool removeHeader(const std::string &key);

  std::string getMessageHeaderRaw() const;

  bool isSent() { return _sent; }

protected:
  bool _headerSent;
  bool _closed;
  bool _sent;

  bool _last;
  bool _chunkedEncoding;
  bool _useChunkedEncodingByDefault;
  bool _sendDate;
  bool _shouldKeepAlive;

  int _contentLength;
  bool _hasBody;
  std::string _trailer;
};

} /* namespace http */
} /* namespace native */

#endif /* __NATIVE_HTTP_OUTGOINGMESSAGE_HPP__ */
