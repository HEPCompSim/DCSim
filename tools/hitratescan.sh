#! /bin/bash

for hitrate in $(LANG=en_US seq 0.0 0.1 1.0)
do 
    ./my-executable data/platform-files/hosts.xml 60 10 3600000000 ${hitrate}
done