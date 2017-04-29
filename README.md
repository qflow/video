# Introduction
# Get sources
    git clone https://github.com/qflow/video.git && cd video
    git submodule update --init --remote --recursive
    git submodule foreach --recursive git pull origin master
    git submodule foreach --recursive git checkout master
