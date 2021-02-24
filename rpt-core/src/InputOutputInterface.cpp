#include <RpT-Core/InputOutputInterface.hpp>


namespace RpT::Core {

InputOutputInterface::InputOutputInterface() : closed_ { false } {}

void InputOutputInterface::close() {
    closed_ = true;
}

bool InputOutputInterface::closed() const {
    return closed_;
}

}