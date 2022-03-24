#!/usr/bin/env python3
import results
import proposed_dist
from statsmodels.stats.multicomp import pairwise_tukeyhsd
import scipy.stats as stats
import numpy as np
import scikit_posthocs as sp


data = [results.rf, results.lf, results.rt, results.lt, results.rs, results.ls, results.walking, results.running, results.standing, results.sitting, results.bicycling]
titles = ['rf','lf','rt','lt','rs','ls','walking','running','standing','sitting','biking']

fvalue, pvalue = stats.f_oneway(*data)
print(fvalue, pvalue)

endog = []
groups = []

for group, title in zip(data, titles):
    endog.extend(group)
    groups.extend([title] * len(group))

m_comp = pairwise_tukeyhsd(np.asarray(endog), np.asarray(groups), 0.05)
print(m_comp)

data_seg = [results.rf, results.lf, results.rt, results.lt, results.rs, results.ls]
data_act = [results.walking, results.running, results.standing, results.sitting, results.bicycling]
f_comp = stats.friedmanchisquare(*data_seg)
print(f_comp)

#data = [proposed_dist.rice, proposed_dist.spline, proposed_dist.fifth, proposed_dist.fourth, proposed_dist.third, proposed_dist.second, proposed_dist.linear, proposed_dist.diff]
data = [proposed_dist.fifth, proposed_dist.fourth, proposed_dist.third, proposed_dist.second, proposed_dist.spline, proposed_dist.linear, proposed_dist.diff]
n_comp = sp.posthoc_nemenyi(data)
print(n_comp)
#n_comp = sp.posthoc_nemenyi_friedman(data)
#print(n_comp)

num_cat = len(data)

rankings = [0 for _ in range(num_cat)]
print(rankings)
for index, _ in enumerate(data[0]):
    values = [data[i][index] for i in range(len(data))]
    for order, value in enumerate(values):
        rankings[order] += sorted(values).index(value)

rankings = [num_cat - a/len(data[0]) for a in rankings]
print(rankings)
print(sum(rankings))
