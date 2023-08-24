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

#include "../header/autotrader.h"

using namespace ReadyTraderGo;

RTG_INLINE_GLOBAL_LOGGER_WITH_CHANNEL(LG_AT, "AUTO")

constexpr int LOT_SIZE = 10;
constexpr int POSITION_LIMIT = 100;
constexpr int TICK_SIZE_IN_CENTS = 100;
constexpr int MIN_BID_NEARST_TICK = (MINIMUM_BID + TICK_SIZE_IN_CENTS) / TICK_SIZE_IN_CENTS * TICK_SIZE_IN_CENTS;
constexpr int MAX_ASK_NEAREST_TICK = MAXIMUM_ASK / TICK_SIZE_IN_CENTS * TICK_SIZE_IN_CENTS;

unsigned long ETFLastAsk = MAX_ASK_NEAREST_TICK;
unsigned long futureLastAsk = MAX_ASK_NEAREST_TICK;
unsigned long ETFLastBid = 0;
unsigned long futureLastBid = 0;

unsigned long priceAdjustment = 0;
unsigned long askPriceAdjustment = 0;
unsigned long bidPriceAdjustment = 0;

unsigned long arbitrageBidPrice = 0;
unsigned long arbitrageAskPrice = MAX_ASK_NEAREST_TICK;
unsigned long newAskPrice = MAX_ASK_NEAREST_TICK;
unsigned long newBidPrice = 0;

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
    RLOG(LG_AT, LogLevel::LL_INFO) << "hedge order " << clientOrderId << " filled for " << volume
                                   << " lots at $" << price << " average price in cents";
}

void AutoTrader::OrderBookMessageHandler(Instrument instrument,
                                         unsigned long sequenceNumber,
                                         const std::array<unsigned long, TOP_LEVEL_COUNT>& askPrices,
                                         const std::array<unsigned long, TOP_LEVEL_COUNT>& askVolumes,
                                         const std::array<unsigned long, TOP_LEVEL_COUNT>& bidPrices,
                                         const std::array<unsigned long, TOP_LEVEL_COUNT>& bidVolumes)
{
    RLOG(LG_AT, LogLevel::LL_INFO) << "order book received for " << instrument << " instrument"
                                   << ": ask prices: " << askPrices[0]
                                   << "; ask volumes: " << askVolumes[0]
                                   << "; bid prices: " << bidPrices[0]
                                   << "; bid volumes: " << bidVolumes[0];

    if (instrument == Instrument::ETF){

        priceAdjustment = - (mPosition / LOT_SIZE) * TICK_SIZE_IN_CENTS;

        arbitrageBidPrice = (askPrices[0]<futureLastBid) ? askPrices[0] + priceAdjustment : 0;
        arbitrageAskPrice = (bidPrices[0]>futureLastAsk) ? bidPrices[0] + priceAdjustment : 0;

        // if (mAskId != 0 && arbitrageAskPrice != 0 && arbitrageAskPrice != mAskPrice)
        // {
        //     SendCancelOrder(mAskId);
        //     mAskId = 0;
        // }
        // if (mBidId != 0 && arbitrageBidPrice != 0 && arbitrageBidPrice != mBidPrice)
        // {
        //     SendCancelOrder(mBidId);
        //     mBidId = 0;
        // }

        if (mAskId == 0 && arbitrageAskPrice != 0 && mPosition > -POSITION_LIMIT)
        {
            mAskId = mNextMessageId++;
            mAskPrice = arbitrageAskPrice;
            SendInsertOrder(mAskId, Side::SELL, arbitrageAskPrice, LOT_SIZE, Lifespan::FILL_AND_KILL);
            mAsks.emplace(mAskId);
        }
        if (mBidId == 0 && arbitrageBidPrice != 0 && mPosition < POSITION_LIMIT)
        {
            mBidId = mNextMessageId++;
            mBidPrice = arbitrageBidPrice;
            SendInsertOrder(mBidId, Side::BUY, arbitrageBidPrice, LOT_SIZE, Lifespan::FILL_AND_KILL);
            mBids.emplace(mBidId);
        }
        ETFLastAsk = askPrices[0];
        ETFLastBid = bidPrices[0];
    }else if (instrument == Instrument::FUTURE){

        arbitrageBidPrice = (ETFLastAsk<bidPrices[0]) ? ETFLastAsk - (mPosition / LOT_SIZE) * TICK_SIZE_IN_CENTS : 0;
        arbitrageAskPrice = (ETFLastBid>askPrices[0]) ? ETFLastBid - (mPosition / LOT_SIZE) * TICK_SIZE_IN_CENTS : 0;

        // if (mAskId != 0 && arbitrageAskPrice != 0 && arbitrageAskPrice != mAskPrice)
        // {
        //     SendCancelOrder(mAskId);
        //     mAskId = 0;
        // }
        // if (mBidId != 0 && arbitrageBidPrice != 0 && arbitrageBidPrice != mBidPrice)
        // {
        //     SendCancelOrder(mBidId);
        //     mBidId = 0;
        // }

        if (mAskId == 0 && arbitrageAskPrice != 0 && mPosition > -POSITION_LIMIT)
        {
            mAskId = mNextMessageId++;
            mAskPrice = arbitrageAskPrice;
            SendInsertOrder(mAskId, Side::SELL, arbitrageAskPrice, LOT_SIZE, Lifespan::FILL_AND_KILL);
            mAsks.emplace(mAskId);
        }
        if (mBidId == 0 && arbitrageBidPrice != 0 && mPosition < POSITION_LIMIT)
        {
            mBidId = mNextMessageId++;
            mBidPrice = arbitrageBidPrice;
            SendInsertOrder(mBidId, Side::BUY, arbitrageBidPrice, LOT_SIZE, Lifespan::FILL_AND_KILL);
            mBids.emplace(mBidId);
        }
        futureLastAsk = askPrices[0];
        futureLastBid = bidPrices[0];
    }
}

void AutoTrader::OrderFilledMessageHandler(unsigned long clientOrderId,
                                           unsigned long price,
                                           unsigned long volume)
{
    RLOG(LG_AT, LogLevel::LL_INFO) << "order " << clientOrderId << " filled for " << volume
                                   << " lots at $" << price << " cents";
    if (mAsks.count(clientOrderId) == 1)
    {
        mPosition -= (long)volume;
        SendHedgeOrder(mNextMessageId++, Side::BUY, MAX_ASK_NEAREST_TICK, volume);
    }
    else if (mBids.count(clientOrderId) == 1)
    {
        mPosition += (long)volume;
        SendHedgeOrder(mNextMessageId++, Side::SELL, MIN_BID_NEARST_TICK, volume);
    }
}

void AutoTrader::OrderStatusMessageHandler(unsigned long clientOrderId,
                                           unsigned long fillVolume,
                                           unsigned long remainingVolume,
                                           signed long fees)
{
    if (remainingVolume == 0)
    {
        if (clientOrderId == mAskId)
        {
            mAskId = 0;
        }
        else if (clientOrderId == mBidId)
        {
            mBidId = 0;
        }

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
    RLOG(LG_AT, LogLevel::LL_INFO) << "trade ticks received for " << instrument << " instrument"
                                   << ": ask prices: " << askPrices[0]
                                   << "; ask volumes: " << askVolumes[0]
                                   << "; bid prices: " << bidPrices[0]
                                   << "; bid volumes: " << bidVolumes[0];
}
