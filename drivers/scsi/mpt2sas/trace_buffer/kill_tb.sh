#!/bin/bash

kill -s HUP `ps -ef | egrep start_tb.sh | grep bash | awk '{print $2}'`
