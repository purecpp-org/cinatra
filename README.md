#cinatra 
[![Build Status](https://travis-ci.org/topcpporg/cinatra.svg?branch=master)](https://travis-ci.org/topcpporg/cinatra)
[![Coverage Status](https://coveralls.io/repos/topcpporg/cinatra/badge.svg?branch=master&service=github)](https://coveralls.io/github/topcpporg/cinatra?branch=master)
a sinatra inspired modern c++ web framework

#Build on linux

### First: Boost library
#####Debian/Ubuntu:

```
sudo apt-get install libboost-dev
sudo apt-get install libboost-system-dev libboost-coroutine-dev libboost-thread-dev

```

#### ArchLinux
```
sudo pacman -S boost
```

### Then: Make binary
```
git clone https://github.com/topcpporg/cinatra.git
cd cinatra
mkdir build
cd build
cmake ..
make
```

### Roadmap

v0.11版本，主要针对上一版本进行改进和完善。

1. 更好用的api
2. 支持更好用的AOP
3. 支持可扩展的session和cookie
4. 加入json序列化模块

v0.12版本，支持web socket

欢迎大家参与进来开发！

##### Done!

##### chinese wiki:https://github.com/topcpporg/cinatra/wiki
##### english wiki:https://github.com/topcpporg/cinatra/wiki/Introduction

# Used open source library:
### boost: http://www.boost.org/

examples: [https://github.com/topcpporg/cinatra_example](https://github.com/topcpporg/cinatra_example)

QQ group：340713904
