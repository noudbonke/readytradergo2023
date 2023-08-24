import dataextraction as de
import matplotlib.pyplot as plt
import numpy as np
import itertools as it

def SMA(priceData, window=20):
    return np.concatenate((np.cumsum(priceData[:window-1])/np.arange(1, window, 1),
                                      np.convolve(priceData, np.ones(window), 'valid')/window));
def EMA(priceData, window=20, smoothing=2):
    factor = smoothing/(window+1);
    res = np.zeros(len(priceData));
    res[:window] = SMA(priceData[:window], window);
    for i in range(window, len(res)):
        res[i] = factor*priceData[i] + (1-factor)*res[i-1];
    return res;

def get_EMA_crossing(priceData, windowShort=10, windowLong=20, smoothingShort=2, smoothingLong=2):
    # one at time where short EMA > long EMA, zero everywhere else
    tmp = np.where(EMA(priceData, window=windowShort, smoothing=smoothingShort) > EMA(priceData, window=windowLong, smoothing=smoothingLong),
                   1, 0)
    # +1: short EMA crossing over in uptick (buy signal)
    # -1: short EMA crossing over in downtick (sell signal)
    # 0 otherwise
    return(np.concatenate((np.zeros(1), tmp[1:]-tmp[:-1])))

market6 = de.OrderBook(r"datanalysis/BGAF_2047/match256_BGAF_2047.log")
# a 1-day EMA is just the price itself
#periods = [1, 4, 8, 12, 16, 20, 32, 40, 60, 80, 120, 160, 200, 240, 280, 320, 360]
periods = np.arange(1, 200, 4)
profitData = np.zeros((int(len(periods)*(len(periods)-1)/2), 3))
for i, comb in enumerate(it.combinations(periods, 2)):
    signal = get_EMA_crossing(market6.fMidPrice, windowShort=comb[0], windowLong=comb[1])
    # sell at bid price (credit); buy at ask price (debit)
    res = np.sum(np.where(signal == -1, market6.eBidPrice, 0)) - np.sum(np.where(signal == 1, market6.eAskPrice, 0))
    profitData[i, :2] = comb; profitData[i, 2] = res



ax = plt.axes(projection='3d')

# Data for a three-dimensional line
ax.scatter3D(profitData[:, 0], profitData[:, 1], profitData[:, 2])
plt.xlabel("short EMA period")
plt.ylabel("long EMA period")
#plt.zlabel("profit")
plt.show()


# plt.plot(market6.eTime, market6.fMidPrice, label='future');
# plt.plot(market6.eTime, EMA(market6.fMidPrice, window=10), label='future EMA')
# plt.plot(market6.eTime, SMA(market6.fMidPrice), label='future SMA')
# #plt.plot(market6.fTime, market6.fMidPrice, label='future');
# plt.legend()
# plt.show()
# plt.plot(market6.eTime, market6.eTime-market6.fTime)
# plt.show()
