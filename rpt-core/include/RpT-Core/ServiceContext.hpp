#ifndef RPT_MINIGAMES_SERVER_SERVICECONTEXT_HPP
#define RPT_MINIGAMES_SERVER_SERVICECONTEXT_HPP

#include <cstdint>

/**
 * @file ServiceContext.hpp
 */


namespace RpT::Core {


/**
 * @brief Provides a context for services to run, same instance expected for constructs all Service instances
 * registered in same SER Protocol.
 *
 * Instance is used for providing events ID.
 *
 * @author ThisALV, https://github.com/ThisALV
 */
class ServiceContext {
private:
    std::size_t events_count_;
    std::size_t timers_count_;

public:
    /**
     * @brief Initialize events count at 0, same for timers count
     */
    ServiceContext();

    /**
     * @brief Increments events count and retrieves its previous value
     *
     * @note Called by `Service::emitEvent()` for retrieving triggered event ID, shouldn't be called by user.
     *
     * @return Previous value for events count
     */
    std::size_t newEventPushed();

    /**
     * @brief Increments timers count and retrieves its previous value
     *
     * @note Called by `Timer` constructor to determine instance token, shouldn't be called by user
     *
     * @returns A free `Timer` token to use
     */
    std::size_t newTimerCreated();
};


}


#endif //RPT_MINIGAMES_SERVER_SERVICECONTEXT_HPP
