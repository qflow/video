# Introduction
# Get sources
    git clone https://github.com/qflow/video.git
    cd video
    git submodule update --init --remote
    git submodule foreach git pull origin master
    git submodule foreach git checkout master
