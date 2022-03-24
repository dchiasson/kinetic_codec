#!/usr/bin/env python3
import numpy as np
import matplotlib.pyplot as plt

label = [
"CSV",
"zip CSV",
"Binary",
#"Rice binary",
#"FLAC binary",
"5th deg. poly.",
"4th deg. poly.",
"3rd deg. poly.",
"2nd deg. poly.",
"1st deg. poly.",
"Delta",
"Linear Prediction",
"Cross-stream Linear",
]

old_data = [
1,
6.39430101906924,
8,
15.7435375654354,
16.6506891059521,
17.0894663675541,
17.0607944899643,
17.7479776359381,
19.0163900005226,
19.1859870218086,
16.1825627149527
]

data = [
1,
3.97198,
8.32,
11.2521984763302,
11.5705458816934,
11.7966305364552,
11.9954132525068,
12.2158506666345,
12.7460540373645,
12.7288487737292,
12.7041915646911,
]


fig = plt.figure(1)
label_size = 13
index = np.arange(len(label))
plt.bar(index[:3], data[:3], color='g')
plt.bar(index[3:], data[3:], color='b')
plt.xlabel('Encoding', fontsize=label_size*1.2)
plt.ylabel('Compression Ratio', fontsize=label_size*1.2)
plt.xticks(index, label, fontsize=label_size, rotation=30, ha='right')
plt.yticks(fontsize=label_size)
plt.title('IMU Data Compression Using Various Techniques', fontsize=label_size*1.2, pad=20)
plt.axvline(x=2.5, linewidth=1, color='k', ls='--')
plt.ylim(0,15)
plt.xlim(-.5,10.5)
fig.axes[0].text(1,11.5, 'Traditional\nMethods', ha='center', fontsize=label_size)
fig.axes[0].text(6.5,13.5, 'Proposed Methods', ha='center', fontsize=label_size)
fig.subplots_adjust(bottom=.4)

plt.show()
"""

plt.figure(2)

label = [
    "Accel",
    "Gyro",
    "Walk",
    "Run",
    "Sit",
    "Stand",
    "Bike",
    "RF",
    "LF",
    "RS",
    "LS",
    "RT",
    "LT",
    "P1",
    "P2",
    "P3",
    "P4",
    "P6",
    "P7",
    "P9",
    "P10",
    "P11",
    "P13",
    "P14",
    "P15",
    "P16",
    "P17",
    "P18",
    ]
data = [
12.257,
16.157,
14.457,
14.457,
14.457,
14.457,
14.457,
15.157,
14.357,
14.657,
14.457,
14.457,
14.457,
14.457,
14.457,
14.457,
14.457,
14.457,
14.457,
14.457,
14.457,
14.457,
14.457,
14.457,
14.457,
14.457,
14.457,
14.457,
        ]

label_size = 10
index = np.arange(len(label))
plt.bar(index, data)
plt.xlabel('Data Type', fontsize=label_size)
plt.ylabel('Compression Ratio', fontsize=label_size)
plt.xticks(index, label, fontsize=label_size, rotation=90)
plt.title('Compression Ratios Compared across type')

plt.show()
"""
