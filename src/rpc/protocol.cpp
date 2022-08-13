#include "rpc/protocol.h"

using namespace MyRPC;

Protocol::MessageType Protocol::ParseHeader() {
    // TODO
    return Protocol::MESSAGE_REQUEST_REGISTRATION;
}

StringBuffer Protocol::GetContent() {
    // TODO
    return StringBuffer();
}
