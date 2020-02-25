#!/usr/bin/env python3
import numpy as np
import matplotlib.pyplot as plt

label = [
"CSV",
"zip CSV",
"Binary",
"",
#"Rice binary",
#"FLAC binary",
"5th deg. poly.",
"4th deg. poly.",
"3rd deg. poly.",
"2nd deg. poly.",
"Linear",
"Difference",
]

data = [
1,
6.39430101906924,
8,
0,
15.7435375654354,
16.6506891059521,
17.0894663675541,
17.0607944899643,
17.7479776359381,
19.0163900005226,
]

fig = plt.figure(1)
label_size = 10
index = np.arange(len(label))
plt.bar(index[:3], data[:3], color='g')
plt.bar(index[4:], data[4:], color='b')
plt.xlabel('Encoding', fontsize=label_size*1.2)
plt.ylabel('Compression Ratio', fontsize=label_size*1.2)
plt.xticks(index, label, fontsize=label_size, rotation=30, ha='right')
plt.title('Compression Ratios of Various Compression Techniques')
plt.axvline(x=3, linewidth=1, color='k', ls='--')
plt.ylim(0,20)
fig.axes[0].text(1,10, 'Traditional\nMethods', ha='center', fontsize=1.5*label_size)
fig.subplots_adjust(bottom=.2)

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
