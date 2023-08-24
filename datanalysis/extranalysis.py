
import pandas as pd

import numpy as np

import matplotlib.pyplot as plt

import dataextraction as da



df = pd.read_csv("newround/match197_score_board.csv")

#print(df.tail)

grouped_df = df.groupby('Team')


# Iterate through each group and plot the P/L against time

fig, ax = plt.subplots(2, 3, figsize = (25, 10))

for team, group in grouped_df:
    if team == "BGAF_2047" or team == "Algorizz_12321": 
        data = group[10:]['ProfitOrLoss']/group[10:]['BuyVolume']
        #group[10:].plot(x='Time', y='ProfitOrLoss', ax=ax[0, 0], label=team)
        ax[0,0].plot(group[10:]["Time"], data, label = team)



# Add axis labels and legend

ax[0,0].set_xlabel('Time')

ax[0,0].set_ylabel('P/L')

ax[0,0].legend()


for team, group in grouped_df:

    group.plot(x='Time', y='TotalFees', ax=ax[0, 1], label=team)



# Add axis labels and legend

ax[0,1].set_xlabel('Time')

ax[0,1].set_ylabel('Total fees')

ax[0,1].legend()



for team, group in grouped_df:

    group.plot(x='Time', y='BuyVolume', ax=ax[1, 0], label=team)



# Add axis labels and legend

ax[1,0].set_xlabel('Time')

ax[1,0].set_ylabel('Buy volume')

ax[1,0].legend()



for team, group in grouped_df:

    group.plot(x='Time', y='SellVolume', ax=ax[1, 1], label=team)



# Add axis labels and legend

ax[1,1].set_xlabel('Time')

ax[1,1].set_ylabel('Sell volume')

ax[1,1].legend()

events = pd.read_csv("BGAF_2047/match253_events.csv")

grouped_events = events.groupby('Competitor')

orderbook = da.OrderBook("newround/match197_BGAF_2047.log")

ax[0,2].plot(orderbook.eTime[2:], orderbook.eOrderBook[2, 2:], label = 'ETF')

ax[0,2].plot(orderbook.fTime[2:], orderbook.fOrderBook[2, 2:], label = 'Future')

sny = events[events["Competitor"] == 'Shedneryan_223251']

orders = sny[sny['Operation'] == "Insert"]

bPrice = orders[orders['Side'] == 'B']

bPrice.plot(x = "Time", y = "Price", ax = ax[0,2])

ax[0,2].set_xlabel('Time')

ax[0,2].set_ylabel('Bid')

ax[0,2].legend()

ax[1,2].plot(orderbook.eTime[2:], orderbook.eOrderBook[0, 2:], label = 'ETF')

ax[1,2].plot(orderbook.fTime[2:], orderbook.fOrderBook[0, 2:], label = 'Future')

sny = events[events["Competitor"] == 'Shedneryan_223251']

orders = sny[sny['Operation'] == "Insert"]

aPrice = orders[orders['Side'] == 'A']

aPrice.plot(x = "Time", y = "Price", ax = ax[1,2])

ax[1,2].set_xlabel('Time')

ax[1,2].set_ylabel('Ask Price')

ax[1,2].legend()

# Show the plot

plt.show()

plt.savefig("new_overview.svg")

bas = pd.merge(bPrice, aPrice, on='Time', how='outer')
bas = bas.sort_values('Time')

fig, ax1 = plt.subplots()
ax1.plot(bas['Time'], bas['Price_y'].interpolate() - bas['Price_x'].interpolate(), label='spread')
ax2 = ax1.twinx()
ax2.plot(orderbook.eTime[2:], orderbook.eMidPrice, color='red', alpha=0.5)
ax1.legend()

fig, ax = plt.subplots(2, 4, figsize = (50, 20))

for team, axes in zip(grouped_df, ax.flat):
    axes.plot(df[df["Team"] == team[0]]['Time'], df[df["Team"] == team[0]]['EtfPosition'], label = team[0])
    ax2 = axes.twinx()
    ax2.plot(orderbook.fTime[2:], orderbook.fMidPrice, color='red', alpha=0.5)
    axes.legend()
plt.savefig("new_etfvol.svg")


fig, ax = plt.subplots(2, 4, figsize = (25, 10))

for team, axes in zip(grouped_df, ax.flat):
    axes.plot(df[df["Team"] == team[0]]['Time'], df[df["Team"] == team[0]]['FuturePosition'], label = team[0])
    ax2 = axes.twinx()
    ax2.plot(orderbook.fTime[2:], orderbook.fMidPrice, color='red', alpha=0.5)
    axes.legend()
plt.savefig("new_futvol.svg")




