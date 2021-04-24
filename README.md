# MetaHash network Peer node 2

This repository contains second generation of the Peer Node. 
Fastest requests processing and small resources consumption. 

## Requirements
```shell
cmake >= 3.14.1
gcc >= 8.2
libev = 4.25
boost >= 1.70
libsecp256k1
libfmt = 5.3.0
gperftools >= 2.7
openssl >= 1.1.1b
libconfig >= 1.7.2
```


### Runing from docker image
Please follow these steps to run node from metahash docker repository
1. Create directory for configs and script
2. Copy your node private.key file to this directory
3. Copy peer.network, peer.conf, start.sh from git repository ([install directory](https://github.com/metahashorg/peer_node/tree/master/install)) to this directory
4. Go to your directory and run start.sh script

You should get something like this:
```
$./start.sh
latest: Pulling from metahash/peer_node
Digest: sha256:759edffe2cd0294138414540e716c03acd743b2f8f7ceb62c13ee1ffe9ff885d
docker.io/metahash/peer_node:latest
Error response from daemon: No such container: peer_node
Error: No such container: peer_node
3fa40f29eac5fa7f0784bd3ad7ca009f9eef27629eed9fb200bd5d5420351e9a
[2019-09-03 19:04:47] [info] Starting...
[2019-09-03 19:04:47] [info] Load config file: peer.conf
[2019-09-03 19:04:47] [info] Peer: 2.1.7 (Sep 20 2019, 14:41:50 UTC), rev 6f0582640e06f355f7de15bc4ffcb231a4a5a095
[2019-09-03 19:04:47] [info] OpenSSL: OpenSSL 1.1.1b  26 Feb 2019
[2019-09-03 19:04:47] [info] Config dump:
[2019-09-03 19:04:47] [info] core
[2019-09-03 19:04:47] [info] 	threads: 8
[2019-09-03 19:04:47] [info] 	reqs_dump_ok: false
[2019-09-03 19:04:47] [info] 	reqs_dump_err: true
[2019-09-03 19:04:47] [info] 	queue_size: 10000000
[2019-09-03 19:04:47] [info] stats
[2019-09-03 19:04:47] [info] 	interval_seconds: 1
[2019-09-03 19:04:47] [info] 	url: http://172.104.236.166:5797/save-metrics
[2019-09-03 19:04:47] [info] 	dump_stdout: false
[2019-09-03 19:04:47] [info] 	send: true
[2019-09-03 19:04:47] [info] http server
[2019-09-03 19:04:47] [info] 	ip: 0.0.0.0
[2019-09-03 19:04:47] [info] 	port: 9999
[2019-09-03 19:04:47] [info] 	max_conns: 70000
[2019-09-03 19:04:47] [info] 	backlog: 70000
[2019-09-03 19:04:47] [info] 	conns_clean_interval_seconds: 5
[2019-09-03 19:04:47] [info] 	keep_alive_timeout_seconds: 600
[2019-09-03 19:04:47] [info] 	request_read_timeout_seconds: 10
[2019-09-03 19:04:47] [info] 	message_max_size: 20485760
[2019-09-03 19:04:47] [info] http client
[2019-09-03 19:04:47] [info] 	conns_per_ip: 10
[2019-09-03 19:04:47] [info] 	pool_max_conns: 100
[2019-09-03 19:04:47] [info] 	reponse_timeout_ms: 5000
[2019-09-03 19:04:47] [info] 	retries: 5
[2019-09-03 19:04:47] [info] 	message_max_size: 4096
[2019-09-03 19:04:47] [info] Load network file: peer.network
[2019-09-03 19:04:47] [info] network: net-main
[2019-09-03 19:04:47] [info] 	206.189.11.155:9999
[2019-09-03 19:04:47] [info] 	206.189.11.153:9999
[2019-09-03 19:04:47] [info] 	206.189.11.126:9999
[2019-09-03 19:04:47] [info] 	206.189.11.128:9999
[2019-09-03 19:04:47] [info] 	206.189.11.121:9999
[2019-09-03 19:04:47] [info] 	206.189.11.148:9999
[2019-09-03 19:04:47] [info] 	206.189.11.133:9999
[2019-09-03 19:04:47] [info] 	206.189.11.147:9999
[2019-09-03 19:04:47] [info] Peer started
```

## Build

You could use Dockerfile to build Peer node by himself.

1. Clone repository with dependecies 
```
git clone --recursive https://github.com/metahashorg/peer_node.git
```

2. Run docker build and wait for a while
```
sudo docker build -t user/peer_node:latest .
```

You can use this image with our start script. Please change 
```APP="metahash/peer_node:latest"```
to ```APP="user/peer_node:latest"``` and comment or remove ```sudo docker pull $APP``` line

## Start, stop, restart
1. Check peer status
```
$ sudo docker ps
CONTAINER ID        IMAGE                          COMMAND                  CREATED             STATUS              PORTS               NAMES
ad96039f1bed        metahash/peer_node:latest      "/app/peer_node p…"      13 seconds ago      Up 12 seconds                           peer_node
```
2. Run start script again to update peer_node to latest version and restart
3. Run ```docker stop peer_node``` to temporary stop peer
4. Run
```
sudo docker update --restart=no peer_node
sudo docker stop peer_node
```
to permanently stop peer
