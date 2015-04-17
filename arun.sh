#!/bin/bash

function get_ip() {
	ifconfig $1|sed -n 2p|awk  '{ print $2 }'|awk -F : '{ print $2 }'
}

adb shell /data/local/tmp/evproxy `get_ip`

