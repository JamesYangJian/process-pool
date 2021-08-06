# process-pool

## This is a multi-process model of linux network server, includes:

* epoll/select implementation
* unified signal source
* preactor mode
* half-sync/half-async

## Example
* A simple cgi program used this model to execute cmdline input from client
* use telnet to connect server, then input full path of command file, ex. /usr/bin/ls to execute command remotely

## Notice
* The basic codes are from <<High performance linux server programming>> Chapter #13
* Add implementation for select
* Do more abstraction 

## Make and Run
make clean;make
./simple-cgi 0.0.0.0 4321 8 //Running on localhost port 4321 with 8 child processes

