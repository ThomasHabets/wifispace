#!/usr/bin/R --slave -f
args <- commandArgs(trailingOnly = TRUE)
png(filename='wifiscanner.png')
values <- scan(args[1])
plot(density(values, 0.0002))
