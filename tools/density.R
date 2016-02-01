#!/usr/bin/R --slave -f
args <- commandArgs(trailingOnly = TRUE)
png(filename='wifispace.png')
values <- scan(args[1])
plot(density(values, 0.0002))
