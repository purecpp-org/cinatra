#!/bin/bash

wget ftp://gsapubftp-anonymous@ftp.broadinstitute.org/travis/gcc_4.9.1-1_amd64.deb
sudo apt-get remove cpp libffi-dev
sudo dpkg --install gcc_4.9.1-1_amd64.deb

echo "BEGIN Eliminating old libstdc++"
sudo rm /usr/lib/gcc/i586-mingw32msvc/4.2.1-sjlj/libstdc++.a
sudo rm /usr/lib/gcc/i586-mingw32msvc/4.2.1-sjlj/libstdc++.la
sudo rm /usr/lib/gcc/i586-mingw32msvc/4.2.1-sjlj/libstdc++_s.a
sudo rm /usr/lib/gcc/i586-mingw32msvc/4.2.1-sjlj/libstdc++_sjlj_6.dll
sudo rm /usr/lib/gcc/x86_64-linux-gnu/4.6/libstdc++.a
sudo rm /usr/lib/gcc/x86_64-linux-gnu/4.6/libstdc++.so
sudo rm /usr/lib/x86_64-linux-gnu/libstdc++.so.6
sudo rm /usr/lib/x86_64-linux-gnu/libstdc++.so.6.0.16
echo "END Eliminating old libstdc++"

export LD_LIBRARY_PATH=/usr/lib64
sudo ln -s /usr/lib64/libstd* /usr/lib/x86_64-linux-gnu/
