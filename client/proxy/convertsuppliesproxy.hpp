/**
  *  \file client/proxy/convertsuppliesproxy.hpp
  *  \brief Class client::proxy::ConvertSuppliesProxy
  */
#ifndef C2NG_CLIENT_PROXY_CONVERTSUPPLIESPROXY_HPP
#define C2NG_CLIENT_PROXY_CONVERTSUPPLIESPROXY_HPP

#include "client/downlink.hpp"
#include "game/session.hpp"
#include "game/types.hpp"
#include "util/requestsender.hpp"
#include "util/slaverequestsender.hpp"

namespace client { namespace proxy {

    /** Supply Conversion Proxy.
        This proxies a game::actions::ConvertSupplies object.

        - create object
        - call init() to select planet and provide status
        - call sellSupplies() or buySupplies() to perform action

        Bidirectional synchronous: initialisation.

        One-way asynchronous: transaction commit */
    class ConvertSuppliesProxy {
     public:
        struct Status {
            int32_t maxSuppliesToSell;
            int32_t maxSuppliesToBuy;
            bool valid;
            Status()
                : maxSuppliesToSell(0),
                  maxSuppliesToBuy(0),
                  valid(false)
                { }
        };

        /** Constructor.
            \param gameSender Game sender*/
        explicit ConvertSuppliesProxy(util::RequestSender<game::Session> gameSender);

        /** Initialize.
            \param link Downlink
            \param planetId Planet Id
            \param reservedSupplies Supplies to reserve (game::actions::ConvertSupplies::setReservedSupplies())
            \param reservedMoney Money to reserve (game::actions::ConvertSupplies::setReservedMoney())
            \return Status. If the planet does not exist or has wrong state, the status will be reported with valid=false. */
        Status init(Downlink& link, game::Id_t planetId, int32_t reservedSupplies, int32_t reservedMoney);

        /** Sell supplies.
            Submits the request to sell the specified number of supplies.
            If the planet is in wrong state, nothing happens.
            If the planet cannot sell that many supplies, the maximum allowed will be sold.
            \param amount Amount */
        void sellSupplies(int32_t amount);

        /** Buy supplies.
            Submits the request to buy the specified number of supplies.
            If the planet is in wrong state, nothing happens.
            If the planet cannot buy that many supplies, the maximum allowed will be bought.
            \param amount Amount */
        void buySupplies(int32_t amount);

     private:
        class Trampoline;
        util::SlaveRequestSender<game::Session, Trampoline> m_slave;
    };

} }

#endif
