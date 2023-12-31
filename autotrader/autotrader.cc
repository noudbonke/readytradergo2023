// Copyright 2021 Optiver Asia Pacific Pty. Ltd.
//
// This file is part of Ready Trader Go.
//
//     Ready Trader Go is free software: you can redistribute it and/or
//     modify it under the terms of the GNU Affero General Public License
//     as published by the Free Software Foundation, either version 3 of
//     the License, or (at your option) any later version.
//
//     Ready Trader Go is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU Affero General Public License for more details.
//
//     You should have received a copy of the GNU Affero General Public
//     License along with Ready Trader Go.  If not, see
//     <https://www.gnu.org/licenses/>.
#include <array>

#include <boost/asio/io_context.hpp>

#include <ready_trader_go/logging.h>

#include "../header/bpp_arb.h"

#include <boost/chrono.hpp>

using namespace ReadyTraderGo;

RTG_INLINE_GLOBAL_LOGGER_WITH_CHANNEL(LG_AT, "AUTO")

constexpr int POSITION_LIMIT = 100;
constexpr int TICK_SIZE_IN_CENTS = 100;
constexpr int MIN_BID_NEARST_TICK = (MINIMUM_BID + TICK_SIZE_IN_CENTS) / TICK_SIZE_IN_CENTS * TICK_SIZE_IN_CENTS;
constexpr int MAX_ASK_NEAREST_TICK = MAXIMUM_ASK / TICK_SIZE_IN_CENTS * TICK_SIZE_IN_CENTS;

constexpr int MAX_UNHEDGED_LOTS = 10;

unsigned long buyVolume = POSITION_LIMIT/2;
unsigned long sellVolume = POSITION_LIMIT/2;

boost::chrono::steady_clock::time_point unhedged_lots_timer = boost::chrono::steady_clock::now();

AutoTrader::AutoTrader(boost::asio::io_context& context) : BaseAutoTrader(context)
{
}

void AutoTrader::DisconnectHandler()
{
    BaseAutoTrader::DisconnectHandler();
    RLOG(LG_AT, LogLevel::LL_INFO) << "execution connection lost";
}

void AutoTrader::ErrorMessageHandler(unsigned long clientOrderId,
                                     const std::string& errorMessage)
{
    RLOG(LG_AT, LogLevel::LL_INFO) << "error with order " << clientOrderId << ": " << errorMessage;
    if (clientOrderId != 0 && ((mAsks.count(clientOrderId) == 1) || (mBids.count(clientOrderId) == 1)))
    {
        OrderStatusMessageHandler(clientOrderId, 0, 0, 0);
    }
}

void AutoTrader::HedgeFilledMessageHandler(unsigned long clientOrderId,
                                           unsigned long price,
                                           unsigned long volume)
{
    // RLOG(LG_AT, LogLevel::LL_INFO) << "hedge order " << clientOrderId << " filled for " << volume
    //                                << " lots at $" << price << " average price in cents";
}

void AutoTrader::OrderBookMessageHandler(Instrument instrument,
                                         unsigned long sequenceNumber,
                                         const std::array<unsigned long, TOP_LEVEL_COUNT>& askPrices,
                                         const std::array<unsigned long, TOP_LEVEL_COUNT>& askVolumes,
                                         const std::array<unsigned long, TOP_LEVEL_COUNT>& bidPrices,
                                         const std::array<unsigned long, TOP_LEVEL_COUNT>& bidVolumes)
{
    // RLOG(LG_AT, LogLevel::LL_INFO) << "order book received for " << instrument << " instrument"
    //                                << ": ask prices: " << askPrices[0]
    //                                << "; ask volumes: " << askVolumes[0]
    //                                << "; bid prices: " << bidPrices[0]
    //                                << "; bid volumes: " << bidVolumes[0];

    if (instrument == Instrument::FUTURE){   
        if (mAskId != 0 && askPrices[0] != mAskPrice)
        {
            SendCancelOrder(mAskId);
            mAskId = 0;
        } else if (mAskId !=0){
            sellFlag = 0;
        }
        if (mBidId != 0 && bidPrices[0] != mBidPrice)
        {
            SendCancelOrder(mBidId);
            mBidId = 0;
        } else if (mBidId !=0) {
            buyFlag = 0;
        } 
        //catch case askprices == 0 so we don't sell everything at no gain
        if (sellFlag && sellVolume && askPrices[0]>0)
        {
            SendInsertOrder(++mNextMessageId, Side::SELL, askPrices[0], sellVolume, Lifespan::GOOD_FOR_DAY);
        }
        if (buyFlag && buyVolume)
        {
            SendInsertOrder(++mNextMessageId, Side::BUY, bidPrices[0], buyVolume, Lifespan::GOOD_FOR_DAY);
        }

        if (sellFlag && sellVolume && askPrices[0]>0)
        {
            mAskPrice = askPrices[0];
            // current message ID is for buy
            if (buyFlag && buyVolume){mAskId = mNextMessageId-1;}else{mAskId = mNextMessageId;}
            mAsks.emplace(mAskId);
        }
        
        if (buyFlag && buyVolume)
        {
            mBidPrice = bidPrices[0];
            mBidId = mNextMessageId;
            mBids.emplace(mBidId);
        }

        if (!isHedged && boost::chrono::duration_cast<boost::chrono::seconds>(boost::chrono::steady_clock::now() - unhedged_lots_timer).count() > 55){
            if (mPosition + fPosition < -10){
                SendHedgeOrder(++mNextMessageId, Side::BUY, MAX_ASK_NEAREST_TICK, -(mPosition+fPosition));
                fPosition += (-(mPosition+fPosition));
            } if (mPosition + fPosition > 10){
                SendHedgeOrder(++mNextMessageId, Side::SELL, MIN_BID_NEARST_TICK, (mPosition+fPosition));
                fPosition -= ((mPosition+fPosition));
            }
            isHedged = 1;
        }

        if (mPosition > 98 && fPosition > 0){
            SendHedgeOrder(++mNextMessageId, Side::SELL, MIN_BID_NEARST_TICK, fPosition);
            fPosition = 0;
            if (!isHedged && std::abs(mPosition + fPosition) <= MAX_UNHEDGED_LOTS) {
                isHedged = 1;
            }
        }else if (mPosition < -98 && -fPosition > 0){
            SendHedgeOrder(++mNextMessageId, Side::BUY, MAX_ASK_NEAREST_TICK, -fPosition);
            fPosition = 0;
            if (!isHedged && std::abs(mPosition + fPosition) <= MAX_UNHEDGED_LOTS) {
                isHedged = 1;
            }
        }
        buyFlag = 1; sellFlag = 1;
    }
}

void AutoTrader::OrderFilledMessageHandler(unsigned long clientOrderId,
                                           unsigned long price,
                                           unsigned long volume)
{
    // RLOG(LG_AT, LogLevel::LL_INFO) << "order " << clientOrderId << " filled for " << volume
    //                                << " lots at $" << price << " cents";
    if (mAsks.count(clientOrderId) == 1)
    {
        mPosition -= (long)volume;
    }
    else if (mBids.count(clientOrderId) == 1)
    {
        mPosition += (long)volume;
    }
    if (isHedged && std::abs(mPosition + fPosition) > MAX_UNHEDGED_LOTS){
        unhedged_lots_timer = boost::chrono::steady_clock::now();
        isHedged = 0;
    } else if (!isHedged && std::abs(mPosition + fPosition) <= MAX_UNHEDGED_LOTS) {
        isHedged = 1;
    }

    sellVolume = (POSITION_LIMIT + mPosition)/2;
    buyVolume = (POSITION_LIMIT - mPosition)/2;
}

void AutoTrader::OrderStatusMessageHandler(unsigned long clientOrderId,
                                           unsigned long fillVolume,
                                           unsigned long remainingVolume,
                                           signed long fees)
{
    if (remainingVolume == 0)
    {
        mAsks.erase(clientOrderId);
        mBids.erase(clientOrderId);
    }
}

void AutoTrader::TradeTicksMessageHandler(Instrument instrument,
                                          unsigned long sequenceNumber,
                                          const std::array<unsigned long, TOP_LEVEL_COUNT>& askPrices,
                                          const std::array<unsigned long, TOP_LEVEL_COUNT>& askVolumes,
                                          const std::array<unsigned long, TOP_LEVEL_COUNT>& bidPrices,
                                          const std::array<unsigned long, TOP_LEVEL_COUNT>& bidVolumes)
{
    // RLOG(LG_AT, LogLevel::LL_INFO) << "trade ticks received for " << instrument << " instrument"
    //                                << ": ask prices: " << askPrices[0]
    //                                << "; ask volumes: " << askVolumes[0]
    //                                << "; bid prices: " << bidPrices[0]
    //                                << "; bid volumes: " << bidVolumes[0];
}
