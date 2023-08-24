import numpy as np
import matplotlib.pyplot as plt
from datetime import datetime
from pykalman import KalmanFilter
import statsmodels.api as sm
import pandas as pd
from scipy.stats import linregress

def autocorr(x):
    var = x.var()
    mean = x.mean()
    xp = x - mean
    result = np.correlate(xp, xp, mode='full')/var/len(x)
    return result[result.size // 2:]

class OrderBook:

    def __init__(self, exchangeData):
        self.t0 = 0 
        self.eOrderBook = np.zeros((4,1))
        self.fOrderBook = np.zeros((4,1))
        self.eTime = np.zeros(1)
        self.fTime = np.zeros(1)
        self.extraction(exchangeData)
        self.eMidPrice = self.eOrderBook[0,2:]+self.eOrderBook[2,2:]/2
        self.fMidPrice = self.fOrderBook[0,2:]+self.fOrderBook[2,2:]/2
        self.eCorr = sm.tsa.stattools.acf(np.diff(self.eMidPrice))
        self.fCorr = sm.tsa.stattools.acf(np.diff(self.eMidPrice))
        self.epCorr = sm.tsa.stattools.pacf(np.diff(self.eMidPrice))
        self.fpCorr = sm.tsa.stattools.pacf(np.diff(self.eMidPrice))

        self.eTime = (1/self.eTime.max())*(900*self.eTime)
        self.fTime = (1/self.fTime.max())*(900*self.fTime)

    def extraction(self, data):
        orderer = ''
        with open(data) as rawdata:
            for line in rawdata:
                linedat = []
                for char in line.split(" "):
                    linedat.append(char)
                if orderer == '[BASE]':
                    self.t0 = (datetime.strptime(linedat[1], '%H:%M:%S.%f') - datetime(1900,1,1)).total_seconds()
                if linedat[8] == 'book':
                    numbers = np.array([[int(linedat[15].replace(";", ""))], [int(linedat[18].replace(";", ""))], [int(linedat[21].replace(";", ""))], [int(linedat[24])]])
                    timeUpdate = (datetime.strptime(linedat[1], '%H:%M:%S.%f') - datetime(1900,1,1)).total_seconds() - self.t0
                    if linedat[11] == 'ETF':
                        self.eOrderBook = np.hstack((self.eOrderBook, numbers))
                        self.eTime = np.hstack((self.eTime, timeUpdate))
                    #print(linedat[1], linedat[11], int(linedat[15].replace(";", "")), int(linedat[18].replace(";", "")), int(linedat[21].replace(";", "")), int(linedat[24]))
            
                    if linedat[11] == 'Future':
                        self.fOrderBook = np.hstack((self.fOrderBook, numbers))
                        self.fTime = np.hstack((self.eTime, timeUpdate))
                orderer = linedat[6]
                    

    def kalman(self, instrument):
        kf = KalmanFilter(transition_matrices = [1], observation_matrices = [1], initial_state_mean = 0, initial_state_covariance = 1, observation_covariance=1, transition_covariance=.01)  
        if instrument == 'future':
            stateMeans, a = kf.filter(self.fMidPrice)
        else:
            stateMeans, a = kf.filter(self.eMidPrice)
        return stateMeans, a
    
    def linreg(self, instrument):
        if instrument == 'etf':
            res = linregress(self.eTime[2:], self.eMidPrice)
            return res
        else:  
            res = linregress(self.fTime[2:], self.fMidPrice)
            return res

class pnl:
    def __init__(self):
        self.data = pd.read_csv('score_board.csv')
        #score.data.loc[score.data["Team"] == 'gelbis1']
    
    def sharpe(self, alg):
        returns = np.diff(self.data.loc[self.data["Team"] == alg]['ProfitOrLoss'][1::10])
        return returns[returns != 0].mean()/returns[returns != 0].std()
    
    def rollCorr(self, alg, data):
        returns = np.diff(np.log(self.data.loc[self.data["Team"] == alg]['ProfitOrLoss']))
        corrDat = pd.DataFrame({'returns': returns, 'data': data})
        return corrDat['returns'].rolling(3).corr(corrDat['data'])

    def returnPlot(self, alg):
        plt.plot(self.data.loc[self.data["Team"] == alg]["Time"], self.data.loc[self.data["Team"] == alg]['ProfitOrLoss'], label=alg)

    def returnHist(self, alg):
        returns = np.nan_to_num(self.data.loc[self.data["Team"] == alg]['ProfitOrLoss'].pct_change(), copy=False, nan=0.0, posinf=0.0, neginf=0.0)[100:]
        plt.hist(returns[returns != 0], range = (-0.2,0.2), bins=50)
        plt.show()

    def linreg(self, alg):
        res = linregress(self.data.loc[self.data["Team"] == alg]["Time"], self.data.loc[self.data["Team"] == alg]['ProfitOrLoss'])
        return res




'''
stockdat = OrderBook("BGAF_2047/match244_BGAF_2047.log")

plt.plot((stockdat.eTime - stockdat.fTime))
plt.show()

for i in ['arbtr_dyn_FAK', 'arbtr_dyn_GFD', 'japanese_paper', 'hybrid', 'basic_plus', 'basic_plus_polyorder', 'bpp_arb']:
    #returndat.returnPlot(i)
    print(i, returndat.sharpe(i))

a1 = stockdat.linreg('etf')
a2 = returndat.linreg('bpp_arb')

plt.plot(sm.tsa.stattools.ccf(np.diff(stockdat.kalman('etf')[0].T[0][100:]), np.diff(returndat.data.loc[returndat.data["Team"] == 'bpp_arb']['ProfitOrLoss'][101:]), 'full'))
plt.show()

fig, ax1 = plt.subplots()
ax1.plot(stockdat.eTime[103:], np.diff(stockdat.kalman('etf')[0].T[0][100:]), color='blue')
ax1.set_ylabel('stock price', color='blue')
ax1.set_xlabel('time')
ax2 = ax1.twinx()
ax2.plot(returndat.data.loc[returndat.data["Team"] == 'bpp_arb']["Time"][1:], np.diff(returndat.data.loc[returndat.data["Team"] == 'bpp_arb']['ProfitOrLoss']), label='bpp_arb', color='red')
ax2.set_ylabel('profit', color='red')
plt.show()'''

