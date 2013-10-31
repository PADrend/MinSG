#!/usr/bin/Rscript --vanilla

#
# This file is part of the MinSG library.
# Copyright (C) 2013 Benjamin Eikel <benjamin@eikel.org>
#
# This library is subject to the terms of the Mozilla Public License, v. 2.0.
# You should have received a copy of the MPL along with this library; see the 
# file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
#

library(ggplot2)

data <- read.delim("test_statistics.tsv")
pdf("test_statistics.pdf")
ggplot(data=data, aes(class, duration)) + geom_boxplot() + facet_grid(numEventsOverall ~ numEventTypes, scales="free")

ggplot(data=subset(data, class=="Statistics"), aes(x=numEventsOverall, y=duration)) + scale_x_log10() + scale_y_log10() + geom_point() + geom_smooth()

ggplot(data=subset(data, class=="Profiler"), aes(x=numEventsOverall, y=duration)) + scale_x_log10() + scale_y_log10() + geom_point() + geom_smooth()
