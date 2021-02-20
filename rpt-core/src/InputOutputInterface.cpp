#include <RpT-Core/InputOutputInterface.hpp>


namespace RpT::Core {


InputEvent::InputEvent(const std::string_view actor) : actor_ { actor } {}

std::string_view InputEvent::actor() const {
    return actor_;
}


}
