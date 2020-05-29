#!/usr/bin/env python3
import numpy as np
import matplotlib.pyplot as plt
import results

movement_labels = [
        "Run",
        "Walk",
        "Bike",
        "Sit",
        "Stand",
        ]
segment_labels = [
        "R. Foot",
        "L. Foot",
        "R. Shin",
        "L. Shin",
        "R. Thigh",
        "L. Thigh"]
total_labels = [
        "CSV",
        "Delta Coding"]
total_data = [
        1,
        12.7460540373645,
        12.7288487737292]
movement_data = [
        9.94589116552001,
        9.51930553386873,
        17.8668434512873,
        18.6885509305039,
        10.9578584379376]
segment_data = [
        12.6812544614106,
        12.6076663393485,
        12.5430949036446,
        12.7115353451552,
        13.073882659953,
        12.8736024950558]

ylim = (0,30)
label_size = 13
bar_color = '.3'
bar_color_light = '.5'

def draw_movement(labels, data):
    fig = plt.figure()
    index = np.arange(len(labels)) + 1
    #plt.bar(index, data, color=bar_color, linewidth=2)
    plt.boxplot([
        results.running,
        results.walking,
        results.bicycling,
        results.sitting,
        results.standing,
        ])
    #fig.axes[0].yaxis.grid()
    fig.axes[0].set_ylim(ylim)
    fig.axes[0].set_axisbelow(True)

    plt.ylabel('Compression Ratio', fontsize=label_size*1.2)
    plt.xticks(index, labels, fontsize=label_size, rotation=30, ha='right')
    plt.yticks(fontsize=label_size)
    plt.title('Compression of Movement Activities', fontsize=label_size*1.2, pad=20)

def draw_segment(labels, data):
    fig = plt.figure()
    index = np.arange(len(labels)) + 1
    #plt.bar(index, data, color=bar_color, linewidth=2)
    plt.boxplot([
        results.rf,
        results.lf,
        results.rs,
        results.ls,
        results.rt,
        results.lt])
    #fig.axes[0].yaxis.grid()
    fig.axes[0].set_ylim(ylim)
    fig.axes[0].set_axisbelow(True)

    plt.ylabel('Compression Ratio', fontsize=label_size*1.2)
    plt.xticks(index, labels, fontsize=label_size, rotation=30, ha='right')
    plt.yticks(fontsize=label_size)
    plt.title('Compression of Body Segment', fontsize=label_size*1.2, pad=20)

def draw_total(labels, data):
    xlim = (-.5 - .25/2, 1.5 + .25/2)
    ylim = (0,15)
    fig = plt.figure()
    index = np.arange(len(labels))
    plt.bar(index[0], data[0], color=bar_color, width=0.75)
    plt.bar(index[1], data[1], color=bar_color, width=0.75)
    #plt.bar(index[2], data[2], color=bar_color_light, linewidth=2)
    plt.hlines(data[2], *xlim, linestyles='dashed', label='Optimal Limit')
    #fig.axes[0].yaxis.grid()
    fig.axes[0].set_ylim(ylim)
    fig.axes[0].set_xlim(*xlim)
    fig.axes[0].set_axisbelow(True)

    plt.ylabel('Compression Ratio', fontsize=label_size*1.2)
    plt.xticks(index, labels, fontsize=label_size, rotation=30, ha='right')
    plt.yticks(fontsize=label_size)
    plt.title('Best Performing Proposed Method', fontsize=label_size*1.2, pad=20)
    fig.axes[0].text(0,13, 'Optimal Limit', ha='center', fontsize=label_size)

#draw_movement(movement_labels, movement_data)
#draw_segment(segment_labels, segment_data)
draw_total(total_labels, total_data)

plt.show()
